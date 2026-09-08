/**
 * @file    geofence_runtime.h
 * @brief   宠物项圈电子围栏本地定位算法 — 工程实现头文件
 * @details
 *   参考文档: 宠物项圈_围栏定位算法_工程实现文档.md
 *   算法: PNPOLY 射线法 (多边形) + AABB 包围盒快速预判
 *   状态: UNCONFIG / SAFE / DANGER / OUTSIDE + 边沿检测告警
 *
 *   (寻宠模式):
 *     - 配置入口: Comm_Task 收到 geofenceConfig (switch=1 && action=0)
 *       → Geofence_SetFenceConfig(); action=1(删除) → Geofence_Disable()
 *     - GPS 入口: Comm_Task 100ms 状态监控 → GPS_GetCurrentData() →
 *       Geofence_RuntimeOnGpsFix(NMEA 字符串)
 *     - 状态输出: Geofence_GetState() 供 update_device_state_flags 用
 *     - 告警: Geofence_IsAlarmActive() 供 Comm_Task 即时上报
 */

#ifndef __GEOFENCE_RUNTIME_H__
#define __GEOFENCE_RUNTIME_H__

#include <stdint.h>
#include <stdbool.h>
#include "common_def.h"   /* for Geofence_t, GeofencePoint_t, GEOFENCE_POINT_MAX, state_fence_t */

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * PUBLIC API
 *********************************************************************/

/**
 * @brief 初始化围栏运行时 (开机调用, 状态清为 UNCONFIG)。
 * @note 在 Comm_Task 启动时调用; 等待 APP 下发 geofenceConfig 后再 SetFenceConfig。
 */
void Geofence_RuntimeInit(void);

/**
 * @brief 设置围栏配置并(按 switch_on)启用检测。
 * @param config 指向已填充的 Geofence_t (fenceS[10] / fenceD[10] / fenceId /
 *               switch_on / isSet / valid)
 * @note 内部自动统计有效顶点数(忽略 (0,0) 空点)并计算 AABB 包围盒;
 *       switch_on=1 且有效顶点>=3 时, 下一次 GPS fix 即开始越界检测。
 *       此接口同时会重置位置缓存, 使新围栏立即生效。
 */
void Geofence_SetFenceConfig(const Geofence_t *config);

/**
 * @brief 停止围栏检测 (删除围栏/关闭开关时调用)。
 * @note 清空开关与有效性, 状态回 UNCONFIG, 告警解除。
 */
void Geofence_Disable(void);

/**
 * @brief GPS 有效定位点入口 — 执行越界检测 + 状态边沿告警。
 * @details
 *   内部流程:
 *     1. 围栏开关检查 (switch_on == 0 → 跳过)
 *     2. NMEA 字符串去重 (坐标未变 → 跳过全部计算)
 *     3. NMEA → decimal 度 (DDMM.MMMM → dd.dddddd) + 合法性检查
 *     4. 10 米最小移动阈值过滤 (防边界抖动翻转)
 *     5. AABB 包围盒预判 + PNPOLY 射线法:
 *        fenceS (出界=OUTSIDE) / fenceD (入界=DANGER, 仅 isSet=1 时)
 *     6. 边沿检测: SAFE→OUTSIDE/DANGER 触发告警, 反向解除
 *
 * @param nmea_lat  NMEA 纬度字符串 (DDMM.MMMM, e.g. "3149.332006")
 * @param nmea_lon  NMEA 经度字符串 (DDDMM.MMMM, e.g. "11706.913200")
 * @param lat_dir   0 = N, 1 = S (匹配 gps_Status.lat_dir 约定)
 * @param lon_dir   0 = E, 1 = W
 *
 * @note 在 Comm_Task 100ms 状态监控中调用 (GPS_GetCurrentData 取数)。
 *       围栏开启时设备处于寻宠模式, GPS 常开, 数据实时更新。
 */
void Geofence_RuntimeOnGpsFix(const char *nmea_lat,
                              const char *nmea_lon,
                              uint8_t lat_dir,
                              uint8_t lon_dir);

/**
 * @brief 获取当前围栏状态 (上次 GPS 检测的结果)。
 * @return  state_fence_t:
 *           STATE_FENCE_UNCONFIG (1) — 无有效围栏或开关关闭
 *           STATE_FENCE_SAFE      (2) — 在安全区域内 (fenceS 内且 fenceD 外)
 *           STATE_FENCE_DANGER    (3) — 在危险区域内 (fenceD 内)
 *           STATE_FENCE_OUTSIDE   (4) — 超出安全围栏 (fenceS 外)
 */
state_fence_t Geofence_GetState(void);

/**
 * @brief 获取告警是否活跃 (用于 Comm_Task 的即时上报决策)。
 * @return true = 当前处于越界/危险告警中
 */
bool Geofence_IsAlarmActive(void);

/**
 * @brief 清除告警活跃标志 (APP 确认收到报警后调用)。
 */
void Geofence_ClearAlarm(void);

#ifdef __cplusplus
}
#endif

#endif /* __GEOFENCE_RUNTIME_H__ */
