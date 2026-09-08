/**
 * @file    geofence_runtime.c
 * @brief   宠物项圈电子围栏本地定位算法
 * @details
 *   参考: 宠物项圈_围栏定位算法_工程实现文档.md (已通过 geofence_test.html 验证)
 *
 *   核心算法:
 *     - PNPOLY 射线法 (多边形围栏判定, 半开区间避免顶点重复计数)
 *     - 包围盒 (AABB) 快速预判: 点在大围栏包围盒外 → 直接 OUTSIDE, 免射线计算
 *     - 边沿检测告警 (SAFE→OUTSIDE/DANGER 触发, 反向解除)
 *
 *   与项目对接
 *     - 配置入口: Comm_Task 收到 geofenceConfig 指令 (switch=2 && action=1 &&  action=0)
 *       → Geofence_SetFenceConfig() 启动算法; action=1 → Geofence_Disable()
 *     - GPS 入口: Comm_Task 100ms 状态监控 → GPS_GetCurrentData() →
 *       Geofence_RuntimeOnGpsFix(NMEA 字符串)
 *     - 状态输出: Comm_Task update_device_state_flags → Geofence_GetState()
 *     - 告警标志: Comm_Task 即时上报 → Geofence_IsAlarmActive()
 */

#include "geofence_runtime.h"
#include "om_log.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>     /* sin/cos/atan2/sqrt */

/* ---- 常量 ---- */
#define EARTH_RADIUS_M      6371000.0   /* WGS-84 地球半径 (米) */
#define MIN_MOVE_M          10.0        /* 最小移动阈值 (米): 过滤GPS抖动, 防边界频繁翻转 */
#ifndef M_PI
#define M_PI                3.14159265358979323846
#endif

#define GEOF_RT_TAG "[GEOF_RT]"
#define GEOF_LOG(...)   log_debug(GEOF_RT_TAG " " __VA_ARGS__)

/* ---- 全局状态 ---- */
static Geofence_t    g_fence;              /* 当前围栏配置 (含坐标) */
static state_fence_t g_state = STATE_FENCE_UNCONFIG;
static bool          g_alarm_active = false;

/* 上次有效位置 (移动阈值过滤) */
static bool   g_has_last_pos = false;
static double g_last_lat = 0.0;
static double g_last_lon = 0.0;

/* 上次 NMEA 字符串 (快速去重: 坐标没变则跳过一切计算) */
static char   g_last_nmea_lat[24] = {0};
static char   g_last_nmea_lon[24] = {0};

/* 上次围栏状态 (边沿检测) */
static state_fence_t g_prev_state = STATE_FENCE_UNCONFIG;

/* ---- 围栏有效顶点数 + AABB 包围盒 (SetFenceConfig 时计算) ---- */
static uint8_t  g_s_cnt = 0;              /* fenceS 有效顶点数 */
static uint8_t  g_d_cnt = 0;              /* fenceD 有效顶点数 */
static double   g_s_min_lat, g_s_max_lat, g_s_min_lon, g_s_max_lon;
static double   g_d_min_lat, g_d_max_lat, g_d_min_lon, g_d_max_lon;

/*********************************************************************
 * HELPER: 弧度转换
 *********************************************************************/
static double deg2rad(double deg) {
    return deg * M_PI / 180.0;
}

/*********************************************************************
 * HELPER: Haversine 球面距离 (米)
 *********************************************************************/
static double haversine_distance(double lat1, double lon1,
                                  double lat2, double lon2)
{
    double dlat = deg2rad(lat2 - lat1);
    double dlon = deg2rad(lon2 - lon1);

    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(deg2rad(lat1)) * cos(deg2rad(lat2)) *
               sin(dlon / 2.0) * sin(dlon / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return EARTH_RADIUS_M * c;
}

/*********************************************************************
 * HELPER: 统计有效顶点数 (连续非零坐标点, (0,0) 视为空点终止)
 *********************************************************************/
static uint8_t count_valid_vertices(const GeofencePoint_t *v)
{
    uint8_t n = 0;
    if (v == NULL) return 0U;

    for (uint8_t i = 0U; i < GEOFENCE_POINT_MAX; ++i) {
        if (v[i].lat == 0.0 && v[i].lon == 0.0) {
            break;   /* 空点终止 */
        }
        n++;
    }
    return n;
}

/*********************************************************************
 * HELPER: 计算多边形 AABB 包围盒
 *********************************************************************/
static void calc_bbox(const GeofencePoint_t *v, uint8_t cnt,
                      double *min_lat, double *max_lat,
                      double *min_lon, double *max_lon)
{
    *min_lat = *max_lat = v[0].lat;
    *min_lon = *max_lon = v[0].lon;
    for (uint8_t i = 1U; i < cnt; ++i) {
        if (v[i].lat < *min_lat) *min_lat = v[i].lat;
        if (v[i].lat > *max_lat) *max_lat = v[i].lat;
        if (v[i].lon < *min_lon) *min_lon = v[i].lon;
        if (v[i].lon > *max_lon) *max_lon = v[i].lon;
    }
}

/*********************************************************************
 * HELPER: NMEA → 十进制度
 *   lat: "3149.332006" (DDMM.MMMM)  lon: "11706.913200" (DDDMM.MMMM)
 *********************************************************************/
static double nmea_lat_to_decimal(const char *s, uint8_t lat_dir)
{
    if (s == NULL) return 0.0;
    size_t n = strlen(s);
    if (n < 4U) return 0.0;
    double deg = (double)((s[0] - '0') * 10 + (s[1] - '0'));
    double min = atof(s + 2);
    double v = deg + min / 60.0;
    return (lat_dir == 1U) ? -v : v;   /* 1 = S → negative */
}

static double nmea_lon_to_decimal(const char *s, uint8_t lon_dir)
{
    if (s == NULL) return 0.0;
    size_t n = strlen(s);
    if (n < 5U) return 0.0;
    double deg = (double)((s[0] - '0') * 100 +
                          (s[1] - '0') * 10 +
                          (s[2] - '0'));
    double min = atof(s + 3);
    double v = deg + min / 60.0;
    return (lon_dir == 1U) ? -v : v;   /* 1 = W → negative */
}

/*********************************************************************
 * HELPER: PNPOLY 射线法 (参考算法文档 3.3 节)
 *
 * 从被测点向正东发射水平射线，统计与多边形边的交点数。
 * 奇数交点 → INSIDE，偶数交点 → OUTSIDE。
 *
 * 边界条件处理:
 *  - 严格不等式 (yi > lat) != (yj > lat): 半开区间，顶点只被一条边统计
 *  - 边与纬度线平行 (yi == yj): 条件自动跳过
 *  - 顶点数 < 3: 返回 false
 *********************************************************************/
static bool point_in_polygon(double lon, double lat,
                              const GeofencePoint_t *v, uint8_t count)
{
    if (v == NULL || count < 3U) {
        return false;
    }

    bool inside = false;
    uint8_t j = (uint8_t)(count - 1U);

    for (uint8_t i = 0U; i < count; ++i) {
        /* 条件 1: 边 Vj→Vi 是否跨越点 P 的纬度线 */
        if ((v[i].lat > lat) != (v[j].lat > lat)) {
            /* 条件 2: 计算射线与边的交点经度 */
            double x_intersect = v[j].lon +
                (lat - v[j].lat) / (v[i].lat - v[j].lat) * (v[i].lon - v[j].lon);
            /* 条件 3: 交点在射线正方向 (P 的右侧) */
            if (lon < x_intersect) {
                inside = !inside;
            }
        }
        j = i;
    }

    return inside;
}

/*********************************************************************
 * PUBLIC API
 *********************************************************************/

void Geofence_RuntimeInit(void)
{
    memset(&g_fence, 0, sizeof(g_fence));
    g_state        = STATE_FENCE_UNCONFIG;
    g_alarm_active = false;
    g_has_last_pos = false;
    g_prev_state   = STATE_FENCE_UNCONFIG;
    g_last_nmea_lat[0] = '\0';
    g_last_nmea_lon[0] = '\0';
    g_s_cnt = 0U;
    g_d_cnt = 0U;
}

void Geofence_SetFenceConfig(const Geofence_t *config)
{
    if (config == NULL) return;

    memcpy(&g_fence, config, sizeof(Geofence_t));

    /* 合法性钳位 */
    if (g_fence.fenceId < 1U || g_fence.fenceId > 5U) g_fence.fenceId = 1U;
    if (g_fence.switch_on > 1U) g_fence.switch_on = 0U;
    if (g_fence.isSet > 1U)     g_fence.isSet     = 0U;

    /* 统计有效顶点数 + 计算包围盒 */
    g_s_cnt = count_valid_vertices(g_fence.fenceS);
    g_d_cnt = count_valid_vertices(g_fence.fenceD);
    if (g_s_cnt >= 3U) {
        calc_bbox(g_fence.fenceS, g_s_cnt,
                  &g_s_min_lat, &g_s_max_lat, &g_s_min_lon, &g_s_max_lon);
    }
    if (g_d_cnt >= 3U) {
        calc_bbox(g_fence.fenceD, g_d_cnt,
                  &g_d_min_lat, &g_d_max_lat, &g_d_min_lon, &g_d_max_lon);
    }

    /* 重置位置缓存, 下次 GPS fix 立即触发检测 */
    g_has_last_pos = false;
    g_prev_state   = STATE_FENCE_UNCONFIG;
    g_alarm_active = false;
    g_last_nmea_lat[0] = '\0';
    g_last_nmea_lon[0] = '\0';

    GEOF_LOG("SetFence: fenceId=%u switch=%u isSet=%u sCnt=%u dCnt=%u\r\n",
             g_fence.fenceId, g_fence.switch_on, g_fence.isSet,
             g_s_cnt, g_d_cnt);
}

void Geofence_Disable(void)
{
    g_fence.switch_on = 0U;
    g_fence.valid     = 0U;
    g_state           = STATE_FENCE_UNCONFIG;
    g_alarm_active    = false;
    g_has_last_pos    = false;
    g_prev_state      = STATE_FENCE_UNCONFIG;
    g_last_nmea_lat[0] = '\0';
    g_last_nmea_lon[0] = '\0';

    GEOF_LOG("Disable: fence stopped\r\n");
}

void Geofence_RuntimeOnGpsFix(const char *nmea_lat, const char *nmea_lon,
                              uint8_t lat_dir, uint8_t lon_dir)
{
    /* ----- Step 1: 围栏有效性检查 ----- */
    if (g_fence.valid == 0U || g_fence.switch_on == 0U) {
        g_state = STATE_FENCE_UNCONFIG;
        return;
    }
    if (nmea_lat == NULL || nmea_lon == NULL) {
        return;
    }

    /* ----- Step 2: NMEA 去重 (坐标没变, 跳过全部计算) ----- */
    if (strcmp(nmea_lat, g_last_nmea_lat) == 0 &&
        strcmp(nmea_lon, g_last_nmea_lon) == 0) {
        return;   /* 坐标未变, 保持上次状态 */
    }
    strncpy(g_last_nmea_lat, nmea_lat, sizeof(g_last_nmea_lat) - 1U);
    strncpy(g_last_nmea_lon, nmea_lon, sizeof(g_last_nmea_lon) - 1U);

    /* ----- Step 3: NMEA → 十进制度 + 合法性检查 ----- */
    double lat = nmea_lat_to_decimal(nmea_lat, lat_dir);
    double lon = nmea_lon_to_decimal(nmea_lon, lon_dir);
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0 ||
        (lat == 0.0 && lon == 0.0)) {
        return;   /* 非法坐标 */
    }

    /* ----- Step 4: 最小移动阈值 (10米) 过滤 GPS 抖动 ----- */
    if (g_has_last_pos) {
        double dist = haversine_distance(g_last_lat, g_last_lon, lat, lon);
        if (dist < MIN_MOVE_M) {
            return;   /* 移动不足, 保持上次状态 (避免围栏边界来回翻转) */
        }
    }
    g_last_lat  = lat;
    g_last_lon  = lon;
    g_has_last_pos = true;

    /* ----- Step 5: 围栏坐标有效性 (有效顶点 >= 3) ----- */
    if (g_s_cnt < 3U) {
        g_state = STATE_FENCE_UNCONFIG;
        return;
    }

    /* ----- Step 6: 越界判定 (包围盒快速预判 + 射线法) ----- */
    bool inside_s = false;
    if (lat >= g_s_min_lat && lat <= g_s_max_lat &&
        lon >= g_s_min_lon && lon <= g_s_max_lon) {
        inside_s = point_in_polygon(lon, lat, g_fence.fenceS, g_s_cnt);
    }

    state_fence_t cur;
    if (!inside_s) {
        cur = STATE_FENCE_OUTSIDE;
    } else if (g_fence.isSet != 0U && g_d_cnt >= 3U) {
        /* fenceD: 危险区域 (小), 入界 = DANGER (包围盒预判) */
        bool inside_d = false;
        if (lat >= g_d_min_lat && lat <= g_d_max_lat &&
            lon >= g_d_min_lon && lon <= g_d_max_lon) {
            inside_d = point_in_polygon(lon, lat, g_fence.fenceD, g_d_cnt);
        }
        cur = inside_d ? STATE_FENCE_DANGER : STATE_FENCE_SAFE;
    } else {
        cur = STATE_FENCE_SAFE;
    }

    /* ----- Step 7: 边沿检测告警 -----
     * 不安全态 (OUTSIDE/DANGER) 与安全态 (SAFE) 互转时更新告警:
     *   SAFE → OUTSIDE / SAFE → DANGER : 触发告警
     *   OUTSIDE/DANGER → SAFE          : 解除告警
     *   OUTSIDE ↔ DANGER               : 保持告警 */
    bool now_unsafe  = (cur == STATE_FENCE_OUTSIDE || cur == STATE_FENCE_DANGER);
    bool was_unsafe  = (g_prev_state == STATE_FENCE_OUTSIDE ||
                        g_prev_state == STATE_FENCE_DANGER);
    if (now_unsafe && !was_unsafe) {
        g_alarm_active = true;
        GEOF_LOG("ALARM TRIGGERED: fenceId=%u st=%u lat=%.6f lon=%.6f\r\n",
                 g_fence.fenceId, cur, lat, lon);
    } else if (!now_unsafe && was_unsafe) {
        g_alarm_active = false;
        GEOF_LOG("ALARM CLEARED: st=%u lat=%.6f lon=%.6f\r\n", cur, lat, lon);
    }

    g_prev_state = cur;
    g_state = cur;
}

state_fence_t Geofence_GetState(void)
{
    return g_state;
}

bool Geofence_IsAlarmActive(void)
{
    return g_alarm_active;
}

void Geofence_ClearAlarm(void)
{
    g_alarm_active = false;
}
