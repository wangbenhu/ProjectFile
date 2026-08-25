#ifndef __LFS_PORT_H
#define __LFS_PORT_H

#include <string.h>
#include <stdio.h>
#include "lfs.h"
#include "audio_filename.h"
#include "wifi_config_types.h"

#define LFS_BACKUP_AREA_FLAG (0)
#define LFS_CONFIG_API 0

typedef enum
{
    AUDIO_WRITE_OVERWRITE = 0,   // 覆盖写
    AUDIO_WRITE_APPEND          // 追加写
} audio_write_mode_t;

typedef enum {
    FORMAT_STR,		//STRING
	FORMAT_HEX,		//HEX 
} ReadFileFormat;

typedef enum {
    LFS_CREATE_SUCCEED,		    // 文件系统创建成功
	LFS_CREATE_FAILED,		    // 文件系统创建失败
	LFS_SN_GET_SUCCEED,		    // SN号注获取成功
    LFS_SN_GET_FAILED,          // SN号获取失败
    LFS_USER_INFO_GET_SUCCEED,  // 用户信息获取成功
    LFS_USER_INFO_GET_FAILED,   // 用户信息获取失败
} ReadLfsCreatFlag;

// ========== 目录表 ==========
static const char *fs_directories[] = {
    "system",//存储用户信息和系统信息
    "audio",
    "audio/record",//存储用户录音文件 预留
    "audio/player",//默认的音频 用于产测和功能
	"audio/user",//存储用户配置下发音频
    "history",//历史数据 暂时保留
	"production"//存储一些生产数据，原始设计存储是否产测完成，对接工厂系统
};

// ========== 文件表 ==========
static const char *fs_files[] = {
    "system/user.txt",//用户数据
    "system/system.txt",//系统数据
	"system/wifi.txt",//wifi数据
	"/system/fence.txt",//围栏数据
	"system/config.txt",//配置表
    "audio/record/tmp.wav",
    "audio/player/p1.wav",	
	"production/test.txt"
};
typedef enum {
    FENCE_CLEAR_TARGET_FENCE = 0,
    FENCE_CLEAR_TARGET_SAFE_ZONE = 1,
    FENCE_CLEAR_TARGET_ALL = 2
} FenceClearTarget_t;
typedef enum {
    UBI_AUTH_CODE_ID = 0,		// 鉴权码
	UBI_GPS_ADDRESS_ID,		// GPS 地址信息
    UBI_MQTT_CLIENT_ID_ID,		// MQTT client id 
	UBI_ALL_DATA,
} UserBindInfoID;

/* 系统信息字段ID枚举 */
typedef enum {
    SYS_DEVICE_SN_ID = 0,      // 设备序列号
    SYS_FIRMWARE_LTE_VER_ID,   // LTE软件版本
    SYS_FIRMWARE_GNSS_VER_ID,  // GNSS软件版本
    SYS_MANUFACTURE_DATE_ID,   // 生产日期
    SYS_BATCH_NUMBER_ID,       // 生产批次
    SYS_LAST_BOOT_TIME_ID,     // 最近启动时间
    SYS_TOTAL_RUNTIME_ID,      // 累计运行时间
    SYS_DEVICE_TYPE_ID,     // 设备类型
    SYS_FIELD_COUNT            // 字段总数（用于边界检查）
} SystemInfoID;


/* 测试信息字段ID枚举 */
typedef enum {
    TEST_SENSOR_ID = 0,      // 传感器
  
} ProducitonInfoID;

#if (LFS_BACKUP_AREA_FLAG)
	#define SIM_INFO_ADDR (15*1024*1024)
	#define USER_BIND_INFO_ADDR (SIM_INFO_ADDR + 512*1024) 
#endif 

#define FS_DIR_COUNT   (sizeof(fs_directories)/sizeof(fs_directories[0]))
#define FS_FILE_COUNT  (sizeof(fs_files)/sizeof(fs_files[0]))


void lfs_list_dir(const char *path);
int8_t lfs_init(void);
int log_write_with_rotation(const char *filename,const void *data, size_t len);
int lfs_read_file(const char *filename, void *buffer, lfs_size_t max_len, ReadFileFormat mode);
int lfs_read_file_offset(const char *filename, void *buffer, lfs_off_t offset,lfs_size_t length, ReadFileFormat mode);
int lfs_clear_file_content(const char *path);
int lfs_file_exists_and_empty(const char *path);
const char* system_info_get(SystemInfoID id);
ReadLfsCreatFlag littlefs_create_flag_get(void);
void littlefs_create_flag_set(ReadLfsCreatFlag status);
const char* siminfo_get_sn(void);
/*
* @brief 设置用户绑定信息
* @param data 用户绑定信息指针
* @param len 用户绑定信息长度
* @param id 用户绑定信息ID
*/
int user_bindinfo_set(const char *data,uint16_t len,UserBindInfoID id) ;
/*
* @brief 获取用户绑定信息
* @param id 用户绑定信息ID
* @return char* 用户绑定信息指针
*/
const char* user_bindinfo_get(UserBindInfoID id);
/*
* @brief 设置wifi配置
* @param config wifi配置指针
* @return int 0 成功 -1 失败
* @note wifi配置表最大长度为32个字符，MAC地址固定长度为12个字符，纬度和经度为双精度浮点数，6位小数
*/
int wifi_config_set(const WifiConfig_t *config);
/*
* @brief 获取wifi配置
* @param config wifi配置指针
*/
int wifi_config_get(WifiConfig_t *config);
/*
* @brief 设置围栏配置
* @param config 围栏配置指针
* @return int 0 成功 -1 失败
* @note 围栏配置表最大点数为10个，最少为3个点
*/
int fence_config_set(const FenceConfig_t *config);

/*
* @brief 清除围栏配置
* @param target 清除目标
* @return int 0 成功 -1 失败
*/
int fence_config_clear(FenceClearTarget_t target);
/*
* @brief 获取围栏配置
* @param config 围栏配置指针
* @return int 0 成功 -1 失败
*/
int fence_config_get(FenceConfig_t *config);
/*
* @brief 设置配置表
* @param config 配置表指针
*/
int config_table_set(const ConfigTable_t *config);
/*
* @brief 获取配置表
* @param config 配置表指针
*/  
int config_table_get(ConfigTable_t *config);
/*
* @brief 初始化配置表
* @param default_config 默认配置表指针
*/
int config_table_init_if_empty(const ConfigTable_t *default_config);
/*
* @brief 初始化配置表
*/
int config_table_init_defaults(void);
//void user_bindinfo_del(UserBindInfoID id);
void siminfo_set_sn(const char *sn,uint16_t len);
int user_bindinfo_check(void);

int user_bindinfo_delete_field_by_id(UserBindInfoID id);
/*
* @brief 判断挂载是否成功
* @return bool 挂载状态
*/
bool is_lfs_mounted(void);
int lfs_mount_safe(void);
int lfs_unmount_safe(void);
void lfs_timestamp_init(void);
int lfs_format_safe(void);
int system_info_set(const char *data, uint16_t len, SystemInfoID id);

const char* lfs_system_read(SystemInfoID id);
uint8_t lfs_system_write(char *w_data,char w_len,SystemInfoID id);

bool lfs_audio_exist(const char *filename);
int lfs_get_wav_size_by_filename(const char *filename, uint32_t *size);
int lfs_audio_write_by_filename(const char *filename,
                                const uint8_t *data,
                                size_t len,
                                audio_write_mode_t mode);
int lfs_audio_delete_by_filename(const char *filename);
int flash_copy_wav_to_lfs(uint32_t flash_addr, const char *filename);
int wav_get_file_size_from_iflash(uint32_t flash_addr, uint32_t *file_size);
void update_user_bind_status(void);
int lfs_remove_file_if_exists(const char *path);
#endif
