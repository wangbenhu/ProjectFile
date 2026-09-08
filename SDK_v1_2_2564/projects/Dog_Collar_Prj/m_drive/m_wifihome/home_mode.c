#include "home_mode.h"
#include "lfs_port.h"
#include "../battery/m_battery.h"
#include "task.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * 居家模式参数（可后续走配置表下发）
 * ==========================================================================*/
#define HOME_NOFIX_TIMEOUT_MS    (30UL * 60UL * 1000UL)   /* 无定位触发阈值：30min(常规/寻宠统一) */
#define HOME_SCAN_INTERVAL_MS    (10UL * 60UL * 1000UL)   /* 居家内周期 scan：10min */
#define HOME_MISS_EXIT_TIMES     (2U)                     /* 居家内连续未匹配退出 */
#define HOME_SCAN_TIMEOUT_MS     (15UL * 1000UL)          /* scan 回包超时：按未匹配计一次 */
#define HOME_SUSPECT_MISS_TIMES  (2U)                     /* 阶段连续未匹配确认次数：2 次才算真正户外 */
#define HOME_SUSPECT_RESCAN_DELAY_MS (0)      			  /* 未匹配后再次扫描间隔：0s */

/* ============================================================================
 * 状态机
 * ==========================================================================*/
typedef enum {
    HOME_ST_NORMAL = 0,     /* 常态：监控 30min 无定位 */
    HOME_ST_SUSPECT,        /* 疑似：已触发 scan，等 CAT1 回包 */
    HOME_ST_ACTIVE          /* 居家：GPS 断电，10min 周期 scan */
} home_state_t;

static WifiConfig_t g_home_cfg;         /* 白名单（ssid/mac/lat/lon，wifi_config_get 读取） */
static uint8_t      g_wifi_switch;      /* 配置表 [WIFI] 开关 */
static home_state_t g_state;            /* 当前状态 */
static uint32_t     g_last_fix_tick;    /* 上次定位成功 tick */
static uint8_t      g_miss_cnt;         /* 居家内连续未匹配次数 */
static uint32_t     g_scan_start_tick;  /* 本次 scan 发起时刻 */
static bool         g_scan_inflight;    /* 居家自动 scan 等待回包中 */
static uint32_t     g_next_scan_tick;   /* 居家内下次周期 scan 时刻 */
static uint8_t      g_suspect_miss;     /* 疑似阶段连续未匹配次数 */
static uint32_t     g_suspect_rescan_tick; /* 疑似未匹配后再次扫描时刻(0=未设置) */
static AppControlMode_t g_restore_mode; /* 退出居家时恢复的主模式（跨任务指针须静态存储） */

/* 以 COMM 身份发消息（与 Comm_Task.c 的 SendMessageToTask 同逻辑，模块内自治） */
static void home_send_msg(TASK_CMD_T cmd, TASK_ID_T dest_task_id, void *data)
{
    TaskInfo_t *comm_info = GetTaskInfo(COMM_TASK_ID);
    TaskInfo_t *dst_info = GetTaskInfo(dest_task_id);
    if (comm_info == NULL || dst_info == NULL || dst_info->queue_handle == NULL) {
        return;
    }
    Message_t send_msg = {
        .source_id = COMM_TASK_ID,
        .dest_id = dest_task_id,
        .command = cmd,
        .data = data,
        .data_length = 0
    };
    osMessageQueuePut(dst_info->queue_handle, &send_msg, NULL, 0);
}

/* MAC 输出 12 位 hex */
static void mac_normalize(const char *in, char *out, uint32_t out_cap)
{
    uint32_t o = 0;
    if (out == NULL || out_cap == 0) return;
    out[0] = '\0';
    if (in == NULL) return;
    for (uint32_t i = 0; in[i] != '\0' && o + 1 < out_cap; i++) {
        char c = in[i];
        if (c == ':' || c == '-' || c == ' ') continue;
        if (c >= 'a' && c <= 'f') c = (char)(c - 'a' + 'A');
        out[o++] = c;
    }
    out[o] = '\0';
}

/* SSID 是否可安全跨编码比对：纯可打印 ASCII。
 * 含非 ASCII 字节（中文 SSID：CAT1 返回 GBK、文件系统存 UTF-8）无法直接 strcmp，
 * 此时 SSID 条件自动跳过、退化为纯 MAC 匹配。 */
static int ssid_is_ascii(const char *s)
{
    if (s == NULL) return 0;
    for (; *s != '\0'; s++) {
        if ((unsigned char)*s > 0x7F) return 0;
        if (*s == '\r' || *s == '\n') return 0;
    }
    return 1;
}

/* 匹配规则：
 * ① MAC(BSSID) 必匹配（全局唯一，最终兜底）；
 * ② SSID 双匹配：仅当扫描 SSID 与白名单 SSID 均为 ASCII 时严格 strcmp（大小写敏感）；
 *    任一侧含非 ASCII（中文编码不可比）→ 跳过 SSID 条件，MAC 匹配即通过。
 * 注：QWIFISCAN 无 SSID 编码配置参数，模块中文 SSID 固定 GBK 输出（已查证）。 */
static int wifi_match(const DeviceWifiSsid_t *list, const WifiConfig_t *cfg)
{
    int i;
    char cfg_norm[13];
    if (list == NULL || cfg == NULL) return 0;
    if (list->wifi_count == 0 || cfg->mac[0] == '\0') return 0;
    /* 白名单 MAC 同样归一化（APP 可能下发小写 "228bcc5c68a2"），否则与扫描侧大写比对必失败 */
    mac_normalize(cfg->mac, cfg_norm, sizeof(cfg_norm));

    for (i = 0; i < list->wifi_count && i < 10; i++) {
        const WifiInfo_t *w = &list->wifi_list[i];
        char norm[13];
        mac_normalize(w->mac, norm, sizeof(norm));
        if (strcmp(norm, cfg_norm) != 0) continue;
        /* SSID 双匹配：两边均 ASCII 才执行 */
        if (ssid_is_ascii(w->ssid) && ssid_is_ascii(cfg->ssid) &&
            w->ssid[0] != '\0' && strcmp(w->ssid, "[Hidden]") != 0) {
            if (strcmp(w->ssid, cfg->ssid) != 0) continue;
        }
        log_debug("[HOME] match mac=%s ssid_scan='%s' ssid_cfg='%s'\r\n", norm, w->ssid, cfg->ssid);
        return 1;
    }
    return 0;
}

/* 白名单是否有效（已配置 ssid 或 mac） */
static int home_cfg_valid(void)
{
    return (g_home_cfg.ssid[0] != '\0' || g_home_cfg.mac[0] != '\0');
}

/**
 * wifiSwitch 开 
 **/
bool home_mode_enabled(void)
{
    return (g_wifi_switch != 0) && home_cfg_valid();
}

bool home_mode_is_active(void)
{
    return (g_state == HOME_ST_ACTIVE);
}

bool home_mode_is_configured(void)
{
    return home_cfg_valid();
}

bool home_mode_is_scan_inflight(void)
{
    return g_scan_inflight;
}

/* 触发一次 scan */
static void home_trigger_scan(void)
{
    g_scan_inflight = true;
    g_scan_start_tick = osKernelGetTickCount();
    home_send_msg(TASK_START_WIFISCAN, CAT1_UART_TASK_ID, NULL);
}

/* SUSPECT 未匹配（scan 回包或 15s 超时）统一处理：
 * 连续 2 次未匹配才判定"真正户外"，回 NORMAL 等下一轮定时器；
 * 第 1 次未匹配延迟 60s 再扫一次确认（防单次漏扫/瞬时干扰误判户外）。 */
static void home_suspect_no_match(void)
{
    if (++g_suspect_miss < HOME_SUSPECT_MISS_TIMES) {
        g_suspect_rescan_tick = osKernelGetTickCount() + HOME_SUSPECT_RESCAN_DELAY_MS;
        log_debug("[HOME] suspect miss %d, rescan in %lus\r\n",
                  g_suspect_miss, HOME_SUSPECT_RESCAN_DELAY_MS / 1000UL);
    } else {
        uint8_t miss_total = g_suspect_miss;   /* 清零前先保存，日志才正确 */
        g_state = HOME_ST_NORMAL;
        g_suspect_miss = 0;
        g_suspect_rescan_tick = 0;
        g_last_fix_tick = osKernelGetTickCount();   /* 重置计时，等下一轮定时器(30min) */
        log_debug("[HOME] suspect miss x%d, back to normal (outdoor)\r\n", miss_total);
    }
}

/**
 * 进出居家
 **/
static void enter_home(void)
{
    g_state = HOME_ST_ACTIVE;
    g_miss_cnt = 0;
    g_scan_inflight = false;
    g_next_scan_tick = osKernelGetTickCount() + HOME_SCAN_INTERVAL_MS;

    /* 记住进入前主模式 */
    g_restore_mode.mode = (CurrentModeDataGet() == CURRENT_MODE_SEARCH_PET)
                          ? MODE_SEARCH_PET : MODE_STANDARD;
    g_restore_mode.gps_interval = 0;

    /* 通知 GNSS：GPS 完全断电、停止周期状态机 */
    home_send_msg(TASK_HOME_ENTER, GNSS_UART_TASK_ID, NULL);

    log_debug("[HOME] enter home, prev mode=%d\r\n", g_restore_mode.mode);
}

static void exit_home(void)
{
    /* 复用现有模式切换通道恢复 GPS 周期/持续搜星 */
    home_send_msg(TASK_SYSTEM_MODE, GNSS_UART_TASK_ID, &g_restore_mode);

    g_state = HOME_ST_NORMAL;
    g_miss_cnt = 0;
    g_scan_inflight = false;
    g_last_fix_tick = osKernelGetTickCount();   /* 重新计时 30min */

    log_debug("[HOME] exit home, restore mode=%d\r\n", g_restore_mode.mode);
}

/* ============================================================================
 * 对外接口
 * ==========================================================================*/

void home_mode_init(void)
{
    g_state = HOME_ST_NORMAL;
    g_miss_cnt = 0;
    g_scan_inflight = false;
    g_suspect_miss = 0;
    g_suspect_rescan_tick = 0;
    g_last_fix_tick = osKernelGetTickCount();
    g_wifi_switch = 0;
    memset(&g_home_cfg, 0, sizeof(g_home_cfg));
    memset(&g_restore_mode, 0, sizeof(g_restore_mode));

    if (wifi_config_get(&g_home_cfg) < 0) {
        memset(&g_home_cfg, 0, sizeof(g_home_cfg));
    }
    log_debug("[HOME] init done\r\n");
}

/* wifiSsidConfig 写入后重读白名单；中途配置立即生效 */
void home_mode_on_config_updated(void)
{
    memset(&g_home_cfg, 0, sizeof(g_home_cfg));
    if (wifi_config_get(&g_home_cfg) < 0) {
        memset(&g_home_cfg, 0, sizeof(g_home_cfg));
    }
    /* 配置被清空/开关被关 → 退出居家 */
    if (g_state == HOME_ST_ACTIVE && !home_mode_enabled()) {
        exit_home();
    }
    log_debug("[HOME] config updated, ssid='%s'\r\n", g_home_cfg.ssid);
}

void home_mode_set_wifi_switch(uint8_t on)
{
    g_wifi_switch = (on != 0) ? 1 : 0;
    if (g_state == HOME_ST_ACTIVE && !home_mode_enabled()) {
        exit_home();
    }
    log_debug("[HOME] wifi_switch=%d\r\n", g_wifi_switch);
}

/* 10s 周期轮询 */
void home_mode_poll(void)
{
    uint32_t now = osKernelGetTickCount();

    /* ===== 最高优先级：充电时不执行居家功能 =====
     * 充电 = 狗在家 = GPS 有无已知；WiFi 判断无意义还耗电；
     * 插电时若正在居家则退出生效（恢复 GPS），拔电后重置计时重新判断。 */
    if (PM_GetChargeStatus() != CHARGE_STATUS_NO_CHARGE) {
        if (g_state == HOME_ST_ACTIVE) {
            exit_home();
        }
        return;
    }

    /* 前置门：wifiSwitch 未开或未配置白名单 → 不执行；若正在居家则退出 */
    if (!home_mode_enabled()) {
        if (g_state == HOME_ST_ACTIVE) {
            exit_home();
        }
        return;
    }

    /* ---- 居家状态：10min 周期 scan ---- */
    if (g_state == HOME_ST_ACTIVE) {
        if (now >= g_next_scan_tick) {
            g_next_scan_tick = now + HOME_SCAN_INTERVAL_MS;
            home_trigger_scan();
        }
        return;
    }

    {
        GPS_STATUS_t gps;
        GPS_GetCurrentData(&gps);
        if (gps.gps_status == CHANGE_STATUS_POSITION) {
            g_last_fix_tick = now;
        }
    }

    /* ---- scan 回包超时：按未匹配计一次（SUSPECT 走连续 2 次确认逻辑） ---- */
    if (g_scan_inflight && (now - g_scan_start_tick) >= HOME_SCAN_TIMEOUT_MS) {
        g_scan_inflight = false;
        if (g_state == HOME_ST_SUSPECT) {
            home_suspect_no_match();
        } else {
            g_state = HOME_ST_NORMAL;
            g_last_fix_tick = now;
        }
        log_debug("[HOME] scan timeout\r\n");
        return;
    }

    /* ---- 疑似：第 1 次未匹配后延迟再扫一次（确认扫描） ---- */
    if (g_state == HOME_ST_SUSPECT && !g_scan_inflight &&
        g_suspect_rescan_tick != 0 && now >= g_suspect_rescan_tick) {
        g_suspect_rescan_tick = 0;
        home_trigger_scan();
        log_debug("[HOME] suspect confirm rescan\r\n");
    }

    /* ---- 30min 无定位 → 触发居家判断 scan ---- */
    if (g_state == HOME_ST_NORMAL &&
        (now - g_last_fix_tick) >= HOME_NOFIX_TIMEOUT_MS) {
        g_state = HOME_ST_SUSPECT;
        g_suspect_miss = 0;
        g_suspect_rescan_tick = 0;
        home_trigger_scan();
    }
}

/* CAT1 scan 结果 — 由 CAT1 侧累积所有 URC 后一次性发送完整 DeviceWifiSsid_t，
 * 此处直接匹配全部结果，无需再次累积。 */
void home_mode_on_scan_result(const DeviceWifiSsid_t *list)
{
    if (!g_scan_inflight) return;
    g_scan_inflight = false;

    if (g_state == HOME_ST_SUSPECT) {
        if (wifi_match(list, &g_home_cfg)) {
            enter_home();
        } else {
            home_suspect_no_match();
        }
    } else if (g_state == HOME_ST_ACTIVE) {
        if (wifi_match(list, &g_home_cfg)) {
            g_miss_cnt = 0;
            log_debug("[HOME] home keep\r\n");
        } else if (++g_miss_cnt >= HOME_MISS_EXIT_TIMES) {
            exit_home();
            log_debug("[HOME] miss x%d, exit home\r\n", g_miss_cnt);
        }
    }
}
