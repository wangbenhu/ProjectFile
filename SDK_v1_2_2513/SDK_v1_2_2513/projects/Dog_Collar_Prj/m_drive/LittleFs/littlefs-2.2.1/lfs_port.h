#ifndef __LFS_PORT_H
#define __LFS_PORT_H

#include <string.h>
#include <stdio.h>
#include "lfs.h"

#define LFS_BACKUP_AREA_FLAG (0)
#define LFS_CONFIG_API 0

#define AUDIO_MAX_INDEX 5
#define AUDIO_DIR       "/audio/user"

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
    "system",
    "audio",
    "audio/record",
    "audio/player",
	"audio/user",
    "history",
	"production"
};

// ========== 文件表 ==========
static const char *fs_files[] = {
    "system/user.txt",
    "system/system.txt",
    "audio/record/tmp.wav",
    "audio/player/p1.wav",	
	"production/test.txt"
};
// ========== 文件表 ==========
static const char *fs_files_tmp[] = {
    "system/user.tmp",
    "system/system.tmp",
};
#if (LFS_CONFIG_API)
typedef struct {
    char device_sn[33];      // 设备 SN 号 (32字节，不带结束符)
} SimInfo_t;

typedef struct {
    char auth_code[32];     // 鉴权码 (8字节)
    char wifi_mac[51];   // WIFI SSID 地址 (12字节)
    char gps_address[51];  // GPS 地址信息 (50字节)
	char mqtt_client_id[51];//MQTT client id 
} UserBindInfo_t;
#else 

#endif
typedef enum {
    UBI_AUTH_CODE_ID = 0,		// 鉴权码
	UBI_WIFI_MAC_ID,		// WIFI SSID 地址
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
    SYS_CONFIG_VERSION_ID,     // 配置版本
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

void lfs_port_test1(void);
void lfs_port_test2(void);
void lfs_list_dir(const char *path);
int8_t lfs_init(void);
int log_write_with_rotation(const char *filename,const void *data, size_t len);
int lfs_read_file(const char *filename, void *buffer, lfs_size_t max_len, ReadFileFormat mode);
int lfs_read_file_offset(const char *filename, void *buffer, lfs_off_t offset,lfs_size_t length, ReadFileFormat mode);
int lfs_clear_file_content(const char *path);
int lfs_file_exists_and_empty(const char *path);

ReadLfsCreatFlag littlefs_create_flag_get(void);
void littlefs_create_flag_set(ReadLfsCreatFlag status);
//文件系统数据读写
const char* siminfo_get_sn(void);
int user_bindinfo_set(const char *data,uint16_t len,UserBindInfoID id) ;
const char* user_bindinfo_get(UserBindInfoID id);
//void user_bindinfo_del(UserBindInfoID id);
void siminfo_set_sn(const char *sn,uint16_t len);
int user_bindinfo_check(void);

int user_bindinfo_delete_field_by_id(UserBindInfoID id);
// 判断挂载是否成功
bool is_lfs_mounted(void);
int lfs_mount_safe(void);
int lfs_unmount_safe(void);
void lfs_timestamp_init(void);
int lfs_format_safe(void);
const char* system_info_get(SystemInfoID id);
int system_info_set(const char *data, uint16_t len, SystemInfoID id);

const char* lfs_system_read(SystemInfoID id);
uint8_t lfs_system_write(char *w_data,char w_len,SystemInfoID id);

int flash_copy_wav_to_lfs(uint32_t flash_addr, uint8_t index);
int wav_get_file_size_from_iflash(uint32_t flash_addr, uint32_t *file_size);
int lfs_audio_delete_by_index(uint8_t index);
void update_user_bind_status(void);
#endif
