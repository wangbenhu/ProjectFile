#ifndef __home_mode_h__
#define __home_mode_h__

#include <stdint.h>
#include <stdbool.h>
#include "common_def.h"

/* ============================================================================
 * 居家模式（Home Mode）状态机接口
 *
 * 运行线程：COMM_TASK（不新增 RTOS 任务）
 * 触发链路：GPS 30min 无定位(定位成功重置) → 触发 WiFi scan(CAT1 AT+QWIFISCAN)
 *           → 本地 SSID+MAC 匹配 → 进入居家(GPS断电) / 未匹配重置计时
 * 前置门  ：配置表 wifiSwitch 开启 且 已配置 SSID/MAC 白名单
 * ==========================================================================*/

/* 启动初始化：读白名单(WifiConfig_t)并缓存 wifiSwitch（由 COMM 启动时调用） */
void home_mode_init(void);

/* 10s 周期轮询：无定位计时/触发 scan/居家 10min 周期 scan/scan 超时处理（挂 health tick） */
void home_mode_poll(void);

/* CAT1 scan 结果回包（Comm 消费 TASK_START_WIFISCAN 后回调，仅居家自动 scan 时路由到本函数） */
void home_mode_on_scan_result(const DeviceWifiSsid_t *list);

/* wifiSsidConfig 写入文件系统后调用：重读白名单（中途配置立即生效） */
void home_mode_on_config_updated(void);

/* deviceConfig 的 wifiSwitch 变化时调用（立即生效） */
void home_mode_set_wifi_switch(uint8_t on);

/* 居家激活中（驱动上报 state 串 home 位 = CONFIG_ON） */
bool home_mode_is_active(void);

/* 前置门通过：wifiSwitch 开 且 白名单有效 */
bool home_mode_enabled(void);

/* 白名单已配置（ssid 或 mac 非空）——驱动上报 home 位：1=未配置 2=已配置未开启 3=已配置已开启 */
bool home_mode_is_configured(void);

/* 当前是否有居家自动 scan 在等待回包（Comm 据此区分居家 scan 与 APP 主动 scan） */
bool home_mode_is_scan_inflight(void);

#endif /* __home_mode_h__ */
