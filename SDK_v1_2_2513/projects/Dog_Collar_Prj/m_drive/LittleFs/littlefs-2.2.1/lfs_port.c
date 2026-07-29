#include "lfs_port.h"
#include "../source/drv_flash/drv_flash.h"
#include "om_log.h"
#include "cmsis_os2.h"
#include "lfs.h"
#include "lfs_util.h"
#include "littlefs_time_rtc.h"
/*
*时间戳功能
*断电
*上电
* 音频wav格式文件打包功能
*/
/**
 ******************************************************************************
 * @file    	lfs_port.c
 * @brief   	LittleFs文件系统抽象层函数接口定义
 * @author  	benhu wang
 * @version 	V1.0
 * @language 	GB2312
 * @tab 		4B
 * @date    	2025-08-05
 ******************************************************************************
 * @attention
 * 适用于LitteFs文件系统+外部NorFlash+带时间戳的log系统+多文件读写+文件读写轮转机制。
 *
 * 修改记录：
 * V1.0 - 初始版本
 ******************************************************************************
 */

/**
* @brief config
*/
//#define  TIMESTAMP_CHANGE_STR 	0	//时间戳保存是否转换字符串格式

#define LFS_ATTR_MTIME 			0x01  // 自定义属性类型编号（0~255）//之前使用"mtime" 修改时间
#define LFS_ATTR_CTIME 			0x02  // 自定义属性类型编号（0~255）//之前使用"mtime"创建时间
#define LFS_DEBUG_CONFIG 		1
#define EXFLASH_TEST_START 		0
#define MAX_LOG_NAME_LEN 		32

#define TEST_TASK_LITTLEFS 		0		//测试task开启
#define LFS_INIT_IN_TASK_LATER	1		//使用task调用初始化
#define LFS_GPIO_CONFIG         0
/**
* @brief log redefine
*/
#if (LFS_DEBUG_CONFIG == 1)
#define LFS_LOG_DEBUG(format, ...)               	log_debug(format,  ## __VA_ARGS__)
/// log array
#define LFS_LOG_ARRAY(array, len)            do{int __i; for(__i=0;__i<(len);++__i)LFS_LOG_DEBUG("%02X ",((uint8_t *)(array))[__i]);}while(0)
/// log debug array with show more
#define LFS_LOG_DEBUG_ARRAY_EX(note, array, len)    do{LFS_LOG_DEBUG("%s: ",note); LFS_LOG_ARRAY(array,len); LFS_LOG_DEBUG("[%dbytes]\r\n",len);}while(0)


#else
#define LFS_LOG_DEBUG(format, ...)
#define LFS_LOG_ARRAY(level, array, len)
#define LFS_LOG_DEBUG_ARRAY_EX(note, array, len)
#endif

/**
* @brief user config
*/
#define BLOCK_SIZE        4096       // 每个块4KB
#define BLOCK_COUNT       (15*1024*1024/BLOCK_SIZE)      // 总块数 = 16MB / 4KB
#define READ_SIZE         256        // 可设为页大小或更小（依Flash特性）
#define PROG_SIZE         256        // 通常等于页大小
#define CACHE_SIZE        256        // 可设为 READ_SIZE/PROG_SIZE 倍数
#define LOOKAHEAD_SIZE    128        // 管理 128*8 = 1024 个块，可根据 BLOCK_COUNT 增大
#define BLOCK_CYCLES      500        // 擦写均衡周期，默认即可

#define LOG_ROTATE_MAX  5       // 最多保留几个历史文件
#define LOG_MAX_SIZE    (256 * 1024)    // 超过 512KB 时轮换


#define LFS_WAIT_TIMEOUT_MS   500U
#define LFS_MUTEX_TIMEOUT_MS  1000U

#define LFS_USER_TXT_DATA_MAX_SIZE (256)

#define LFS_USER_TXT_FILE_MAX_SIZE (2048)

/**
****************************************************************************
* @type		notes
* @notessn	000001
* @status	start
* @date		2025-08-05
* @brief 文件系统操作原则，读写互斥，严格遵守流程： 初始化flash->初始化文件系统->读写操作,
* 注意原子操作函数和用户抽象函数调用原子操作函数的的互斥锁不共享
* 互斥锁不可嵌套否则任务死锁
****************************************************************************
*/
//通过事件组标志位判断是否满足流程



#if (LFS_INIT_IN_TASK_LATER)

	osEventFlagsId_t sys_events;			//定义事件组标志位	

	#define EVENT_LFS_READY 	(1 << 2)	//littlefs初始化完成标志
	#define EXFLASH_INIT_READY 	(1 << 1)	//外部flash初始化完成标志

#endif


lfs_t lfs;				//文件系统标识
osMutexId_t lfs_mutex;	//文件系统的读写互斥
lfs_file_t file;
time_struct_t littlefs_date;
uint32_t get_current_unix_time(void);

/*--------------------NOTES_NUM:000001---------STATUS:END--------------------*/

/**
* @brief fun declaration
*/
int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block,lfs_off_t off, void *buffer, lfs_size_t size);
int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block,lfs_off_t off, const void *buffer, lfs_size_t size);
int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block);
int user_provided_block_device_sync(const struct lfs_config *c );
int8_t lfs_init(void);
int lfs_delete_bytes(const char *path, lfs_off_t offset, lfs_size_t length);

/**
* @brief struct && enum init
*/
// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    .read  = user_provided_block_device_read,
    .prog  = user_provided_block_device_prog,
    .erase = user_provided_block_device_erase,
    .sync  = user_provided_block_device_sync,

    // block device configuration
    .read_size = READ_SIZE,
    .prog_size = PROG_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = BLOCK_COUNT,
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
#ifdef LFS_NO_MALLOC
	.read_buffer = read_buffer,
	.prog_buffer = prog_buffer,
	.lookahead_buffer = lookahead_buffer,
#endif
	.read_buffer = NULL,
	.prog_buffer = NULL,
	.lookahead_buffer = NULL,
};

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;
typedef struct {
    char name[MAX_LOG_NAME_LEN];  // 文件名，如 "log.txt"
    uint32_t size;                // 文件大小
    uint32_t mtime;               // 修改时间戳（如果有）
} LogFileInfo;
typedef enum {
    LOG_OP_WRITE,
    LOG_OP_ROTATE
} LogOpType;
static LogOpType last_log_op = LOG_OP_WRITE;

typedef struct {
    char name[64];
    time_struct_t s_mtime;
} RecordLastLogFileInfo;
#if (LFS_GPIO_CONFIG)
/**
* @brief exflash GPIO defined
*/
/// Test cs pad for ospi1
#define PAD_OSPI1_CS               8
/// Test ck pad for ospi1
#define PAD_OSPI1_CK               9
/// Test si pad for ospi1
#define PAD_OSPI1_SI               10
/// Test so pad for ospi1
#define PAD_OSPI1_SO               7
/// Test wp pad for ospi1
#define PAD_OSPI1_WP               12
/// Test hd pad for ospi1
#define PAD_OSPI1_HD               11
/// Test oflash pin configuration
static const pin_config_t pin_config[] = {
    {PAD_OSPI1_CS, {PINMUX_PAD8_OSPI1_CS_CFG},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_CK, {PINMUX_PAD9_OSPI1_CK_CFG},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_SI, {PINMUX_PAD10_OSPI1_SI_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_SO, {PINMUX_PAD7_OSPI1_SO_CFG},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_WP, {PINMUX_PAD12_OSPI1_WP_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_HD, {PINMUX_PAD11_OSPI1_HD_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};
#endif

typedef struct {
    UserBindInfoID id;
    const char* header;
} field_header_map_t;
/* 字段头映射结构 */
typedef struct {
    SystemInfoID id;
    const char* header;
} field_system_header_map_t;
/* 字段头映射结构 */
typedef struct {
    ProducitonInfoID id;
    const char* header;
} field_test_header_map_t;
static const field_header_map_t field_header_map[] = {
    { UBI_AUTH_CODE_ID,     "[AUTH_CODE]"     },
    { UBI_WIFI_MAC_ID,      "[WIFI_MAC]"      },
    { UBI_GPS_ADDRESS_ID,   "[GPS_ADDRESS]"   },
    { UBI_MQTT_CLIENT_ID_ID,"[MQTT_CLIENT_ID]"},
};

/* 系统信息字段头映射表 */
static const field_system_header_map_t field_system_header_map[] = {
    { SYS_DEVICE_SN_ID,       	"[DEVICE_SN]"        },
    { SYS_FIRMWARE_LTE_VER_ID,  "[LTE_VER]"    		 },
    { SYS_FIRMWARE_GNSS_VER_ID, "[GNSS_VER]"    	 },
    { SYS_MANUFACTURE_DATE_ID,	"[MANUFACTURE_DATE]" },
    { SYS_BATCH_NUMBER_ID,    	"[BATCH_NUMBER]"     },
    { SYS_LAST_BOOT_TIME_ID,  	"[LAST_BOOT_TIME]"   },
    { SYS_TOTAL_RUNTIME_ID,   	"[TOTAL_RUNTIME]"    },
    { SYS_CONFIG_VERSION_ID, 	"[CONFIG_VERSION]"   }
};
/* 测试信息字段头映射表 */
static const field_test_header_map_t field_test_header_map[] = {
    { TEST_SENSOR_ID,           "[SENSOR]"           },
};
/* 获取测试字段头字符串 */
static const char* get_test_field_header(ProducitonInfoID id) {
    for (int i = 0; i < sizeof(field_test_header_map)/sizeof(field_test_header_map[0]); i++) {
        if (field_test_header_map[i].id == id) {
            return field_test_header_map[i].header;
        }
    }
    return NULL;
}


static uint8_t littlefs_create_flag = LFS_CREATE_FAILED; 
#if (LFS_BACKUP_AREA_FLAG)
static uint8_t backup_select = 0; 
#endif 

#if (LFS_CONFIG_API)
SimInfo_t sim_info={
.device_sn={0}
};
UserBindInfo_t user_bind_info;

#else

#endif //LFS_CONFIG_API
//获取当前列表数量
size_t get_field_header_map_count(void)
{
    return sizeof(field_header_map) / sizeof(field_header_map[0]);
}
//获取字符串
const char* get_bind_field_header(UserBindInfoID id)
{
    for (size_t i = 0; i < get_field_header_map_count(); i++) {
        if (field_header_map[i].id == id) {
            return field_header_map[i].header;
        }
    }
    return NULL; // 未找到
}
size_t get_field_header_length(UserBindInfoID id)
{
    for (size_t i = 0; i < get_field_header_map_count(); i++) {
        if (field_header_map[i].id == id) {
            return strlen(field_header_map[i].header);
        }
    }
    return 0; // 未找到
}
/**
 * 写文件（覆盖方式）
 * 
 * @param path       文件路径，例如 "/system/user.txt"
 * @param data       要写入的数据指针
 * @param size       字节长度
 * @param overwrite  是否覆盖（1 = 覆盖写，0 = 追加写）
 * 
 * @return >=0 写入的字节数
 *         <0  错误
 */
int lfs_write_file(const char *path, const void *data, size_t size, int overwrite)
{
    if (!is_lfs_mounted()) {
        return -1;
    }
    if (!path || (!data && size > 0)) {
        return -2;
    }

    lfs_file_t file;
    int flags = LFS_O_WRONLY | LFS_O_CREAT;

    if (overwrite) {
        flags |= LFS_O_TRUNC;  // 覆盖模式
    } else {
        flags |= LFS_O_APPEND; // 追加模式
    }

    int ret = lfs_file_open(&lfs, &file, path, flags);
    if (ret < 0) {
        return -3;
    }

    int written = 0;
    if (size > 0) {
        written = lfs_file_write(&lfs, &file, data, size);
        if (written < 0) {
            lfs_file_close(&lfs, &file);
            return -4;
        }
    }

    lfs_file_close(&lfs, &file);

    return written;  // 成功返回写入的字节数
}

/* 获取文件大小 */
size_t lfs_get_file_size(const char *filename) {
    struct lfs_info info;
    if (lfs_stat(&lfs, filename, &info) == 0) {
        return info.size;
    }
    return 0;
}

/* 检查文件是否存在 */
bool lfs_file_exists(const char *filename) {
    struct lfs_info info;
    return (lfs_stat(&lfs, filename, &info) == 0);
}

void lfs_mutex_init(void)
{
    if (lfs_mutex == NULL) {
        const osMutexAttr_t mutex_attr = { .name = "lfs_mutex" };
        lfs_mutex = osMutexNew(&mutex_attr);
        if (lfs_mutex == NULL) {
            // 错误处理，创建失败
        }
    }
}
/**
 * 删除文件
 */
int lfs_delete(const char *filename) {
    return lfs_remove(&lfs, filename);
}
const char* user_bindinfo_get(UserBindInfoID id)
{
    static const char empty_string[] = "";
#if (LFS_INIT_IN_TASK_LATER)
    // 等待外部Flash准备就绪（如GT25Q64A）
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      50);
    if (result < 0) {
        LFS_LOG_DEBUG("[LFS] wait flash ready failed (%ld)\r\n", result);
        return empty_string;
    }
#endif
    static char result_buffer[LFS_USER_TXT_DATA_MAX_SIZE];
    memset(result_buffer, 0, sizeof(result_buffer));

	
    const char* filename = "/system/user.txt";

    /* 1. 检查字段头 */
    const char* header = get_bind_field_header(id);
    if (header == NULL) {
        LFS_LOG_DEBUG("Invalid field ID: %d\r\n", id);
        return empty_string;
    }
    size_t header_len = strlen(header);

    /* 2. 检查文件是否存在 */
    if (!lfs_file_exists(filename)) {
        LFS_LOG_DEBUG("user.txt not found\r\n");
        return empty_string;
    }

    /* 3. 获取文件大小并验证 */
    size_t file_size = lfs_get_file_size(filename);
    if (file_size == 0 || file_size >= LFS_USER_TXT_FILE_MAX_SIZE) {
        LFS_LOG_DEBUG("Invalid file size: %u\r\n", file_size);
        return empty_string;
    }

    /* 4. 为文件内容分配内存 */
    char *file_content = pvPortMalloc(file_size + 1);
    if (!file_content) {
        LFS_LOG_DEBUG("Malloc failed for file buffer\r\n");
        return empty_string;
    }

    /* 5. 读取文件内容 */
    int read_len = lfs_read_file(filename, file_content, file_size, 0);
    if (read_len <= 0) {
        LFS_LOG_DEBUG("File read failed: %d\r\n", read_len);
        vPortFree(file_content);
        return empty_string;
    }
    file_content[read_len] = '\0';

    /* 6. 查找字段（增强版匹配，确保是完整字段名） */
    char *pos = file_content;
    char *field_start = NULL;

    while ((pos = strstr(pos, header)) != NULL) {
        /* 判断是否是行首字段（避免匹配到类似 [ID01]XXXX[ID011]） */
        if (pos == file_content || *(pos - 1) == '\n' || *(pos - 1) == '\r') {
            field_start = pos + header_len;   // 跳过字段头
            break;
        }
        pos += 1;  // 继续向后查找
    }

    if (!field_start) {
        LFS_LOG_DEBUG("Field not found: %s\r\n", header);
        vPortFree(file_content);
        return empty_string;
    }

    /* 7. 查找字段结束（\r\n 或文件尾） */
    char *field_end = strstr(field_start, "\r\n");
    if (!field_end) {
        field_end = file_content + strlen(file_content); // 文件最后一行
    }

    size_t value_len = field_end - field_start;
	/* 去掉结尾的换行符 */
	while (value_len > 0 &&
		   (field_start[value_len - 1] == '\n' ||
			field_start[value_len - 1] == '\r')) {
		value_len--;
	}
    /* 8. 检查字段长度 */
    if (value_len >= sizeof(result_buffer)) {
        LFS_LOG_DEBUG("Value too long (%u), truncated\r\n", value_len);
        value_len = sizeof(result_buffer) - 1;
    }

    /* 9. 复制字段内容（支持空字符串） */
    memcpy(result_buffer, field_start, value_len);
    result_buffer[value_len] = '\0';

    vPortFree(file_content);

    LFS_LOG_DEBUG("Field %s = %s\r\n", header, result_buffer);

    return result_buffer;
}


///* 获取设备用户信息 - 返回const char*，直接读取源文件 */
//const char* user_bindinfo_get(UserBindInfoID id) {

//	 static char empty_string[1] = "";  // 空字符串，包含 '\0'
//    // 使用静态缓冲区存储返回值
//    static char result_buffer[LFS_USER_TXT_DATA_MAX_SIZE];
//    memset(result_buffer, 0, sizeof(result_buffer));

//    // 文件路径
//    const char* filename = "/system/user.txt";
//    // 检查文件是否存在
//    if (!lfs_file_exists(filename)) {
//        LFS_LOG_DEBUG("User info file not exists\r\n");
//        return empty_string;
//    }
//    // 获取文件大小
//    size_t file_size = lfs_get_file_size(filename);
//    if (file_size <= 0 || file_size >= LFS_USER_TXT_FILE_MAX_SIZE) {
//        LFS_LOG_DEBUG("Invalid file size: %d\r\n", file_size);
//        return empty_string;
//    }
//    // 读取文件内容
//    char *file_content = (char*)pvPortMalloc(file_size + 1);
//    if (!file_content) {
//        LFS_LOG_DEBUG("Memory allocation failed\r\n");
//        return empty_string;
//    }

//    int read_len = lfs_read_file(filename, file_content, file_size, 1);
//    if (read_len <= 0) {
//        LFS_LOG_DEBUG("Failed to read file\r\n");
//        vPortFree(file_content);
//        return empty_string;
//    }
//    file_content[read_len] = '\0';

//    // 查找字段并提取值
//    bool found = false;
//	const char *field_start = strstr(file_content, get_bind_field_header(id));
//    if (field_start) {
//        // 跳过字段头
//        field_start += get_field_header_length(id);
//        // 查找字段结束位置（行尾）
//        char *field_end = strstr(field_start, "\r\n");
//        if (!field_end) {
//            // 如果没有找到行尾，可能是文件最后一行
//            field_end = file_content + strlen(file_content);
//        }
//        // 计算字段值长度
//        size_t value_len = field_end - field_start;
//        if (value_len > 0 && value_len < sizeof(result_buffer)) {
//            // 复制字段值到结果缓冲区
//            memcpy(result_buffer, field_start, value_len);
//            result_buffer[value_len] = '\0';
//            found = true;
//            LFS_LOG_DEBUG("Found field %s: %s\r\n", get_bind_field_header(id), result_buffer);
//        } else if (value_len >= sizeof(result_buffer)) {
//            LFS_LOG_DEBUG("Field value too long: %d, max: %d\r\n", value_len, sizeof(result_buffer)-1);
//        }
//    }
//    // 清理资源
//    vPortFree(file_content);

//    if (found) {
////       LFS_LOG_DEBUG("-==-=-=-=-=--=-=-=-=-= ");
////		for(int i =0;i<strlen(result_buffer);i++)
////		{
////			LFS_LOG_DEBUG("%x ",result_buffer[i]);
////		}LFS_LOG_DEBUG("\r\n");
//		return result_buffer;
//    } else {
//        LFS_LOG_DEBUG("Field not found: %s\r\n", get_bind_field_header(id));
//        return empty_string;
//    }
//}
/* 设置设备用户信息 - 使用.tmp文件进行缓冲操作 */
/* 设置设备用户信息 - 使用.tmp文件进行缓冲操作 */
int user_bindinfo_set(const char *data, uint16_t len, UserBindInfoID id)
{
    LFS_LOG_DEBUG("user_bindinfo_set - using tmp file buffer= %d\r\n",is_lfs_mounted());
	if(is_lfs_mounted()==false)
	{
		lfs_mount_safe();
	}
    if (!data) {
        LFS_LOG_DEBUG("LFS_USER_WRITE_ERROR: NULL data pointer\r\n");
        return -1;
    }
    if (len == 0 || len >= LFS_USER_TXT_DATA_MAX_SIZE) {
        LFS_LOG_DEBUG("LFS_USER_WRITE_ERROR: Invalid data length = %u\r\n", (unsigned)len);
        return -2;
    }
    if (id >= UBI_ALL_DATA) {
        LFS_LOG_DEBUG("LFS_USER_WRITE_ERROR: Invalid field ID\r\n");
        return -3;
    }

#if (LFS_INIT_IN_TASK_LATER)
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);

    if (result < 0) {
//		// 获取当前事件标志状态
//    uint32_t current_flags = osEventFlagsGet(sys_events);
//    
//    LFS_LOG_DEBUG("[LFS] wait ready failed: %ld\r\n", result);
//    LFS_LOG_DEBUG("[LFS] Current event flags: 0x%08lX\r\n", current_flags);
//    
//    // 检查每个标志位
//    if ((current_flags & EVENT_LFS_READY) == 0) {
//        LFS_LOG_DEBUG("[LFS] EVENT_LFS_READY NOT set\r\n");
//    } else {
//        LFS_LOG_DEBUG("[LFS] EVENT_LFS_READY IS set\r\n");
//    }
//    
//    if ((current_flags & EXFLASH_INIT_READY) == 0) {
//        LFS_LOG_DEBUG("[LFS] EXFLASH_INIT_READY NOT set\r\n");
//    } else {
//        LFS_LOG_DEBUG("[LFS] EXFLASH_INIT_READY IS set\r\n");
//    }
//    
//    // 检查错误类型
//    switch (result) {
//        case -1: // osErrorTimeout (有些RTOS实现)
//            LFS_LOG_DEBUG("[LFS] Timeout waiting for events\r\n");
//            break;
//        case -2: // 可能是osErrorParameter
//            LFS_LOG_DEBUG("[LFS] Parameter error in osEventFlagsWait\r\n");
//            break;
//        default:
//            LFS_LOG_DEBUG("[LFS] Unknown error code: %ld\r\n", result);
//            break;
//    }
//    
//    return -4;
        LFS_LOG_DEBUG("[LFS] wait ready failed: %ld\r\n", result);
        return -4;
    }
#endif

    const char *original_file = "/system/user.txt";
    const char *temp_file     = "/system/user.tmp";

    /* get header and validate */
    const char *header = get_bind_field_header(id);
    if (!header) {
        LFS_LOG_DEBUG("Invalid header for id %d\r\n", id);
        return -5;
    }
    size_t header_len = strlen(header);

    /* reject newline chars in data (single-line fields only) */
    for (uint16_t i = 0; i < len; ++i) {
        if (data[i] == '\r' || data[i] == '\n') {
            LFS_LOG_DEBUG("Field value contains CR/LF - rejected\r\n");
            return -6;
        }
    }

    /* read original file if exists */
    char *file_content = NULL;
    size_t file_size = 0;
    size_t read_len = 0;
    bool original_exists = false;

    if (lfs_file_exists(original_file)) {
        file_size = lfs_get_file_size(original_file);
        if (file_size > 0 && file_size < LFS_USER_TXT_FILE_MAX_SIZE) {
            file_content = (char*)pvPortMalloc(file_size + 1);
            if (!file_content) {
                LFS_LOG_DEBUG("Malloc failed for file_content\r\n");
                return -7;
            }
            read_len = lfs_read_file(original_file, file_content, file_size, 1);
            if (read_len > 0) {
                /* ensure NUL termination based on actual bytes read */
                file_content[read_len] = '\0';
                original_exists = true;
            } else {
                vPortFree(file_content);
                file_content = NULL;
                original_exists = false;
            }
        } else {
            LFS_LOG_DEBUG("Original file size invalid: %zu\r\n", file_size);
        }
    }

    /* build new field line: "<header><value>\r\n" (use CRLF or LF depending on your convention) */
    const char *line_end = "\r\n"; /* keep existing convention */
    size_t line_end_len = strlen(line_end);

    size_t new_line_len = header_len + len + line_end_len;
    if (new_line_len >= LFS_USER_TXT_DATA_MAX_SIZE) {
        LFS_LOG_DEBUG("New field too long: %zu\r\n", new_line_len);
        if (file_content) vPortFree(file_content);
        return -8;
    }

    char *new_line = (char*)pvPortMalloc(new_line_len + 1);
    if (!new_line) {
        if (file_content) vPortFree(file_content);
        return -9;
    }
    /* compose field */
    memcpy(new_line, header, header_len);
    memcpy(new_line + header_len, data, len);
    memcpy(new_line + header_len + len, line_end, line_end_len);
    new_line[new_line_len] = '\0';

    /* build new file content in memory */
    char *new_content = NULL;
    size_t new_content_size = 0;

    if (original_exists && file_content) {
        /* find header occurrence - but ensure it is at line start */
        char *pos = file_content;
        char *field_start = NULL;
        while (1) {
            char *p = strstr(pos, header);
            if (!p) break;
            /* ensure line start: p==file_content or previous char is '\n' or '\r' */
            if (p == file_content || (*(p - 1) == '\n') || (*(p - 1) == '\r')) {
                field_start = p;
                break;
            }
            pos = p + 1;
        }

        if (field_start) {
            /* find end of this line (first '\n' after field_start) */
            char *linebreak = strchr(field_start, '\n');
            char *after_line = NULL;
            if (linebreak) {
                /* point to char after '\n' (may be '\0' if it was last) */
                after_line = linebreak + 1;
            } else {
                /* no newline found: treat until end of file (use read_len bytes) */
                after_line = file_content + read_len;
            }

            size_t prefix_len = (size_t)(field_start - file_content);
            size_t suffix_len = (size_t)((file_content + read_len) - after_line);

            new_content_size = prefix_len + new_line_len + suffix_len;
            if (new_content_size >= LFS_USER_TXT_FILE_MAX_SIZE) {
                LFS_LOG_DEBUG("New content too large: %zu\r\n", new_content_size);
                vPortFree(file_content);
                vPortFree(new_line);
                return -10;
            }

            new_content = (char*)pvPortMalloc(new_content_size + 1);
            if (!new_content) {
                vPortFree(file_content);
                vPortFree(new_line);
                return -11;
            }

            /* copy prefix */
            if (prefix_len) memcpy(new_content, file_content, prefix_len);
            /* copy new line */
            memcpy(new_content + prefix_len, new_line, new_line_len);
            /* copy suffix */
            if (suffix_len) memcpy(new_content + prefix_len + new_line_len, after_line, suffix_len);
            new_content[new_content_size] = '\0';
        } else {
            /* header not found -> append */
            new_content_size = (size_t)read_len + new_line_len;
            if (new_content_size >= LFS_USER_TXT_FILE_MAX_SIZE) {
                LFS_LOG_DEBUG("Append would exceed max file size: %zu\r\n", new_content_size);
                vPortFree(file_content);
                vPortFree(new_line);
                return -12;
            }
            new_content = (char*)pvPortMalloc(new_content_size + 1);
            if (!new_content) {
                vPortFree(file_content);
                vPortFree(new_line);
                return -13;
            }
            memcpy(new_content, file_content, read_len);
            memcpy(new_content + read_len, new_line, new_line_len);
            new_content[new_content_size] = '\0';
        }
    }

    /* rebuild file if original doesn't exist or format was bad */
    if (!original_exists) {
        new_content_size = new_line_len;
        new_content = (char*)pvPortMalloc(new_content_size + 1);
        if (!new_content) {
            if (file_content) vPortFree(file_content);
            vPortFree(new_line);
            return -14;
        }
        memcpy(new_content, new_line, new_line_len);
        new_content[new_line_len] = '\0';
    }

    if (file_content) { vPortFree(file_content); file_content = NULL; }
    vPortFree(new_line);

    /* 4) 写入临时文件 - 使用 lfs_write_file（一次性写入） */
    if (lfs_file_exists(temp_file)) lfs_delete(temp_file);

    if (lfs_write_file(temp_file, new_content, new_content_size, 1) != (int)new_content_size) {
        LFS_LOG_DEBUG("Temp write failed (lfs_write_file)\r\n");
        lfs_delete(temp_file);
        vPortFree(new_content);
        return -15;
    }

    size_t tmp_sz = lfs_get_file_size(temp_file);
    if (tmp_sz != new_content_size) {
        LFS_LOG_DEBUG("Temp file size mismatch: %zu != %zu\r\n", tmp_sz, new_content_size);
        lfs_delete(temp_file);
        vPortFree(new_content);
        return -16;
    }

    /* 5) 备份原文件（rename original->backup），然后 rename temp->original */
    char backup_file[64];
    snprintf(backup_file, sizeof(backup_file), "%s.bak", original_file);

    if (lfs_file_exists(original_file)) {
        if (lfs_file_exists(backup_file)) lfs_delete(backup_file);
        if (lfs_rename(&lfs, original_file, backup_file) != 0) {
            LFS_LOG_DEBUG("Warning: rename original->backup failed (continue)\r\n");
            /* we continue to attempt to rename temp->original; original is still present */
        }
    }

    if (lfs_rename(&lfs, temp_file, original_file) != 0) {
        LFS_LOG_DEBUG("Rename temp->original failed, attempting restore from backup\r\n");
        /* try restore from backup if exists */
        if (lfs_file_exists(backup_file)) {
            size_t bsz = lfs_get_file_size(backup_file);
            if (bsz > 0 && bsz < LFS_USER_TXT_FILE_MAX_SIZE) {
                char *bcontent = (char*)pvPortMalloc(bsz + 1);
                if (bcontent) {
                    int br = lfs_read_file(backup_file, bcontent, bsz, 1);
                    if (br > 0) {
                        lfs_write_file(original_file, bcontent, br, 1);
                        LFS_LOG_DEBUG("Restored original from backup (%d bytes)\r\n", br);
                    }
                    vPortFree(bcontent);
                }
            }
        }
        lfs_delete(temp_file);
        vPortFree(new_content);
        return -17;
    }

    /* delete backup if exists */
    if (lfs_file_exists(backup_file)) lfs_delete(backup_file);

    vPortFree(new_content);

    LFS_LOG_DEBUG("User info updated OK: %s = %.*s\r\n", get_bind_field_header(id), (int)len, data);
    return 0;
}

#if (LFS_BACKUP_AREA_FLAG)
/*
* @brief  设置system系统信息备份
* @param  data  备份数据
* @param  size  备份数据大小
* @retval 无
*/
void Set_SystemInfo_Backup(char *data,uint32_t size)
{
	LFS_LOG_DEBUG("Set_SystemInfo_Backup = %d\r\n",((USER_BIND_INFO_ADDR - SIM_INFO_ADDR)/1024)/64);
	
	lfs_read_file("/system/system.txt", &sim_info.device_sn[0],sizeof(sim_info),1);
	
    for(int i=0;i<((USER_BIND_INFO_ADDR - SIM_INFO_ADDR)/1024)/64;i++)
    {
        drv_flash_erase(OM_FLASH1,SIM_INFO_ADDR+i*(64*1024), FLASH_ERASE_64K, 100);
    }
    drv_flash_write(OM_FLASH1,SIM_INFO_ADDR, (uint8_t *)data, size, 100);
}

/*
* @brief  获取sim信息备份
* @param  buf  备份数据缓存
* @param  size  备份数据大小
* @retval 0 成功 -1 参数错误 -2 读取失败
*/
int Get_SystemInfo_Backup(char *buf, uint32_t size)
{
	LFS_LOG_DEBUG("Get_SystemInfo_Backup = %d\r\n",((USER_BIND_INFO_ADDR - SIM_INFO_ADDR)/1024)/64);
    if (!buf || size == 0) {
        return -1;   // 参数错误
    }

    // 从 flash 固定地址读取
    int ret = drv_flash_read(OM_FLASH1, SIM_INFO_ADDR, (uint8_t *)buf, size);
	uint8_t tmp=0xFF;
    if (ret != 0) {
        return -2;   // 读取失败
    }
	for(int i=0;i<size;i++)
	{
		tmp &= buf[i];
	}
	if(tmp==0xFF)
	{
		return -3;
	}

    return 0; // 成功
}
#endif


/*
* * @brief  判断是否有用户信息
* @param  无
* @retval 0 无用户信息 1 有用户信息
*/
int user_bindinfo_check(void)
{
	log_debug("user_bindinfo_check\r\n");
	 if(strlen(user_bindinfo_get(UBI_AUTH_CODE_ID))<=0)//CLIENT已弃用
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
///* 获取设备 用户信息 */
//const char* user_bindinfo_get(UserBindInfoID id) {
////	 LFS_LOG_DEBUG("eeee user_bindinfo_get\r\n");
//#if (LFS_CONFIG_API)
//	
//#if (LFS_INIT_IN_TASK_LATER)
//    // 等待文件系统或Flash准备就绪
//    int32_t result = osEventFlagsWait(sys_events,
//                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
//                                      osFlagsWaitAll | osFlagsNoClear,
//                                      LFS_WAIT_TIMEOUT_MS);
//    if (result < 0) {
//        if (result == osFlagsErrorTimeout) {
//            LFS_LOG_DEBUG("[LFS] wait timeout for ready flags\r\n");
//        } else {
//            LFS_LOG_DEBUG("[LFS] event wait error: %ld\r\n", result);
//        }
//        return 0;
//    }
//#endif
//	memset( &user_bind_info,0,sizeof(user_bind_info));
//	lfs_read_file("/system/user.txt", &user_bind_info.auth_code[0],sizeof(user_bind_info),1);
//    switch(id)
//    {
//        case UBI_AUTH_CODE_ID:
//			if(strlen(user_bind_info.auth_code)==0)
//			{
//				memset( &user_bind_info.auth_code[0],0,sizeof(user_bind_info.auth_code));
//				user_bind_info.auth_code[0]='\0';
//			}
//            return user_bind_info.auth_code;
//        case UBI_WIFI_MAC_ID:
//			if(strlen(user_bind_info.wifi_mac)==0)
//			{
//				memset( &user_bind_info.wifi_mac[0],0,sizeof(user_bind_info.wifi_mac));
//				user_bind_info.wifi_mac[0]='\0';
//			}
//            return user_bind_info.wifi_mac;
//        case UBI_GPS_ADDRESS_ID:
//			if(strlen(user_bind_info.gps_address)==0)
//			{
//				memset( &user_bind_info.gps_address[0],0,sizeof(user_bind_info.gps_address));
//				user_bind_info.gps_address[0]='\0';
//			}
//            return user_bind_info.gps_address;
//        case UBI_MQTT_CLIENT_ID_ID:
//			if(strlen(user_bind_info.mqtt_client_id)==0)
//			{
//				memset( &user_bind_info.mqtt_client_id[0],0,sizeof(user_bind_info.mqtt_client_id));
//				user_bind_info.mqtt_client_id[0]='\0';
//			}
//            return user_bind_info.mqtt_client_id;   
//        default:
//            break;
//    }
//#else

//#endif
//	return 0;
//}

/* 删除 用户信息 */
//void user_bindinfo_del(UserBindInfoID id) {
//	 LFS_LOG_DEBUG("eeee user_bindinfo_del\r\n");
//#if (LFS_CONFIG_API)
//	
//#if (LFS_INIT_IN_TASK_LATER)
//    // 等待文件系统或Flash准备就绪
//    int32_t result = osEventFlagsWait(sys_events,
//                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
//                                      osFlagsWaitAll | osFlagsNoClear,
//                                      LFS_WAIT_TIMEOUT_MS);
//    if (result < 0) {
//        if (result == osFlagsErrorTimeout) {
//            LFS_LOG_DEBUG("[LFS] wait timeout for ready flags\r\n");
//        } else {
//            LFS_LOG_DEBUG("[LFS] event wait error: %ld\r\n", result);
//        }
//        return ;
//    }
//#endif
//	memset( &user_bind_info,0,sizeof(user_bind_info));
//    lfs_read_file("/system/user.txt", &user_bind_info.auth_code[0],sizeof(user_bind_info),1);

//    switch(id)
//    {
//        case UBI_AUTH_CODE_ID:
//			memset( &user_bind_info.auth_code[0],0,sizeof(user_bind_info.auth_code));
//			user_bind_info.auth_code[0]='\0';
//            break;
//        case UBI_WIFI_MAC_ID:
//			memset( &user_bind_info.wifi_mac[0],0,sizeof(user_bind_info.wifi_mac));
//			user_bind_info.wifi_mac[0]='\0';
//            break;
//        case UBI_GPS_ADDRESS_ID:
//			memset( &user_bind_info.gps_address[0],0,sizeof(user_bind_info.gps_address));
//			user_bind_info.gps_address[0]='\0';
//            break;
//        case UBI_MQTT_CLIENT_ID_ID:
//			memset( &user_bind_info.mqtt_client_id[0],0,sizeof(user_bind_info.mqtt_client_id));
//			user_bind_info.mqtt_client_id[0]='\0';
//            break;  
//		case UBI_ALL_DATA:
//			memset( &user_bind_info,0,sizeof(user_bind_info));
//			break;
//        default:
//            break;
//    }
//    lfs_delete_bytes("/system/user.txt",0,sizeof(user_bind_info));//删除字段

//    log_write_with_rotation("/system/user.txt",&user_bind_info.auth_code[0],sizeof(user_bind_info));//写操作
//#else

//#endif
//}

/**
 * 读取文件（完整读取）
 *
 * @param path    文件路径，如 "/system/user.txt"
 * @param buf     用于存放读取结果的缓冲区
 * @param size    buf 的大小
 * @param trim    是否去掉末尾多余的 '\0'（1 = 是，0 = 否）
 *
 * @return >0：实际读取的字节数
 *         0：文件为空
 *        <0：错误
 */
int lfs_read_file_user(const char *path, void *buf, size_t size, int trim)
{
    if (!is_lfs_mounted()) {
        return -1;
    }
    if (!path || !buf || size == 0) {
        return -2;
    }

    if (!lfs_file_exists(path)) {
        return -3; // 文件不存在
    }

    size_t file_size = lfs_get_file_size(path);
    if (file_size == 0) {
        return 0;
    }

    if (file_size > size) {
        // 缓冲区不够大
        return -4;
    }

    lfs_file_t file;
    int ret = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (ret < 0) {
        return -5;
    }

    int read_len = lfs_file_read(&lfs, &file, buf, file_size);
    lfs_file_close(&lfs, &file);

    if (read_len != (int)file_size) {
        return -6; // 读取不完整
    }

    // 可选：末尾补 '\0'
    if (trim && read_len < size) {
        ((char*)buf)[read_len] = '\0';
    }

    return read_len;
}
static int replace_file_content_safely(const char *original_file,
                                       const char *temp_file,
                                       const char *new_content,
                                       size_t content_len)
{
    LFS_LOG_DEBUG("replace_file_content_safely: Starting\r\n");

    if (!original_file || !temp_file || !new_content) {
        return -1;
    }

    // 1. 写临时文件（overwrite = 1）
    lfs_delete(temp_file);
    int w = lfs_write_file(temp_file, new_content, content_len, 1);
    if (w < 0 || (size_t)w != content_len) {
        LFS_LOG_DEBUG("Write temp failed\r\n");
        lfs_delete(temp_file);
        return -2;
    }

    // 2. 校验临时文件
    size_t temp_size = lfs_get_file_size(temp_file);
    if (temp_size != content_len) {
        LFS_LOG_DEBUG("Temp size mismatch\r\n");
        lfs_delete(temp_file);
        return -3;
    }

    // 3. 原文件不存在 → 直接替换成功
    if (!lfs_file_exists(original_file)) {
        if (lfs_rename(&lfs, temp_file, original_file) == 0) {
            return 0;
        }
        lfs_delete(temp_file);
        return -4;
    }

    // 4. 构建备份路径
    char backup_file[64];
    snprintf(backup_file, sizeof(backup_file), "%s.bak", original_file);

    if (lfs_file_exists(backup_file)) {
        lfs_delete(backup_file);
    }

    // 5. 备份原文件
    size_t orig_size = lfs_get_file_size(original_file);
    if (orig_size > 0 && orig_size < 65536) {
        char *orig_buf = pvPortMalloc(orig_size + 1);
        if (orig_buf) {
            int r = lfs_read_file_user(original_file, orig_buf, orig_size, 1);
            if (r > 0) {
                lfs_write_file(backup_file, orig_buf, r, 1);
            }
            vPortFree(orig_buf);
        }
    }

    // 6. rename 替换（原子的）
    int ret = lfs_rename(&lfs, temp_file, original_file);
    if (ret != 0) {

        LFS_LOG_DEBUG("Rename failed, restoring...\r\n");

        // 恢复备份
        if (lfs_file_exists(backup_file)) {
            size_t bs = lfs_get_file_size(backup_file);
            char *backup_buf = pvPortMalloc(bs + 1);
            if (backup_buf) {
                int br = lfs_read_file_user(backup_file, backup_buf, bs, 1);
                if (br > 0) {
                    lfs_write_file(original_file, backup_buf, br, 1);
                }
                vPortFree(backup_buf);
            }
        }

        lfs_delete(temp_file);
        return -5;
    }

    // 7. 删除备份
    if (lfs_file_exists(backup_file)) {
        lfs_delete(backup_file);
    }

    return 0;
}



int user_bindinfo_delete_all(void) {
    const char* filename = "/system/user.txt";
    
    // 未挂载的情况
    if (!is_lfs_mounted()) {
        LFS_LOG_DEBUG("delete_all: filesystem not mounted\r\n");
        return -1;
    }

    // 文件不存在当成功处理
    if (!lfs_file_exists(filename)) {
        LFS_LOG_DEBUG("delete_all: file not exist\r\n");
        return 0;
    }

    size_t size = lfs_get_file_size(filename);
    LFS_LOG_DEBUG("delete_all: deleting (size=%d)\r\n", size);

    // 尝试删除
    int ret = lfs_delete(filename);
    if (ret != 0) {
        LFS_LOG_DEBUG("delete_all: delete failed=%d, try rewrite empty\r\n", ret);

        // 尝试强制写一个空内容覆盖
        const char empty_str[] = "";
        const char* temp_file = "/system/user.tmp";

        int fix = replace_file_content_safely(filename, temp_file, empty_str, 0);
        if (fix == 0) {
            LFS_LOG_DEBUG("delete_all: recovered by writing empty file\r\n");
            return 0;
        }

        LFS_LOG_DEBUG("delete_all: recovery failed fix=%d\r\n", fix);
        return ret;
    }

    // 清理备份/临时文件
    const char* backup_files[] = {
        "/system/user.bak",
        "/system/user.tmp",
        "/system/user_backup.txt",
        "/system/user_temp.txt"
    };

    for (int i = 0; i < sizeof(backup_files)/sizeof(backup_files[0]); i++) {
        if (lfs_file_exists(backup_files[i])) {
            int del = lfs_delete(backup_files[i]);
            if (del == 0)
                LFS_LOG_DEBUG("delete_all: deleted backup=%s\r\n", backup_files[i]);
        }
    }

    LFS_LOG_DEBUG("delete_all: success\r\n");
    return 0;
}

// 自定义空白字符检查函数 
static int is_whitespace_char(char c) 
{
 return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'); 
}

int user_bindinfo_delete_field_by_id(UserBindInfoID id)
{
    const char* filename = "/system/user.txt";
    const char* temp_file = "/system/user.tmp";

    if (id >= UBI_ALL_DATA)
        return -1;

    if (!is_lfs_mounted())
        return -2;

    if (id == UBI_ALL_DATA)
        return user_bindinfo_delete_all();

    if (!lfs_file_exists(filename))
        return 0;

    // 读取整个文件
    size_t file_size = lfs_get_file_size(filename);
    if (file_size == 0)
        return 0;

    char* file_content = pvPortMalloc(file_size + 1);
    if (!file_content)
        return -3;

    int read_len = lfs_read_file(filename, file_content, file_size, 1);
    file_content[read_len] = '\0';

    // 字段头
    const char* header = get_bind_field_header(id);

    // 逐行处理
    char* new_buf = pvPortMalloc(file_size + 1);
    if (!new_buf) {
        vPortFree(file_content);
        return -4;
    }
    new_buf[0] = '\0';

    char* line = strtok(file_content, "\r\n");
    bool found = false;

    while (line) {
        if (strncmp(line, header, strlen(header)) == 0) {
            found = true;  // 这一行就是要删除的
        } else {
            strcat(new_buf, line);
            strcat(new_buf, "\n");
        }
        line = strtok(NULL, "\r\n");
    }

    // 如果没找到要删的字段
    if (!found) {
        vPortFree(file_content);
        vPortFree(new_buf);
        return 0;
    }

    // 判断是否为空内容
    bool all_blank = true;
    for (int i = 0; new_buf[i]; i++) {
        if (!is_whitespace_char(new_buf[i])) {
            all_blank = false;
            break;
        }
    }

    // 写入空文件
    if (all_blank) {
        const char empty_str[] = "";
        int ret = replace_file_content_safely(filename, temp_file, empty_str, 0);
        vPortFree(file_content);
        vPortFree(new_buf);
        return ret;
    }

    // 写入新内容
    int ret = replace_file_content_safely(filename, temp_file, new_buf, strlen(new_buf));

    vPortFree(file_content);
    vPortFree(new_buf);

    return ret;
}


void test_auth_code_set(char *auth_code,uint16_t len)
{
	lfs_list_dir("/");
	const char *sn_ptr=NULL;
	LFS_LOG_DEBUG("test_sn_set = %s %d\r\n",auth_code,len);
	user_bindinfo_set(auth_code,len,UBI_AUTH_CODE_ID);
	sn_ptr = user_bindinfo_get(UBI_AUTH_CODE_ID);	
	LFS_LOG_DEBUG("user_bindinfo_get = %s\r\n",sn_ptr);
	lfs_list_dir("/");
}



/* 获取字段头字符串 */
static const char* get_system_field_header(SystemInfoID id) {
    for (int i = 0; i < sizeof(field_system_header_map)/sizeof(field_system_header_map[0]); i++) {
        if (field_system_header_map[i].id == id) {
            return field_system_header_map[i].header;
        }
    }
    return NULL;
}
///* 设置系统信息 - 使用.tmp文件进行缓冲操作 */
//int system_info_set(const char *data, uint16_t len, SystemInfoID id) {
//    LFS_LOG_DEBUG("system_info_set - field: %d\r\n", id);
//    
//    // 检查参数是否有效
//    if (!data) {
//        LFS_LOG_DEBUG("LFS_SYSTEM_WRITE_ERROR: NULL data pointer\r\n");
//        return -1;
//    }

//    // 验证数据长度
//    if (len == 0 || len > LFS_USER_TXT_DATA_MAX_SIZE-1) {
//        LFS_LOG_DEBUG("LFS_SYSTEM_WRITE_ERROR: Data too long: %d\r\n", len);
//        return -1;
//    }

//#if (LFS_INIT_IN_TASK_LATER)
//    // 等待文件系统准备就绪
//    int32_t result = osEventFlagsWait(sys_events,
//                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
//                                      osFlagsWaitAll | osFlagsNoClear,
//                                      LFS_WAIT_TIMEOUT_MS);
//    if (result < 0) {
//        LFS_LOG_DEBUG("[LFS] wait for ready flags failed: %ld\r\n", result);
//        return -1;
//    }
//#endif

//    // 获取字段头
//    const char* field_header = get_system_field_header(id);
//    if (!field_header) {
//        LFS_LOG_DEBUG("LFS_SYSTEM_WRITE_ERROR: Invalid field ID: %d\r\n", id);
//        return -1;
//    }

//    // 文件路径定义
//    const char* original_file = "/system/system.txt";
//    const char* temp_file = "/system/system.tmp";

//    // 步骤1: 读取原文件内容（如果存在）
//    char *file_content = NULL;
//    size_t file_size = 0;
//    bool original_exists = false;
//    
//    if (lfs_file_exists(original_file)) {
//        file_size = lfs_get_file_size(original_file);
//        if (file_size > 0 && file_size < LFS_USER_TXT_FILE_MAX_SIZE) {
//            file_content = (char*)pvPortMalloc(file_size + 1);
//            if (file_content) {
//                int read_len = lfs_read_file(original_file, file_content, file_size, 1);
//                if (read_len > 0) {
//                    file_content[read_len] = '\0';
//                    original_exists = true;
//                    LFS_LOG_DEBUG("Original system file read, size: %d\r\n", read_len);
//                } else {
//                    vPortFree(file_content);
//                    file_content = NULL;
//                }
//            }
//        }
//    }

//    // 步骤2: 构建新字段行
//    char new_field_line[LFS_USER_TXT_DATA_MAX_SIZE] = {0};
//    if (len == 0) {
//        // 如果长度为0，表示删除该字段（写入空值）
//        snprintf(new_field_line, sizeof(new_field_line), "%s%s", field_header, "\r\n");
//    } else {
//        snprintf(new_field_line, sizeof(new_field_line), "%s%.*s\r\n", field_header, len, data);
//    }
//    
//    if (strlen(new_field_line) >= sizeof(new_field_line)) {
//        LFS_LOG_DEBUG("LFS_SYSTEM_WRITE_ERROR: Field line too long\r\n");
//        if (file_content) vPortFree(file_content);
//        return -1;
//    }

//    // 步骤3: 在内存中处理内容更新
//    char *new_content = NULL;
//    size_t new_content_size = 0;
//    bool field_updated = false;
//    
//    if (original_exists && file_content) {
//        // 查找现有字段
//        char *field_start = strstr(file_content, field_header);
//        
//        if (field_start) {
//            // 找到现有字段，定位字段结束位置
//            char *line_end = strstr(field_start, "\r\n");
//            if (line_end) {
//                line_end += strlen("\r\n"); // 指向下一行开始
//                
//                // 计算新内容大小：字段前部分 + 新字段行 + 字段后部分
//                size_t prefix_len = field_start - file_content;
//                size_t suffix_len = strlen(line_end);
//                new_content_size = prefix_len + strlen(new_field_line) + suffix_len;
//                
//                new_content = (char*)pvPortMalloc(new_content_size + 1);
//                if (new_content) {
//                    // 构建新内容
//                    memcpy(new_content, file_content, prefix_len);
//                    strcpy(new_content + prefix_len, new_field_line);
//                    strcpy(new_content + prefix_len + strlen(new_field_line), line_end);
//                    field_updated = true;
//                    LFS_LOG_DEBUG("Field updated in memory\r\n");
//                }
//            } else {
//                // 格式错误，没有找到行结束符，重建文件
//                LFS_LOG_DEBUG("Format error, rebuilding system file\r\n");
//                original_exists = false;
//            }
//        }
//    }
//    
//    if (!original_exists || !field_updated) {
//        // 字段不存在或需要重建
//        if (original_exists && file_content) {
//            // 追加到文件末尾
//            new_content_size = file_size + strlen(new_field_line);
//            new_content = (char*)pvPortMalloc(new_content_size + 1);
//            if (new_content) {
//                strcpy(new_content, file_content);
//                strcat(new_content, new_field_line);
//                LFS_LOG_DEBUG("Field appended to system file\r\n");
//            }
//        } else {
//            // 文件不存在，创建新内容
//            new_content_size = strlen(new_field_line);
//            new_content = (char*)pvPortMalloc(new_content_size + 1);
//            if (new_content) {
//                strcpy(new_content, new_field_line);
//                LFS_LOG_DEBUG("New system file content created\r\n");
//            }
//        }
//    }
//    
//    if (!new_content) {
//        LFS_LOG_DEBUG("LFS_SYSTEM_WRITE_ERROR: Memory allocation failed\r\n");
//        if (file_content) vPortFree(file_content);
//        return -1;
//    }
//    
//    // 步骤4: 写入临时文件
//    bool temp_write_success = false;
//    
//    // 删除可能已存在的临时文件
//    lfs_delete(temp_file);
//    
//    // 写入临时文件
//    if (log_write_with_rotation(temp_file, new_content, strlen(new_content)) > 0) {
//        LFS_LOG_DEBUG("System temp file written successfully\r\n");
//        
//        // 验证临时文件内容
//        size_t temp_size = lfs_get_file_size(temp_file);
//        if (temp_size == strlen(new_content)) {
//            temp_write_success = true;
//        } else {
//            LFS_LOG_DEBUG("System temp file verification failed\r\n");
//        }
//    }
//    
//    // 步骤5: 如果临时文件写入成功，替换原文件
//    if (temp_write_success) {
//        // 删除原文件
//        lfs_delete(original_file);
//        
//        // 重命名临时文件为原文件
//        if (lfs_rename(&lfs,temp_file, original_file) == 0) {
//            LFS_LOG_DEBUG("System info updated successfully: %s\r\n", field_header);
//            if (len > 0) {
//                LFS_LOG_DEBUG("Data: %.*s\r\n", len > 32 ? 32 : len, data);
//            }
//        } else {
//            LFS_LOG_DEBUG("LFS_SYSTEM_WRITE_ERROR: Failed to rename temp file\r\n");
//            // 重命名失败，尝试恢复
//            lfs_delete(temp_file);
//            vPortFree(new_content);
//            if (file_content) vPortFree(file_content);
//            return -1;
//        }
//    } else {
//        LFS_LOG_DEBUG("LFS_SYSTEM_WRITE_ERROR: Temp file write failed\r\n");
//        // 清理临时文件
//        lfs_delete(temp_file);
//        vPortFree(new_content);
//        if (file_content) vPortFree(file_content);
//        return -1;
//    }
//    
//    // 步骤6: 清理内存
//    vPortFree(new_content);
//    if (file_content) {
//        vPortFree(file_content);
//    }
//    
//    return 0;
//   
//}


int system_info_set(const char *data, uint16_t len, SystemInfoID id)
{
    LFS_LOG_DEBUG("system_info_set - field: %d\r\n", id);

    /* 参数检查 */
    if (!data) return -1;
    if (len >= LFS_USER_TXT_DATA_MAX_SIZE - 8) return -2;

#if (LFS_INIT_IN_TASK_LATER)
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);
    if (result < 0) return -3;
#endif

    /* 字段头 */
    const char *field_header = get_system_field_header(id);
    if (!field_header) return -4;

    const char *original_file = "/system/system.txt";
    const char *temp_file     = "/system/system.tmp";

    char *file_content = NULL;
    size_t file_size = 0;

    /*----------------------------
     * 1. 读取原文件
     *----------------------------*/
    if (lfs_file_exists(original_file)) {
        file_size = lfs_get_file_size(original_file);
        if (file_size > 0 && file_size < LFS_USER_TXT_FILE_MAX_SIZE) {
            file_content = (char*)pvPortMalloc(file_size + 2);
            if (!file_content) return -5;

            int read_len = lfs_read_file(original_file, file_content, file_size, 1);
            if (read_len <= 0) {
                vPortFree(file_content);
                file_content = NULL;
                file_size = 0;
            } else {
                file_content[read_len] = '\0';
                file_size = read_len;
            }
        }
    }

    /*----------------------------
     * 2. 构建新字段行
     *----------------------------*/
    char new_field_line[LFS_USER_TXT_DATA_MAX_SIZE];
    size_t new_line_len;

    if (len == 0) {
        /* 空值 => 清空字段 */
        new_line_len = snprintf(new_field_line, sizeof(new_field_line),
                                "%s\r\n", field_header);
    } else {
        new_line_len = snprintf(new_field_line, sizeof(new_field_line),
                                "%s%.*s\r\n", field_header, len, data);
    }

    if (new_line_len == 0 || new_line_len >= sizeof(new_field_line)) {
        if (file_content) vPortFree(file_content);
        return -6;
    }

    /*----------------------------
     * 3. 创建 new_content，并在其中更新字段
     *----------------------------*/
    char *new_content = NULL;
    size_t new_size = 0;
    bool updated = false;

    /* 确保文件内容以换行符结束，否则追加时会破坏格式 */
    if (file_content && file_size > 0) {
        char last = file_content[file_size - 1];
        if (last != '\n' && last != '\r') {
            /* 补一个 \n */
            file_content[file_size] = '\n';
            file_content[file_size + 1] = '\0';
            file_size += 1;
        }
    }

    /*-------- 遍历每一行，寻找匹配字段 --------*/
    if (file_content && file_size > 0) {
        /* 临时大缓冲区 */
        new_content = (char*)pvPortMalloc(file_size + new_line_len + 16);
        if (!new_content) {
            vPortFree(file_content);
            return -7;
        }
        new_content[0] = '\0';

        char *p = file_content;
        while (*p) {
            char *line_end =
                strpbrk(p, "\r\n"); /* 寻找行结束符（支持 CR/LF 任意格式） */

            size_t line_len;
            if (line_end)
                line_len = line_end - p;
            else
                line_len = strlen(p);

            /* 提取行开头字段匹配 */
            if (!updated &&
                strncmp(p, field_header, strlen(field_header)) == 0) {
                /* 匹配此字段 => 写入新值 */
                strncat(new_content, new_field_line, new_line_len);
                updated = true;
            } else {
                /* 保留旧行 */
                strncat(new_content, p, line_len);
                strcat(new_content, "\n");
            }

            if (!line_end) break;

            /* 跳过 \r\n、\n 或 \r */
            if (*line_end == '\r' && *(line_end + 1) == '\n')
                p = line_end + 2;
            else
                p = line_end + 1;
        }

        /* 字段不存在 => 追加 */
        if (!updated) {
            strcat(new_content, new_field_line);
        }

        new_size = strlen(new_content);
    } else {
        /* 文件不存在 => 直接创建 */
        new_content = (char*)pvPortMalloc(new_line_len + 1);
        if (!new_content) return -8;

        memcpy(new_content, new_field_line, new_line_len + 1);
        new_size = new_line_len;
    }

    if (file_content) vPortFree(file_content);

    /*----------------------------
     * 4. 写入临时文件
     *----------------------------*/
    lfs_delete(temp_file);

    uint32_t w = log_write_with_rotation(temp_file, new_content, new_size);
    if (w != new_size) {
        lfs_delete(temp_file);
        vPortFree(new_content);
        return -9;
    }

    /*----------------------------
     * 5. 用临时文件替换原文件
     *----------------------------*/
    lfs_delete(original_file);

    if (lfs_rename(&lfs, temp_file, original_file) != 0) {
        lfs_delete(temp_file);
        vPortFree(new_content);
        return -10;
    }

    vPortFree(new_content);
    return 0;
}



/* 获取系统信息 - 返回const char* */
//const char* system_info_get(SystemInfoID id) {
//	 static char empty_string[1] = "";  // 空字符串，包含 '\0'
//    // 获取字段头
//    const char* field_header = get_system_field_header(id);
//    if (!field_header) {
//        LFS_LOG_DEBUG("LFS_SYSTEM_READ_ERROR: Invalid field ID: %d\r\n", id);
//        return empty_string;
//    }

//    // 使用静态缓冲区存储返回值
//    static char result_buffer[256];
//    memset(result_buffer, 0, sizeof(result_buffer));

//    // 文件路径
//    const char* filename = "/system/system.txt";
//    
//    // 检查文件是否存在
//    if (!lfs_file_exists(filename)) {
//        LFS_LOG_DEBUG("System info file not exists\r\n");
//        return empty_string;
//    }

//    // 获取文件大小
//    size_t file_size = lfs_get_file_size(filename);
//    if (file_size <= 0 || file_size >= 2048) {
//        LFS_LOG_DEBUG("Invalid system file size: %d\r\n", file_size);
//        return empty_string;
//    }

//    // 读取文件内容
//    char *file_content = (char*)pvPortMalloc(file_size + 1);
//    if (!file_content) {
//        LFS_LOG_DEBUG("Memory allocation failed for system file\r\n");
//        return empty_string;
//    }
//    
//    int read_len = lfs_read_file(filename, file_content, file_size, 1);
//    if (read_len <= 0) {
//        LFS_LOG_DEBUG("Failed to read system file\r\n");
//        vPortFree(file_content);
//        return empty_string;
//    }
//    file_content[read_len] = '\0';

//    // 查找字段并提取值
//    bool found = false;
//    char *field_start = strstr(file_content, field_header);
//    if (field_start) {
//        // 跳过字段头
//        field_start += strlen(field_header);
//        
//        // 查找字段结束位置（行尾）
//        char *field_end = strstr(field_start, "\r\n");
//        if (!field_end) {
//            // 如果没有找到行尾，可能是文件最后一行
//            field_end = file_content + strlen(file_content);
//        }

//        // 计算字段值长度
//        size_t value_len = field_end - field_start;
//        if (value_len > 0 && value_len < sizeof(result_buffer)) {
//            // 复制字段值到结果缓冲区
//            memcpy(result_buffer, field_start, value_len);
//            result_buffer[value_len] = '\0';
//            found = true;
//         //   LFS_LOG_DEBUG("Found system field %s: %s\r\n", field_header, result_buffer);
//        }
//    }

//    // 清理资源
//    vPortFree(file_content);

//    if (found) {
//        return result_buffer;
//    } else {
//        LFS_LOG_DEBUG("System field not found: %s\r\n", field_header);
//        return empty_string;
//    }
//}
const char* system_info_get(SystemInfoID id)
{
    static char empty_string[1] = "";
    static char result_buffer[256];
#if (LFS_INIT_IN_TASK_LATER)
    // 等待外部Flash准备就绪（如GT25Q64A）
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      50);
    if (result < 0) {
        LFS_LOG_DEBUG("[LFS] wait flash ready failed (%ld)\r\n", result);
        return empty_string;
    }
#endif
    const char* header = get_system_field_header(id);
    if (!header) return empty_string;

    memset(result_buffer, 0, sizeof(result_buffer));

    const char* filename = "/system/system.txt";

    if (!lfs_file_exists(filename)) return empty_string;

    size_t file_size = lfs_get_file_size(filename);
    if (file_size == 0 || file_size > 2048) return empty_string;

    char *file_content = (char*)pvPortMalloc(file_size + 1);
    if (!file_content) return empty_string;

    int read_len = lfs_read_file(filename, file_content, file_size, 1);
    if (read_len <= 0) {
        vPortFree(file_content);
        return empty_string;
    }
    file_content[read_len] = '\0';

    /* --- 严格查找字段（必须在行首） --- */
    char *p = file_content;
    char *found = NULL;

    while (1) {
        char *f = strstr(p, header);
        if (!f) break;

        /* 行首判断 */
        if (f == file_content ||
            *(f - 1) == '\n' ||
            *(f - 1) == '\r')
        {
            found = f;
            break;
        }

        p = f + 1;  // 继续查找下一处
    }

    if (!found) {
        vPortFree(file_content);
        return empty_string;
    }

    /* 跳过 header */
    char *value_start = found + strlen(header);

    /* 查找行尾: 支持 CRLF / LF / CR / 文件结尾 */
    char *value_end = NULL;

    for (char *q = value_start; q < file_content + read_len; q++) {
        if (*q == '\n' || *q == '\r') {
            value_end = q;
            break;
        }
    }
    if (!value_end) value_end = file_content + read_len;

    size_t value_len = value_end - value_start;
		/* 去掉结尾的换行符 */
	while (value_len > 0 &&
		   (value_start[value_len - 1] == '\n' ||
			value_start[value_len - 1] == '\r')) {
		value_len--;
	}
    if (value_len >= sizeof(result_buffer))
        value_len = sizeof(result_buffer) - 1;

    memcpy(result_buffer, value_start, value_len);
    result_buffer[value_len] = '\0';

    vPortFree(file_content);
    return result_buffer;
}

/* 获取系统信息到指定缓冲区 */
int system_info_get_buffer(SystemInfoID id, char *buffer, uint16_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    const char* result = system_info_get(id);
    if (!result) {
        return -1;
    }
    
    size_t len = strlen(result);
    if (len >= buffer_size) {
        len = buffer_size - 1;
    }
    
    memcpy(buffer, result, len);
    buffer[len] = '\0';
    
    return len;
}
/* 设置测试信息 - 使用.tmp文件进行缓冲操作 */
int test_info_set(const char *data, uint16_t len, ProducitonInfoID id) {
    LFS_LOG_DEBUG("test_info_set - field: %d\r\n", id);
    // 检查参数是否有效
    if (!data) {
        LFS_LOG_DEBUG("LFS_TEST_WRITE_ERROR: NULL data pointer\r\n");
        return -1;
    }

    // 验证数据长度
    if (len > 255) {
        LFS_LOG_DEBUG("LFS_TEST_WRITE_ERROR: Data too long: %d\r\n", len);
        return -1;
    }

#if (LFS_INIT_IN_TASK_LATER)
    // 等待文件系统准备就绪
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);
    if (result < 0) {
        LFS_LOG_DEBUG("[LFS] wait for ready flags failed: %ld\r\n", result);
        return -1;
    }
#endif

    // 获取字段头
    const char* field_header = get_test_field_header(id);
    if (!field_header) {
        LFS_LOG_DEBUG("LFS_TEST_WRITE_ERROR: Invalid field ID: %d\r\n", id);
        return -1;
    }

    // 文件路径定义
    const char* original_file = "/production/test.txt";
    const char* temp_file = "/production/test.tmp";

    // 步骤1: 读取原文件内容（如果存在）
    char *file_content = NULL;
    size_t file_size = 0;
    bool original_exists = false;
    
    if (lfs_file_exists(original_file)) {
        file_size = lfs_get_file_size(original_file);
        if (file_size > 0 && file_size < 4096) { // 测试文件可能更大
            file_content = (char*)pvPortMalloc(file_size + 1);
            if (file_content) {
                int read_len = lfs_read_file(original_file, file_content, file_size, 1);
                if (read_len > 0) {
                    file_content[read_len] = '\0';
                    original_exists = true;
                    LFS_LOG_DEBUG("Original test file read, size: %d\r\n", read_len);
                } else {
                    vPortFree(file_content);
                    file_content = NULL;
                }
            }
        }
    }

    // 步骤2: 构建新字段行
    char new_field_line[256] = {0};
    if (len == 0) {
        // 如果长度为0，表示删除该字段（写入空值）
        snprintf(new_field_line, sizeof(new_field_line), "%s%s", field_header, "\r\n");
    } else {
        snprintf(new_field_line, sizeof(new_field_line), "%s%.*s\r\n", field_header, len, data);
    }
    
    if (strlen(new_field_line) >= sizeof(new_field_line)) {
        LFS_LOG_DEBUG("LFS_TEST_WRITE_ERROR: Field line too long\r\n");
        if (file_content) vPortFree(file_content);
        return -1;
    }

    // 步骤3: 在内存中处理内容更新
    char *new_content = NULL;
    size_t new_content_size = 0;
    bool field_updated = false;
    
    if (original_exists && file_content) {
        // 查找现有字段
        char *field_start = strstr(file_content, field_header);
        
        if (field_start) {
            // 找到现有字段，定位字段结束位置
            char *line_end = strstr(field_start, "\r\n");
            if (line_end) {
                line_end += strlen("\r\n"); // 指向下一行开始
                
                // 计算新内容大小：字段前部分 + 新字段行 + 字段后部分
                size_t prefix_len = field_start - file_content;
                size_t suffix_len = strlen(line_end);
                new_content_size = prefix_len + strlen(new_field_line) + suffix_len;
                
                new_content = (char*)pvPortMalloc(new_content_size + 1);
                if (new_content) {
                    // 构建新内容
                    memcpy(new_content, file_content, prefix_len);
                    strcpy(new_content + prefix_len, new_field_line);
                    strcpy(new_content + prefix_len + strlen(new_field_line), line_end);
                    field_updated = true;
                    LFS_LOG_DEBUG("Test field updated in memory\r\n");
                }
            } else {
                // 格式错误，没有找到行结束符，重建文件
                LFS_LOG_DEBUG("Test file format error, rebuilding\r\n");
                original_exists = false;
            }
        }
    }
    
    if (!original_exists || !field_updated) {
        // 字段不存在或需要重建
        if (original_exists && file_content) {
            // 追加到文件末尾
            new_content_size = file_size + strlen(new_field_line);
            new_content = (char*)pvPortMalloc(new_content_size + 1);
            if (new_content) {
                strcpy(new_content, file_content);
                strcat(new_content, new_field_line);
                LFS_LOG_DEBUG("Test field appended to file\r\n");
            }
        } else {
            // 文件不存在，创建新内容
            new_content_size = strlen(new_field_line);
            new_content = (char*)pvPortMalloc(new_content_size + 1);
            if (new_content) {
                strcpy(new_content, new_field_line);
                LFS_LOG_DEBUG("New test file content created\r\n");
            }
        }
    }
    
    if (!new_content) {
        LFS_LOG_DEBUG("LFS_TEST_WRITE_ERROR: Memory allocation failed\r\n");
        if (file_content) vPortFree(file_content);
        return -1;
    }
    
    // 步骤4: 写入临时文件
    bool temp_write_success = false;
    
    // 删除可能已存在的临时文件
    lfs_delete(temp_file);
    
    // 写入临时文件
    if (log_write_with_rotation(temp_file, new_content, strlen(new_content)) == 0) {
        LFS_LOG_DEBUG("Test temp file written successfully\r\n");
        
        // 验证临时文件内容
        size_t temp_size = lfs_get_file_size(temp_file);
        if (temp_size == strlen(new_content)) {
            temp_write_success = true;
        } else {
            LFS_LOG_DEBUG("Test temp file verification failed\r\n");
        }
    }
    
    // 步骤5: 如果临时文件写入成功，替换原文件
    if (temp_write_success) {
        // 删除原文件
        lfs_delete(original_file);
        
        // 重命名临时文件为原文件
        if (lfs_rename(&lfs,temp_file, original_file) == 0) {
            LFS_LOG_DEBUG("Test info updated successfully: %s\r\n", field_header);
            if (len > 0) {
                LFS_LOG_DEBUG("Data: %.*s\r\n", len > 32 ? 32 : len, data);
            }
        } else {
            LFS_LOG_DEBUG("LFS_TEST_WRITE_ERROR: Failed to rename temp file\r\n");
            // 重命名失败，尝试恢复
            lfs_delete(temp_file);
            vPortFree(new_content);
            if (file_content) vPortFree(file_content);
            return -1;
        }
    } else {
        LFS_LOG_DEBUG("LFS_TEST_WRITE_ERROR: Temp file write failed\r\n");
        // 清理临时文件
        lfs_delete(temp_file);
        vPortFree(new_content);
        if (file_content) vPortFree(file_content);
        return -1;
    }
    
    // 步骤6: 清理内存
    vPortFree(new_content);
    if (file_content) {
        vPortFree(file_content);
    }
    
    return 0;

}
/* 获取测试信息 - 返回const char* */
const char* test_info_get(ProducitonInfoID id) {
	static char empty_string[1] = "";  // 空字符串，包含 '\0'
    // 获取字段头
    const char* field_header = get_test_field_header(id);
    if (!field_header) {
        LFS_LOG_DEBUG("LFS_TEST_READ_ERROR: Invalid field ID: %d\r\n", id);
        return empty_string;
    }

    // 使用静态缓冲区存储返回值
    static char result_buffer[256];
    memset(result_buffer, 0, sizeof(result_buffer));

    // 文件路径
    const char* filename = "/production/test.txt";
    
    // 检查文件是否存在
    if (!lfs_file_exists(filename)) {
        LFS_LOG_DEBUG("Test info file not exists\r\n");
        return empty_string;
    }

    // 获取文件大小
    size_t file_size = lfs_get_file_size(filename);
    if (file_size <= 0 || file_size >= 4096) {
        LFS_LOG_DEBUG("Invalid test file size: %d\r\n", file_size);
        return empty_string;
    }

    // 读取文件内容
    char *file_content = (char*)pvPortMalloc(file_size + 1);
    if (!file_content) {
        LFS_LOG_DEBUG("Memory allocation failed for test file\r\n");
        return empty_string;
    }
    
    int read_len = lfs_read_file(filename, file_content, file_size, 1);
    if (read_len <= 0) {
        LFS_LOG_DEBUG("Failed to read test file\r\n");
        vPortFree(file_content);
        return empty_string;
    }
    file_content[read_len] = '\0';

    // 查找字段并提取值
    bool found = false;
    char *field_start = strstr(file_content, field_header);
    if (field_start) {
        // 跳过字段头
        field_start += strlen(field_header);
        
        // 查找字段结束位置（行尾）
        char *field_end = strstr(field_start, "\r\n");
        if (!field_end) {
            // 如果没有找到行尾，可能是文件最后一行
            field_end = file_content + strlen(file_content);
        }

        // 计算字段值长度
        size_t value_len = field_end - field_start;
        if (value_len > 0 && value_len < sizeof(result_buffer)) {
            // 复制字段值到结果缓冲区
            memcpy(result_buffer, field_start, value_len);
            result_buffer[value_len] = '\0';
            found = true;
            LFS_LOG_DEBUG("Found test field %s: %s\r\n", field_header, result_buffer);
        }
    }

    // 清理资源
    vPortFree(file_content);

    if (found) {
        return result_buffer;
    } else {
        LFS_LOG_DEBUG("Test field not found: %s\r\n", field_header);
        return empty_string;
    }
    
}

/* 获取测试信息到指定缓冲区 */
int test_info_get_buffer(ProducitonInfoID id, char *buffer, uint16_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    const char* result = test_info_get(id);
    if (!result) {
        return -1;
    }
    
    size_t len = strlen(result);
    if (len >= buffer_size) {
        len = buffer_size - 1;
    }
    
    memcpy(buffer, result, len);
    buffer[len] = '\0';
    
    return len;
}

/* 安全获取测试信息（永不返回NULL） */
const char* test_info_get_safe(ProducitonInfoID id, const char* default_value) {
    const char* result = test_info_get(id);
    if (result) {
        return result;
    }
    return default_value ? default_value : "";
}
/* 删除测试信息文件 */
int test_info_delete_file(void) {
    const char* filename = "/production/test.txt";
    
    if (!lfs_file_exists(filename)) {
        LFS_LOG_DEBUG("Test info file already deleted or not exists\r\n");
        return 0;
    }
    
    int ret = lfs_delete(filename);
    if (ret == 0) {
        LFS_LOG_DEBUG("Test info file deleted successfully\r\n");
        // 同时删除可能的临时文件和备份文件
        lfs_delete("/system/test.tmp");
        lfs_delete("/system/test.bak");
        lfs_delete("/system/test.bak1");  // 如果有多个备份
        lfs_delete("/system/test.bak2");
    } else {
        LFS_LOG_DEBUG("Failed to delete test info file, error: %d\r\n", ret);
    }
    
    return ret;
}

/* 删除指定测试字段 */
int test_info_delete_field(ProducitonInfoID id) {
    // 通过设置为空字符串来删除字段
    return test_info_set("", 0, id);
}
/* 删除系统信息文件 */
int system_info_delete_file(void) {
    const char* filename = "/system/system.txt";
    
    if (!lfs_file_exists(filename)) {
        LFS_LOG_DEBUG("System info file already deleted or not exists\r\n");
        return 0;
    }
    
    int ret = lfs_delete(filename);
    if (ret == 0) {
        LFS_LOG_DEBUG("System info file deleted successfully\r\n");
        // 同时删除可能的临时文件
        lfs_delete("/system/system.tmp");
        lfs_delete("/system/system.bak");
    } else {
        LFS_LOG_DEBUG("Failed to delete system info file, error: %d\r\n", ret);
    }
    
    return ret;
}

///* 删除指定系统字段 */
//int system_info_delete_field(SystemInfoID id) {
//    // 通过设置为空字符串来删除字段
//    return system_info_set("", 0, id);
//}
int system_info_delete_field(SystemInfoID id)
{
    const char *field_header = get_system_field_header(id);
    if (!field_header) {
        LFS_LOG_DEBUG("system_info_delete_field: invalid id %d\r\n", id);
        return -1;
    }

    const char* original_file = "/system/system.txt";
    const char* temp_file     = "/system/system.tmp";

    /*----------------------------
     * 1. 如果文件不存在 => 没必要删
     *----------------------------*/
    if (!lfs_file_exists(original_file)) {
        return 0;
    }

    size_t file_size = lfs_get_file_size(original_file);
    if (file_size == 0 || file_size >= LFS_USER_TXT_FILE_MAX_SIZE) {
        return -2;
    }

    char *file_content = (char*)pvPortMalloc(file_size + 2);
    if (!file_content) return -3;

    int read_len = lfs_read_file(original_file, file_content, file_size, 1);
    if (read_len <= 0) {
        vPortFree(file_content);
        return -4;
    }
    file_content[read_len] = '\0';

    /*----------------------------
     * 2. 遍历文件逐行，过滤掉指定字段
     *----------------------------*/
    char *new_content = (char*)pvPortMalloc(file_size + 4);
    if (!new_content) {
        vPortFree(file_content);
        return -5;
    }
    new_content[0] = '\0';

    size_t header_len = strlen(field_header);
    char *p = file_content;

    while (*p) {
        char *line_end = strpbrk(p, "\r\n");
        size_t line_len = line_end ? (size_t)(line_end - p) : strlen(p);

        /*-------- 匹配行首字段 --------*/
        if (strncmp(p, field_header, header_len) == 0) {
            /* 匹配成功 -> 不写入 new_content = 删除此字段 */
        } else {
            /* 保留非目标字段 */
            strncat(new_content, p, line_len);
            strcat(new_content, "\n");
        }

        /* 跳过 CRLF、CR、LF */
        if (!line_end) break;
        if (*line_end == '\r' && *(line_end+1) == '\n')
            p = line_end + 2;
        else
            p = line_end + 1;
    }

    size_t new_size = strlen(new_content);

    /* 无变化（字段不存在） */
    if (new_size == file_size) {
        vPortFree(file_content);
        vPortFree(new_content);
        return 0;
    }

    /*----------------------------
     * 3. 写临时文件
     *----------------------------*/
    lfs_delete(temp_file);

    uint32_t w = log_write_with_rotation(temp_file, new_content, new_size);
    if (w != new_size) {
        lfs_delete(temp_file);
        vPortFree(file_content);
        vPortFree(new_content);
        return -6;
    }

    /*----------------------------
     * 4. 替换原文件
     *----------------------------*/
    lfs_delete(original_file);

    if (lfs_rename(&lfs, temp_file, original_file) != 0) {
        lfs_delete(temp_file);
        vPortFree(file_content);
        vPortFree(new_content);
        return -7;
    }

    vPortFree(file_content);
    vPortFree(new_content);

    LFS_LOG_DEBUG("system field deleted: %s\r\n", field_header);
    return 0;
}

///* 设置设备 SN */
//void siminfo_set_sn(const char *sn,uint16_t len) {
//	
//		 LFS_LOG_DEBUG("eeee siminfo_set_sn\r\n");
//	
//#if (LFS_INIT_IN_TASK_LATER)
//    // 等待文件系统或Flash准备就绪
//    int32_t result = osEventFlagsWait(sys_events,
//                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
//                                      osFlagsWaitAll | osFlagsNoClear,
//                                      LFS_WAIT_TIMEOUT_MS);
//    if (result < 0) {
//        if (result == osFlagsErrorTimeout) {
//            LFS_LOG_DEBUG("[LFS] wait timeout for ready flags\r\n");
//        } else {
//            LFS_LOG_DEBUG("[LFS] event wait error: %ld\r\n", result);
//        }
//        return ;
//    }
//#endif
//	LFS_LOG_DEBUG("lfs_read_file = %d %d %d,%d\r\n",sizeof(sim_info.device_sn),strlen(sn),sizeof(sim_info.device_sn)/sizeof(sim_info.device_sn[0]), len);
//   // 检查参数是否有效 字符指针是否为空 字符串长度是否超过最大长度 
//   if (!sn || strlen(sn)!=len || len > (sizeof(sim_info.device_sn)-1)) 
//   {
//        return;
//   }
//	memset(&sim_info,0,sizeof(sim_info));
//#if (LFS_BACKUP_AREA_FLAG)
//   if(backup_select ==1)
//   {
//		Get_SystemInfo_Backup(&sim_info.device_sn[0],sizeof(sim_info));
//	   
//		memset(&sim_info.device_sn[0],0,sizeof(sim_info.device_sn));
//		memcpy(sim_info.device_sn, sn, len);
//		sim_info.device_sn[len] = '\0';
//	   
//		Set_SystemInfo_Backup(&sim_info.device_sn[0],sizeof(sim_info));
//   }
//   else
//#endif
//   {
//   
//		lfs_read_file("/system/system.txt", &sim_info.device_sn[0],sizeof(sim_info),1);
//		
//		lfs_delete_bytes("/system/system.txt",0,sizeof(sim_info));//删除字段
//   
//		memset(&sim_info.device_sn[0],0,sizeof(sim_info.device_sn));
//		strncpy(sim_info.device_sn, sn, len);
//		sim_info.device_sn[len] = '\0';
//		
//		log_write_with_rotation("/system/system.txt",&sim_info.device_sn[0],sizeof(sim_info));//写操作
//	}
//#if (LFS_BACKUP_AREA_FLAG)
//	SimInfo_t sim_info_tmp;
//	if(Get_SystemInfo_Backup(&sim_info_tmp.device_sn[0],sizeof(sim_info_tmp)) == -3)
//	{
//		Set_SystemInfo_Backup(&sim_info.device_sn[0],sizeof(sim_info.device_sn));
//	}
//#endif
//}
///* 获取设备 SN */
//const char* siminfo_get_sn(void) {
//	 LFS_LOG_DEBUG("eeee siminfo_get_sn\r\n");
//	char *return_data=NULL; 

//#if (LFS_INIT_IN_TASK_LATER)
//    // 等待文件系统或Flash准备就绪
//    int32_t result = osEventFlagsWait(sys_events,
//                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
//                                      osFlagsWaitAll | osFlagsNoClear,
//                                      LFS_WAIT_TIMEOUT_MS);
//    if (result < 0) {
//        if (result == osFlagsErrorTimeout) {
//            LFS_LOG_DEBUG("[LFS] wait timeout for ready flags\r\n");
//        } else {
//            LFS_LOG_DEBUG("[LFS] event wait error: %ld\r\n", result);
//        }
//        return NULL;
//    }
//#endif
//	int get_byte_num=0;
//	memset(&sim_info,0,sizeof(sim_info));
//#if (LFS_BACKUP_AREA_FLAG)
//	if(backup_select ==1)
//	{
//		get_byte_num = Get_SystemInfo_Backup(&sim_info.device_sn[0],sizeof(sim_info.device_sn));
//	}
//	else
//#endif
//	{
//		get_byte_num = lfs_read_file("/system/system.txt", &sim_info.device_sn[0],sizeof(sim_info.device_sn),1);
//	}
////	LFS_LOG_DEBUG("siminfo_get_sn = %d %d\r\n",get_byte_num,backup_select);
//	if(get_byte_num <= 0)
//	{
//		memset(&sim_info.device_sn[0],0,sizeof(sim_info.device_sn));
//		sim_info.device_sn[0]='\0';
//	}
//	LFS_LOG_DEBUG("**sn: %s\r\n",sim_info.device_sn);
//	
//	
//	return_data = sim_info.device_sn;
//	return return_data;
//}


void test_sn_set(char *sn,uint16_t len)
{
	uint8_t ererere[33]={0};
	lfs_read_file("/system/system.txt", ererere,33,1);
	uint8_t test_flag=0;
	LFS_LOG_DEBUG("******** = ");
	for(int i=0;i<33;i++)
	{
			LFS_LOG_DEBUG("%#x ",ererere[i]);
	}
	LFS_LOG_DEBUG("\r\n");
	//lfs_list_dir("/");
	const char *sn_ptr=NULL;
	LFS_LOG_DEBUG("test_sn_set = %s %d\r\n",sn,len);
//	siminfo_set_sn(sn,len);
	system_info_set(sn,len,SYS_DEVICE_SN_ID);
	sn_ptr = system_info_get(SYS_DEVICE_SN_ID);
	LFS_LOG_DEBUG("siminfo_get_sn = %s\r\n",sn_ptr);
	//lfs_list_dir("/");
	lfs_read_file("/system/system.txt", ererere,33,1);
	LFS_LOG_DEBUG("-------- = ");
	for(int i=0;i<33;i++)
	{
		LFS_LOG_DEBUG("%#x ",ererere[i]);
	}
	LFS_LOG_DEBUG("\r\n");
}
#if (LFS_BACKUP_AREA_FLAG)
/*
* @brief  设置用户绑定信息备份
* @param  data  备份数据
* @param  size  备份数据大小
* @retval 无
*/
void set_user_bindinfo_Backup(uint8_t *data,uint32_t size)
{
    for(int i=0;i<512/8;i++)
    {
        drv_flash_erase(OM_FLASH1,USER_BIND_INFO_ADDR+i*(64*1024), FLASH_ERASE_64K, 1000);
    }
    drv_flash_write(OM_FLASH1,USER_BIND_INFO_ADDR, data, size, 1000);
}
/*
* @brief  获取用户绑定信息备份
* @param  buf  备份数据缓存
* @param  size  备份数据大小
* @retval 0 成功 -1 参数错误 -2 读取失败
*/
int get_user_bindinfo_Backup(uint8_t *buf, uint32_t size)
{
    if (!buf || size == 0) {
        return -1;   // 参数错误
    }

    // 从 flash 固定地址读取
    int ret = drv_flash_read(OM_FLASH1, USER_BIND_INFO_ADDR, buf, size);
    if (ret != 0) {
        return -2;   // 读取失败
    }

    return 0; // 成功
}
#endif 
void littlefs_create_flag_set(ReadLfsCreatFlag status)
{
	littlefs_create_flag = status;
}
ReadLfsCreatFlag littlefs_create_flag_get(void)
{
	return littlefs_create_flag;
}
void osEventLittleFsFlagsInit(void)
{
#if (LFS_INIT_IN_TASK_LATER)
	sys_events = osEventFlagsNew(NULL);
	   if (sys_events == NULL) {
		   LFS_LOG_DEBUG("osEventLittleFsFlagsInit failed\r\n");
        // 创建失败
    }
	   else{
	      LFS_LOG_DEBUG("osEventLittleFsFlagsInit finish\r\n");
	   }
#endif
	   
	   
}
// 使用现有的函数组合实现
//extern om_error_t wakeup_and_reconfigure(void);
/**
 * @brief   外部flash初始化
 * @param   void
 * @note    需要定义头文件 #include "../source/drv_flash/drv_flash.h"以及引脚定义
 */
void user_exflash_quad_init(void)
{
    /* external flash */
#if (LFS_GPIO_CONFIG)
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
#endif
    // Init Flash
    flash_config_t oflash_config = {
        .clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / 32000000U,
        .delay = FLASH_DELAY_AUTO,
        .read_cmd = FLASH_FAST_READ_QIO,
        .write_cmd = FLASH_PAGE_PROGRAM_QI,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH1, &oflash_config);
#if (LFS_INIT_IN_TASK_LATER)
	osEventFlagsSet(sys_events, EXFLASH_INIT_READY);
#endif
	LFS_LOG_DEBUG("user_exflash_quad_init finish\r\n");
	
//	wakeup_and_reconfigure();
	
#if (EXFLASH_TEST_START!=0)
		/// Buffer that stored the data to be written
	uint8_t write_buf[100];
	/// Buffer that stored the data to be read
	uint8_t read_buf[100];
    drv_flash_read(OM_FLASH1, 0 * 1024, read_buf, 100, 1000);
	
	LFS_LOG_DEBUG_ARRAY_EX("init_read =",read_buf,100);

	// Erase 4k in 128k
	drv_flash_erase(OM_FLASH1, 0 * 1024, FLASH_ERASE_4K, 1000);
	// Read 100 bytes in 128k, it should be all 0xFF
	drv_flash_read(OM_FLASH1, 0 * 1024, read_buf, 100, 1000);
	
	LFS_LOG_DEBUG_ARRAY_EX("erase read =",read_buf,100);
	for(int i=0;i<100;i++)
	{
		write_buf[i]=i;
	}
	// Write 100 bytes to 128k
	drv_flash_write(OM_FLASH1, 0 * 1024, write_buf, 100, 1000);
	// Read 100 bytes in 128k, it should be same as write_buf
	drv_flash_read(OM_FLASH1, 0 * 1024, read_buf, 100, 1000);
	
	LFS_LOG_DEBUG_ARRAY_EX("write read =",read_buf,100);
	
#endif

}
/**
 * @brief littlefs read interface
 * @param [in] c lfs_config数据结构
 * @param [in] block 要读的块
 * @param [in] off 在当前块的偏移
 * @param [out] buffer 读取到的数据
 * @param [in] size 要读取的字节数
 * @return 0 成功 <0 错误
 * @note littlefs 一定不会存在跨越块存储的情况
 */
int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block,lfs_off_t off, void *buffer, lfs_size_t size)
{
	int return_data =0;

	return_data = drv_flash_read(OM_FLASH1, c->block_size*block+off, buffer, size);
	//LFS_LOG_DEBUG("drv_flash_read = %lu %lu %lu\r\n",block, off, size);
	//LFS_LOG_DEBUG("read blk=%lu off=%lu size=%lu\n", block, off, size);
//	LFS_LOG_DEBUG(" return = %d\r\n",return_data);
	 return (return_data == 0) ? LFS_ERR_OK : LFS_ERR_IO;
}

/**
 * @brief littlefs write interface
 * @param [in] c lfs_config数据结构
 * @param [in] block 要读的块
 * @param [in] off 在当前块的偏移
 * @param [out] buffer 读取到的数据
 * @param [in] size 要读取的字节数
 * @return 0 成功 <0 错误
 * @note littlefs 一定不会存在跨越块存储的情况
 */
int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block,lfs_off_t off, const void *buffer, lfs_size_t size)
{
	int return_data =0;
	//LFS_LOG_DEBUG("drv_flash_write = %lu %lu %lu \r\n",block, off, size);
	return_data =	drv_flash_write(OM_FLASH1, c->block_size*block+off,(uint8_t *)buffer, size);
//	LFS_LOG_DEBUG(" return = %d\r\n",return_data);
	return (return_data == 0) ? LFS_ERR_OK : LFS_ERR_IO;
}

/**
 * @brief littlefs 擦除一个块
 * @param [in] c lfs_config数据结构
 * @param [in] block 要擦出的块
 * @return 0 成功 <0 错误
 */
int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{	
	int return_data =0;
	//LFS_LOG_DEBUG("drv_flash_write = %lu \r\n",block);
	return_data = drv_flash_erase(OM_FLASH1, c->block_size*block, FLASH_ERASE_4K);
//	LFS_LOG_DEBUG(" return = %d\r\n",return_data);
	return (return_data == 0) ? LFS_ERR_OK : LFS_ERR_IO;
}
/**
 * @brief littlefs 写缓冲
 * @param [in] c lfs_config数据结构
 * @return 0 成功 <0 错误
 * @note flash驱动是立即写入所以不需要具体定义该函数
 */
int user_provided_block_device_sync(const struct lfs_config *c )
{
	return  LFS_ERR_OK;
}

#ifdef LFS_NO_MALLOC
// 分配静态缓冲区（.bss段，自动清零）
static uint8_t read_buffer[CACHE_SIZE];
static uint8_t prog_buffer[CACHE_SIZE];
static uint8_t lookahead_buffer[LOOKAHEAD_SIZE];
#endif

// entry point
void lfs_port_test1(void) {
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);
	LFS_LOG_DEBUG("lfs_mount = %d\r\n",err);
    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &cfg);
        err = lfs_mount(&lfs, &cfg);
		LFS_LOG_DEBUG("res_lfs_mount = %d\r\n",err);
    }
    uint8_t boot_count[100] = {0};
	// 配置512字节的自定义缓冲区
//	if (lfs_mkdir(&lfs, "/data") < 0) {
//    // 可忽略或打印警告
//}
	int errr = lfs_file_open(&lfs, &file, "/log.txt", 
                          LFS_O_RDWR | LFS_O_CREAT);
	LFS_LOG_DEBUG("lfs_file_open =%d\r\n",errr);
	if (errr) {
		return;
	}	
	 lfs_file_rewind(&lfs, &file);
	LFS_LOG_DEBUG("lfs_file_write = %d\r\n",lfs_file_write(&lfs, &file, "Hello World!wangbnehushigedashuibi", 20));

	lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET);  // 回到文件开头

	LFS_LOG_DEBUG("lfs_file_read =%d\r\n",lfs_file_read(&lfs, &file, boot_count, 100));

	LFS_LOG_DEBUG("lfs_file_read_data =%s\r\n",boot_count);
	lfs_file_close(&lfs, &file);
    // release any resources we were using
    lfs_unmount(&lfs);
}



void lfs_port_test2(void)
{
	//初始化
	lfs_init();

	//读文件系统
		// lfs_list_dir("/data");
	//写文件系统
//	uint8_t test[200]={0};
//	for(int i =0;i<200;i++)
//	{
//		test[i]=i+1;
//	}
//	log_write_with_rotation("/data/log.txt",test,200);
//	
//	//读目录列表
//	lfs_list_dir("/data");
	//读文件系统
	
	//jeson格式写
	//读取文件系统
	//读文件系统
	//读取时间戳
	//设置时间戳
	//文件列表
	//重复读写已有文件
}

#define TEST_FILE       "power_test.bin"
#define MIN_DELAY_MS    100
#define MAX_DELAY_MS    2000

typedef struct {
    uint32_t counter;
    uint32_t crc;
} test_data_t;

static uint32_t crc32_calc(const void *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p = data;
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

static uint32_t get_random_delay(void) {
    return (rand() % (MAX_DELAY_MS - MIN_DELAY_MS + 1)) + MIN_DELAY_MS;
}

static void simulate_power_loss(void) {
    NVIC_SystemReset();
}
/**
 * @brief 文件系统的系统文件内写入LTE版本号
 * 
 * @param w_data 数据
 * @param w_len 数据长度（字节）
 * @param id 字段选择 @SystemInfoID
 * @return int 0=成功, 其它=错误码
 */
uint8_t lfs_system_write(char *w_data,char w_len,SystemInfoID id)
{
	int r_vlue = 0;
	if(lfs_mount_safe() < 0 || id >= SYS_FIELD_COUNT)
		return 1;
	system_info_set(w_data,w_len,id);
	//lfs_unmount_safe();
	return r_vlue;
}
const char* lfs_system_read(SystemInfoID id)
{
	static char buffer[64]; // 使用静态缓冲区，确保返回后内存有效
    buffer[0] = '\0';
    
    if(lfs_mount_safe() < 0 || id >= SYS_FIELD_COUNT)
        return buffer;
    
    const char *tmp = system_info_get(id);
    if(tmp) {
        strncpy(buffer, tmp, sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0'; // 确保null终止
    }
    
//    lfs_unmount_safe();
    return buffer;
}

/*
*测试掉电保存
*正常：
*Last counter: 159
*Written counter: 160
*Power loss in 452 ms
*上次掉电前写的 159 是完整的（CRC 校验成功）。
*这次成功写入了 160。

*异常：
*CRC ERROR! File corrupted!
*Written counter: 412
*Power loss in 1500 ms
*上次掉电时文件没写完整（可能只写了一半），CRC 校验失败。
*本次继续写 412 进入文件。
*/
void littlefs_power_loss_auto_test(void) {
    test_data_t data = {0};
	
	
	uint8_t buffer_tmp[100]={0};
	for(int i=0;i<100;i++)
		buffer_tmp[i]=i+1;
//LFS_LOG_DEBUG("littlefs_power_loss_auto_test\r\n");
    // 挂载
    if (lfs_mount(&lfs, &cfg) != 0) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    // 读取上次记录
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, TEST_FILE, LFS_O_RDONLY) == 0) {
        if (lfs_file_read(&lfs, &file, &data, sizeof(data)) == sizeof(data)) {
            uint32_t crc_calc = crc32_calc(&data.counter, sizeof(data.counter));
            if (crc_calc != data.crc) {
                LFS_LOG_DEBUG("CRC ERROR! File corrupted!\n");
            } else {
                LFS_LOG_DEBUG("Last counter: %lu\n", data.counter);
            }
        }
        lfs_file_close(&lfs, &file);
    } else {
        LFS_LOG_DEBUG("No previous file, start at 0\n");
        data.counter = 0;
    }

    // 递增计数
    data.counter++;
    data.crc = crc32_calc(&data.counter, sizeof(data.counter));

	if(data.counter==7)
	{
		drv_flash_write(OM_FLASH1, 4*1024,(uint8_t *)buffer_tmp, 100);
	}
	else
	{
		 // 写入
			   if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return ;
    }
	}
	
    if (lfs_file_open(&lfs, &file, TEST_FILE,
                      LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) == 0) {
        lfs_file_write(&lfs, &file, &data, sizeof(data));
        lfs_file_sync(&lfs, &file);
        lfs_file_close(&lfs, &file);
    }
   // lfs_sync(&lfs);
    osMutexRelease(lfs_mutex);

    LFS_LOG_DEBUG("Written counter: %lu\n", data.counter);

    // 随机延时
  //  srand(data.counter);
  //  uint32_t delay = get_random_delay();
   // LFS_LOG_DEBUG("Power loss in %lu ms\n", delay);
    fflush(stdout);
   // osDelay(delay);

    // 掉电
  //  simulate_power_loss();
}
void test_lfs_powercut() {
    // 挂载
    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        // 如果没格式化过，先格式化
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    // 打开文件写入
    lfs_file_t file;
    lfs_file_open(&lfs, &file, "test.txt", LFS_O_WRONLY | LFS_O_CREAT);

    // 写入多段数据，每段写后 sync
    for (int i = 0; i < 10; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "write round %d\n", i);

        lfs_file_write(&lfs, &file, buffer, strlen(buffer));

        // 每轮都 sync，确保数据提交
        lfs_file_sync(&lfs, &file);

        // 模拟“掉电点”
        if (i == 5) {
			simulate_power_loss();
            // 在这里强制重启/断电
            // 嵌入式上可以直接 NVIC_SystemReset();
            // PC 仿真上可以 exit(0);
        }
    }

    lfs_file_close(&lfs, &file);

    // 卸载
    lfs_unmount(&lfs);
}// 重启后调用此函数验证
void verify_lfs_after_reset() {
    lfs_mount(&lfs, &cfg);

    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, "test.txt", LFS_O_RDONLY) >= 0) {
        char buffer[64];
        while (true) {
            int size = lfs_file_read(&lfs, &file, buffer, sizeof(buffer)-1);
            if (size <= 0) break;
            buffer[size] = '\0';
            LFS_LOG_DEBUG("%s", buffer);
        }
        lfs_file_close(&lfs, &file);
    } else {
        LFS_LOG_DEBUG("file not found\n");
    }

    lfs_unmount(&lfs);
}
// 自动写时间戳
static void lfs_update_mtime(const char *path) {
	 uint32_t now = get_current_unix_time();  //  实现见下
    lfs_setattr(&lfs, path, LFS_ATTR_MTIME, &littlefs_date, sizeof(littlefs_date));
}
// 创建目录（存在则跳过）
static int fs_mkdir_if_needed(const char *path) {
    int err = lfs_mkdir(&lfs, path);
    if (err < 0 && err != LFS_ERR_EXIST) {
        return err;
    }
	lfs_update_mtime(path);
    return 0;
}

// 创建文件（存在则跳过）
static int fs_touch(const char *path) {
    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, path, LFS_O_RDWR | LFS_O_CREAT);
    if (err < 0) {
        return err;
    }
	lfs_update_mtime(path);
    lfs_file_close(&lfs, &file);
    return 0;
}

// 遍历初始化
int fs_init_structure(void) {
    int err;
    // 创建所有目录
    for (size_t i = 0; i < FS_DIR_COUNT; i++) {
        err = fs_mkdir_if_needed(fs_directories[i]);
        if (err < 0) {
            LFS_LOG_DEBUG("mkdir failed: %s (%d)\n", fs_directories[i], err);
            return err;
        }
    }

    // 创建所有文件
    for (size_t i = 0; i < FS_FILE_COUNT; i++) {
        err = fs_touch(fs_files[i]);
        if (err < 0) {
            LFS_LOG_DEBUG("file create failed: %s (%d)\n", fs_files[i], err);
            return err;
        }
    }

    return 0; // 成功
}
void vStartLittleFsTask(void);

//static void lfs_sleep_quit_callback(pm_sleep_state_t sleep_state, pm_status_t power_status)
//{
//    if (sleep_state == PM_SLEEP_RESTORE_HSI) {
//		//user_exflash_quad_init();
//    }
//}


/**
 * @brief 更新用户绑定信息状态
 * @return 无
 */
void update_user_bind_status(void)
{
	if(is_lfs_mounted()==false)
	{
		lfs_mount_safe();
	}
    if (user_bindinfo_check()) {
        littlefs_create_flag_set(LFS_USER_INFO_GET_SUCCEED);
    } else {
        littlefs_create_flag_set(LFS_USER_INFO_GET_FAILED);
    }
}
/*
* 安全挂载文件系统
* */
int lfs_mount_safe(void)
{
    int ret = 0;
	if(is_lfs_mounted())
	{
		return 0;
	}
	
	LFS_LOG_DEBUG("***************lfs_init_start*****************\r\n");
	uint32_t result = 0;
	    // 等待外部Flash或系统准备好
    result = osEventFlagsWait(sys_events,
                                      EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);
    if (result & osFlagsError) {
        user_exflash_quad_init();
    }
#if (LFS_INIT_IN_TASK_LATER)
	
    // 等待外部Flash或系统准备好
    result = osEventFlagsWait(sys_events,
                                      EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);
//    if ((int32_t)result < 0) {
	if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
         //   LFS_LOG_DEBUG("lfs_mount_safe wait timeout EXFLASH_INIT_READY flag\r\n");
        } else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
          //  LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
        }
        return -255;
    }
#endif

	    // 创建互斥锁
//    const osMutexAttr_t mutex_attr = { .name = "lfs_mutex" };
//    lfs_mutex = osMutexNew(&mutex_attr);
	lfs_mutex_init();

    // === 尝试挂载 ===
    ret = lfs_mount(&lfs, &cfg);
	log_debug("lfs_mount = %d\r\n",ret);
    if (ret == 0) {
#if (LFS_INIT_IN_TASK_LATER)
	osEventFlagsSet(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY);
#endif
        LFS_LOG_DEBUG("[LFS] mount success\r\n");
		if(strlen(system_info_get(SYS_DEVICE_SN_ID))<=0)
        {
            littlefs_create_flag_set(LFS_SN_GET_FAILED);
        }
        else{
			fs_init_structure();//补充目录和文件
            update_user_bind_status();
        }
		lfs_list_dir("/");
        return 0;

    }
    else{
        littlefs_create_flag_set(LFS_CREATE_FAILED);
		return -1;
    }
     return 0;
}



void lfs_timestamp_init(void)
{
	 pmu_reboot_reason_t reboot_reason = m_system_get_reboot_reason();
	 
	//LFS_LOG_DEBUG("reboot_reason %d\n", reboot_reason);

	if(reboot_reason==PMU_REBOOT_FROM_SOFT_RESET_USER || reboot_reason == PMU_REBOOT_FROM_WDT)
    {
        timestamp_rtc_init(0);
    }else{
        timestamp_rtc_init(1);
    }
	time_struct_t get_time;
	get_timestamp_date(&get_time);
	
	LFS_LOG_DEBUG("get_timestamp_date = %d-%d-%d:%d-%d-%d\r\n", get_time.year,get_time.month,get_time.day,\
	get_time.hour,get_time.minute,get_time.second);
}

/*
* 文件系统初始化
* */
int8_t lfs_init(void) {
	
	LFS_LOG_DEBUG("***************lfs_init_start*****************\r\n");
	user_exflash_quad_init();
		    // 创建互斥锁
//    const osMutexAttr_t mutex_attr = { .name = "lfs_mutex" };
//    lfs_mutex = osMutexNew(&mutex_attr);
	lfs_mutex_init();
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result = osEventFlagsWait(sys_events, EXFLASH_INIT_READY, osFlagsWaitAny | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
			
		}
		return -1;
	}
#endif
	   if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -2;
    }
    // 尝试挂载
    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        
		 // 格式化
        err = lfs_format(&lfs, &cfg);
        if (err) {
            LFS_LOG_DEBUG("lfs_format failed (%d)\n", err);
			 return -3;
        }

        // 再次挂载
        err = lfs_mount(&lfs, &cfg);
        if (err) {
			 return -4;
        }
		else
		{
			LFS_LOG_DEBUG("LFS_FIRST_CREAT\r\n");
		}
    }
	else
	{
		LFS_LOG_DEBUG("LFS_FIRST_CREAT_NOT\r\n");
	}

// 2. 初始化目录/文件结构
    err = fs_init_structure();
    if (err == 0) {
		
        LFS_LOG_DEBUG("FS structure created OK\n");
    }
	else{
	 return -5;
	}
    // 可选：创建目录
//    lfs_mkdir(&lfs, "/data");
    osMutexRelease(lfs_mutex);

	
	osEventFlagsSet(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY);

	LFS_LOG_DEBUG("lfs_init finish\r\n");
	
	lfs_list_dir("/");
//	pm_sleep_store_restore_callback_register(lfs_sleep_quit_callback);
	LFS_LOG_DEBUG("*************lfs_init_end*******************\r\n");
	return 0;
}

static void rotate_logs(const char *filename) {
    struct lfs_info info;
    char oldname[64], newname[64];
  last_log_op = LOG_OP_ROTATE;  // 标记最近操作是轮转
    for (int i = LOG_ROTATE_MAX - 1; i >= 1; i--) {
        snprintf(oldname, sizeof(oldname), "%s.%d.txt", filename, i);
        snprintf(newname, sizeof(newname), "%s.%d.txt", filename, i + 1);
        if (lfs_stat(&lfs, oldname, &info) == 0) {
            lfs_remove(&lfs, newname);
            lfs_rename(&lfs, oldname, newname);
        }
    }

    snprintf(newname, sizeof(newname), "%s.1.txt", filename);
    lfs_remove(&lfs, newname);
    lfs_rename(&lfs, filename, newname);
}
/**
 * @brief 安全格式化 LittleFS 文件系统
 * 
 * @return int 
 *         0   = 成功  
 *        <0   = 失败（返回 LittleFS 错误码）
 */
int lfs_format_safe(void)
{
	LFS_LOG_DEBUG("lfs_format_safe\r\n");
    int ret = 0;

#if (LFS_INIT_IN_TASK_LATER)
    // 等待外部Flash准备就绪（如GT25Q64A）
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      osWaitForever);
    if (result < 0) {
        LFS_LOG_DEBUG("[LFS] wait flash ready failed (%ld)\r\n", result);
        return -255;
    }
#endif

    // === 加锁保护 ===
    if (osMutexAcquire(lfs_mutex, osWaitForever) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire failed!\r\n");
        return -1;
    }

    // === 先尝试卸载（防止已挂载）===
    lfs_unmount(&lfs);

    LFS_LOG_DEBUG("[LFS] start format...\r\n");

    // === 执行格式化 ===
    ret = lfs_format(&lfs, &cfg);
    if (ret < 0) {
        LFS_LOG_DEBUG("[LFS] format failed, code=%d\r\n", ret);
        osMutexRelease(lfs_mutex);
        return ret;
    }
#if (LFS_INIT_IN_TASK_LATER)
    osEventFlagsClear(sys_events, EVENT_LFS_READY);
#endif
    osMutexRelease(lfs_mutex);
    return ret;
}

// === 函数返回值 ===
//  0 : 成功
// <0 : 错误码

int lfs_unmount_safe(void)
{
    int ret = 0;
LFS_LOG_DEBUG("lfs_unmount_safe\r\n");
	
#if (LFS_INIT_IN_TASK_LATER)
    // 等待文件系统或Flash准备就绪
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);
    if (result < 0) {
        if (result == osFlagsErrorTimeout) {
            LFS_LOG_DEBUG("[LFS] wait timeout for ready flags\r\n");
        } else {
            LFS_LOG_DEBUG("[LFS] event wait error: %ld\r\n", result);
        }
        return -255;
    }
#endif

    // === 加锁保护 ===
    if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire failed!\r\n");
        return -1;
    }

    // === 执行安全卸载 ===
    ret = lfs_unmount(&lfs);
    if (ret < 0) {
        LFS_LOG_DEBUG("[LFS] unmount failed, code=%d\r\n", ret);
    } else {
        LFS_LOG_DEBUG("[LFS] unmount success\r\n");
    }
#if (LFS_INIT_IN_TASK_LATER)
    osEventFlagsClear(sys_events, EVENT_LFS_READY);
#endif
    osMutexRelease(lfs_mutex);
    return ret;
}
// 判断挂载是否成功
bool is_lfs_mounted(void)
{
	
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result = osEventFlagsWait(sys_events, EVENT_LFS_READY, osFlagsWaitAny | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
			
		}
		return false;
	}
#else
    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        LFS_LOG_DEBUG("Mount failed: %d\n", err);
        return false;
    }

#endif
    return true;
}
/*
*带轮转写且不超过最大限制 
*********** 增加写文件类型和大小限制***********
*/
int log_write_with_rotation_limmax(const char *filename, const char *msg) {
	LFS_LOG_DEBUG("log_write_with_rotation_limmax\r\n");
#if (LFS_INIT_IN_TASK_LATER)
    uint32_t result = osEventFlagsWait(sys_events,
                    EVENT_LFS_READY | EXFLASH_INIT_READY,
                    osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
    if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
            LFS_LOG_DEBUG("log_write_with_rotation_limmax wait timeout EXFLASH_INIT_READY flag\r\n");
        } else {
            LFS_LOG_DEBUG("evt wait error code %ld\r\n", result);
        }
        return -255;
    }
#endif
		   if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }

    const char *p = msg;
    size_t remain_len = strlen(msg);
    int total_written = 0;

    while (remain_len > 0) {
        struct lfs_info info;
        if (lfs_stat(&lfs, filename, &info) == 0 && info.size >= LOG_MAX_SIZE) {
            rotate_logs(filename);
        }
		else {
			last_log_op = LOG_OP_WRITE; // 标记最近操作是写入
		}
        size_t space_left = LOG_MAX_SIZE;
        if (lfs_stat(&lfs, filename, &info) == 0) {
            if (info.size < LOG_MAX_SIZE) {
                space_left = LOG_MAX_SIZE - info.size;
            } else {
                space_left = 0;
            }
        }

        if (space_left == 0) {
            rotate_logs(filename);
            space_left = LOG_MAX_SIZE;
        }

        size_t chunk = (remain_len < space_left) ? remain_len : space_left;

        lfs_file_t file;
        int err = lfs_file_open(&lfs, &file, filename,
                                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
        if (err < 0) {
            osMutexRelease(lfs_mutex);
            return err;
        }

        int written = lfs_file_write(&lfs, &file, p, chunk);
        lfs_file_sync(&lfs, &file);

        // 更新时间属性

		uint32_t now = get_current_unix_time();  // 实现见下
		if(now>0)
		{
			lfs_setattr(&lfs, filename, LFS_ATTR_MTIME, &littlefs_date, sizeof(littlefs_date));
		}
	
        lfs_file_close(&lfs, &file);

        if (written > 0) {
            total_written += written;
            remain_len -= written;
            p += written;
        } else {
            break;
        }
    }

    osMutexRelease(lfs_mutex);
    return total_written;
}
/*
*带轮转写且可超过最大限制
*/
//轮转写
int log_write_with_rotation(const char *filename,const void *data, size_t len) {
#if (LFS_INIT_IN_TASK_LATER)
	 // 等待 LFS 和外部 Flash 初始化完成
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
		if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			//LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
			
		}
		return -255;
	} 
#endif
		  if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
    // 检查当前日志大小，必要时轮转
    struct lfs_info info;
    int has = lfs_stat(&lfs, filename, &info);
    if (has == 0 && info.size >= LOG_MAX_SIZE) {
        // 执行轮换，先删除最旧
        char oldname[64], newname[64];
		last_log_op = LOG_OP_ROTATE;  // 标记最近操作是轮转
        for (int i = LOG_ROTATE_MAX - 1; i >= 1; i--) {
            snprintf(oldname, sizeof(oldname), "%s.%d.txt", filename, i);
            snprintf(newname, sizeof(newname), "%s.%d.txt", filename, i + 1);

            if (lfs_stat(&lfs, oldname, &info) == 0) {
                lfs_remove(&lfs, newname);          // 删除 log.N+1
                lfs_rename(&lfs, oldname, newname); // log.N → log.N+1
            }
        }

        // log.txt → log.1.txt 
		snprintf(newname, sizeof(newname), "%s.1.txt", filename);
		lfs_remove(&lfs, newname);
        lfs_rename(&lfs, filename, newname);
    }
	else
	{
		last_log_op = LOG_OP_WRITE; // 标记最近操作是写入
	}

    // 写入当前日志
    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, filename,
                            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (err < 0) {
        osMutexRelease(lfs_mutex);
        return err;
    }

    int written = lfs_file_write(&lfs, &file, data, len);


    // 更新 mtime 属性
    uint32_t now = get_current_unix_time();  // ?? 实现见下
    lfs_setattr(&lfs, filename, LFS_ATTR_MTIME, &littlefs_date, sizeof(littlefs_date));
	
    lfs_file_close(&lfs, &file);
    osMutexRelease(lfs_mutex);
    return written;
}
int get_latest_log_file(RecordLastLogFileInfo *latest) {
    lfs_dir_t dir;
    struct lfs_info info;
#if (LFS_INIT_IN_TASK_LATER)
	
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
		if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			//LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
			
		}
		return -255;
	}
#endif
		if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
			LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
			return -254;
		}
    uint32_t max_time = 0;
		time_struct_t littlefs_mtime;
	
    char latest_name[64] = {0};

    if (lfs_dir_open(&lfs, &dir, "/data") < 0) { 
		osMutexRelease(lfs_mutex);
        return -1;
    }

    while (lfs_dir_read(&lfs, &dir, &info) > 0) {
	
        if (info.type == LFS_TYPE_REG && strstr(info.name, "log") == info.name) {
            uint32_t mtime = 0;
		char fullpath[128];
		snprintf(fullpath, sizeof(fullpath), "%s/%s", "/data", info.name);
			
	//	LFS_LOG_DEBUG("111=%d\r\n",lfs_getattr(&lfs, fullpath, LFS_ATTR_MTIME, &littlefs_mtime, sizeof(littlefs_mtime)));
            if (lfs_getattr(&lfs, fullpath, LFS_ATTR_MTIME, &littlefs_mtime, sizeof(littlefs_mtime)) >0) {
				mtime = datetime_to_seconds(littlefs_mtime);
                if (mtime > max_time) {
                    max_time = mtime;
                    strncpy(latest_name, info.name, sizeof(latest_name));
                }
            }
        }
    }
    lfs_dir_close(&lfs, &dir);
    if (max_time > 0) {
		memcpy(&latest->s_mtime,&littlefs_mtime,sizeof(time_struct_t));
       // latest->mtime = max_time;
        strncpy(latest->name, latest_name, sizeof(latest->name));
		 osMutexRelease(lfs_mutex);
        return 0;
    }
	 osMutexRelease(lfs_mutex);
    return -2;
}
/////////////////////////////////////////////////////////
/***************************************************************************************/
int wav_get_file_size_from_iflash(uint32_t flash_addr, uint32_t *file_size)
{
    uint8_t header[12];

    if (file_size == NULL)
        return -1;

	drv_flash_read(OM_FLASH0,  flash_addr, header, 12);
  //  memcpy(header, (uint8_t *)flash_addr, 12);
	LFS_LOG_DEBUG("flash read = ");
	for(int i =0;i<12;i++)
	{
		LFS_LOG_DEBUG(" %02x",header[i]);
	}
	LFS_LOG_DEBUG("\r\n");
    /* 检查 RIFF */
    if (memcmp(header, "RIFF", 4) != 0)
        return -2;

    /* 检查 WAVE */
    if (memcmp(&header[8], "WAVE", 4) != 0)
        return -3;

    uint32_t chunk_size =
          (uint32_t)header[4]
        | ((uint32_t)header[5] << 8)
        | ((uint32_t)header[6] << 16)
        | ((uint32_t)header[7] << 24);

    *file_size = chunk_size + 8;

    return 0;
}
/*获取文件的大小*/
int lfs_get_wav_file_size(const char *path, uint32_t *size)
{
    struct lfs_info info;

    if (path == NULL || size == NULL)
        return -1;

    int ret = lfs_stat(&lfs, path, &info);
    if (ret < 0)
        return -2;

    if (info.type != LFS_TYPE_REG)
        return -3;

    *size = info.size;

    return 0;
}
/*根据序列号去获取看有没有该文件*/
bool lfs_audio_exist(uint8_t index)
{
    char path[64];
    struct lfs_info info;

    /* 序号检查 */
    if (index < 1 || index > AUDIO_MAX_INDEX)
        return false;

    /* 生成路径 */
    snprintf(path, sizeof(path), "%s/%d.wav", AUDIO_DIR, index);

    /* 查询文件 */
    if (lfs_stat(&lfs, path, &info) == 0)
    {
        if (info.type == LFS_TYPE_REG)
            return true;
    }

    return false;
}
/*根据序列号去获取长度*/
int lfs_get_wav_size_by_index(uint8_t index, uint32_t *size)
{
    char path[64];

    if (index < 1 || index > 5)
        return -1;

    snprintf(path, sizeof(path), "/audio/user/%d.wav", index);

    return lfs_get_wav_file_size(path, size);
}
/*根据序列号写入*/
int lfs_audio_write_by_index(uint8_t index,
                             const uint8_t *data,
                             size_t len,
                             audio_write_mode_t mode)
{
    char path[64];
    lfs_file_t file;
    int flags;
    int written;

    if (data == NULL || len == 0)
        return -1;

    /* 序号检查 */
    if (index < 1 || index > AUDIO_MAX_INDEX)
        return -2;

    /* 创建目录（存在不会报错） */
    lfs_mkdir(&lfs, AUDIO_DIR);

    /* 生成路径 */
    snprintf(path, sizeof(path), "%s/%d.wav", AUDIO_DIR, index);

    /* 根据模式选择打开方式 */
    if (mode == AUDIO_WRITE_APPEND)
    {
        flags = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND;
    }
    else
    {
        flags = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
    }

    /* 打开文件 */
    int ret = lfs_file_open(&lfs, &file, path, flags);
    if (ret < 0)
        return -3;

    /* 写入数据 */
    written = lfs_file_write(&lfs, &file, data, len);

    /* 同步到Flash（防掉电） */
    lfs_file_sync(&lfs, &file);

    /* 关闭文件 */
    lfs_file_close(&lfs, &file);

    if (written < 0)
        return -4;

    if ((size_t)written != len)
        return -5;

    return 0;
}
#define FLASH_READ_CHUNK   1024

int flash_copy_wav_to_lfs(uint32_t flash_addr, uint8_t index)
{
    uint32_t file_size;
    uint32_t offset = 0;
    uint8_t buffer[FLASH_READ_CHUNK];
    size_t chunk;
    int ret;

    /* 获取wav文件大小 */
    ret = wav_get_file_size_from_iflash(flash_addr, &file_size);
    if (ret < 0)
        return -1;

    while (offset < file_size)
    {
        chunk = file_size - offset;

        if (chunk > FLASH_READ_CHUNK)
            chunk = FLASH_READ_CHUNK;

        /* 从内部flash读取 */
      //  memcpy(buffer, (uint8_t *)(flash_addr + offset), chunk);
drv_flash_read(OM_FLASH0,flash_addr + offset, buffer, chunk);		
//log_write_with_rotation("/audio/player/p1.wav",audio_data,file_size);
        /* 写入littlefs */
        if (offset == 0)
        {
            /* 第一包覆盖写 */
            ret = lfs_audio_write_by_index(index,
                                           buffer,
                                           chunk,
                                           AUDIO_WRITE_OVERWRITE);
        }
        else
        {
            /* 后续追加写 */
            ret = lfs_audio_write_by_index(index,
                                           buffer,
                                           chunk,
                                           AUDIO_WRITE_APPEND);
        }

        if (ret < 0)
            return -2;

        offset += chunk;
    }

    return 0;
}
/*根据序列号删除*/
int lfs_audio_delete_by_index(uint8_t index)
{
    char path[64];
    struct lfs_info info;

    /* 序号检查 */
    if (index < 1 || index > AUDIO_MAX_INDEX)
        return -1;

    /* 生成路径 */
    snprintf(path, sizeof(path), "%s/%d.wav", AUDIO_DIR, index);

    /* 判断文件是否存在 */
    int ret = lfs_stat(&lfs, path, &info);
    if (ret < 0)
        return -2;   // 文件不存在

    if (info.type != LFS_TYPE_REG)
        return -3;

    /* 删除文件 */
    ret = lfs_remove(&lfs, path);
    if (ret < 0)
        return -4;

    return 0;
}
/**************************************************************************************/
////////////////////////////////////////////////

//不带轮转写
int  lfs_write_log(const char *filename, const char *msg) {
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
		if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			//LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
			
		}
		return -255;
	} 
#endif
		if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
			LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
			return -254;
		}

   lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, filename,
                            LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT);
	if (err < 0) {
        osMutexRelease(lfs_mutex);
        return err;
    }

    int written = lfs_file_write(&lfs, &file, msg, strlen(msg));
    lfs_file_close(&lfs, &file);

    uint32_t now = get_current_unix_time();
    lfs_setattr(&lfs, filename, LFS_ATTR_MTIME, &littlefs_date, sizeof(littlefs_date));

    osMutexRelease(lfs_mutex);
    return written;
	   // 检查当前日志文件大小
	
}
/**
 * @brief 从 LittleFS 文件中读取固定位置的数据
 * @param filename   文件路径
 * @param buffer     读取数据缓冲区
 * @param offset     文件内起始偏移（字节）
 * @param length     要读取的字节数
 * @param mode       FORMAT_STR: 文本模式，自动加'\0'；FORMAT_BIN: 二进制模式
 * @return 实际读取的字节数（>=0 表示成功），<0 表示错误
 */
int lfs_read_file_offset(const char *filename, void *buffer, lfs_off_t offset,
                         lfs_size_t length, ReadFileFormat mode)
{
//	LFS_LOG_DEBUG("lfs_read_file_offset\r\n");
#if (LFS_INIT_IN_TASK_LATER)
    uint32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      0);
    if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
            LFS_LOG_DEBUG(" lfs_read_file_offset wait timeout EXFLASH_INIT_READY flag\r\n");
        } else {
            LFS_LOG_DEBUG("evt wait error code %ld\r\n", result);
        }
        return -255;
    }
#endif

    int bytes_read = 0;
    	   if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }

    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY);
    if (err < 0) {
        osMutexRelease(lfs_mutex);
        return err; // -2: 文件不存在
    }

    //  定位到指定偏移
    lfs_soff_t pos = lfs_file_seek(&lfs, &file, offset, LFS_SEEK_SET);
    if (pos < 0) {
        LFS_LOG_DEBUG("seek failed: %ld\r\n", pos);
        lfs_file_close(&lfs, &file);
        osMutexRelease(lfs_mutex);
        return pos;
    }

    //  按模式读取
    if (mode == FORMAT_STR) {
        // 留1字节给 '\0'
        bytes_read = lfs_file_read(&lfs, &file, buffer, length - 1);
        if (bytes_read < 0) {
            bytes_read = 0;
        }
        ((char *)buffer)[bytes_read] = '\0';
    } else {
        bytes_read = lfs_file_read(&lfs, &file, buffer, length);
    }

    lfs_file_close(&lfs, &file);
    osMutexRelease(lfs_mutex);

    return bytes_read;
}
/*
支持 多线程安全读取文件

从文件 头部开始读取

最多读取 max_len 字节

返回：实际读取字节数（≥0），失败返回负值*/
// mode = 0 → 文本模式（自动补 '\0'）
// mode = 1 → 二进制模式（原始字节，不补 '\0'）
int lfs_read_file(const char *filename, void *buffer, lfs_size_t max_len, ReadFileFormat mode) {
	
//		LFS_LOG_DEBUG("lfs_read_file\r\n");
#if (LFS_INIT_IN_TASK_LATER)
    uint32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);
    if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
            LFS_LOG_DEBUG("lfs_read_file wait timeout EXFLASH_INIT_READY flag\r\n");
        } else {
            LFS_LOG_DEBUG("evt wait error code %ld\r\n", result);
        }
        return -255;
    }
#endif

    int bytes_read = 0;
    	   if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }

    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY);
    if (err < 0) {
        osMutexRelease(lfs_mutex);
        return err; // -2: 文件不存在
    }

    // 从开头读取
    lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET);

    if (mode == FORMAT_STR) { // 文本模式
        // 预留 1 字节给 '\0'
        bytes_read = lfs_file_read(&lfs, &file, buffer, max_len - 1);
        if (bytes_read < 0) {
            bytes_read = 0; // 出错时当作空字符串
        }
        ((char *)buffer)[bytes_read] = '\0';
    } else { // 二进制模式
        bytes_read = lfs_file_read(&lfs, &file, buffer, max_len);
        // 不做 '\0' 处理
    }

    lfs_file_close(&lfs, &file);
    osMutexRelease(lfs_mutex);

    return bytes_read;
}
//读取所有数据
int lfs_read_whole_file(const char *filename, char **out_buf) {
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
			
		}
		return -255;
	} 
#endif
   	   if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }

    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY) < 0) {
        osMutexRelease(lfs_mutex);
        return -1;
    }

    lfs_soff_t size = lfs_file_size(&lfs, &file);
    if (size <= 0) {
        lfs_file_close(&lfs, &file);
        osMutexRelease(lfs_mutex);
        return 0;
    }

    char *buf = pvPortMalloc(size + 1);
    if (!buf) {
        lfs_file_close(&lfs, &file);
        osMutexRelease(lfs_mutex);
        return -2;
    }

    lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET);
    int n = lfs_file_read(&lfs, &file, buf, size);
    buf[n] = '\0'; // 保证结尾

    lfs_file_close(&lfs, &file);

    *out_buf = buf;
	
	osMutexRelease(lfs_mutex);
    return n;
}

static void lfs_list_dir_internal(const char *path, int level) {
    lfs_dir_t dir;
    struct lfs_info info;

    int err = lfs_dir_open(&lfs, &dir, path);
    if (err < 0) {
        LFS_LOG_DEBUG("open dir error: %s, code: %d\r\n", path, err);
        return;
    }

    while (true) {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res <= 0) {
            break; // 读完了或者出错
        }

        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) {
            continue;
        }

        // 打印缩进
        for (int i = 0; i < level; i++) {
            LFS_LOG_DEBUG("  ");
        }

        char fullpath[128];
        if (strcmp(path, "/") == 0) {
            snprintf(fullpath, sizeof(fullpath), "/%s", info.name);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, info.name);
        }

        if (info.type == LFS_TYPE_REG) {
			  // 文件
            time_struct_t littlefs_mtime;
            char time_buf[32] = {0};

            int has_time = lfs_getattr(&lfs, fullpath,
                                       LFS_ATTR_MTIME,
                                       &littlefs_mtime,
                                       sizeof(time_struct_t));

            if (has_time > 0) {
                rtc_to_string(&littlefs_mtime, time_buf, sizeof(time_buf));
                LFS_LOG_DEBUG("[F] %-20s %8lu bytes  date %s\r\n",
                              fullpath,
                              (unsigned long)info.size,
                              time_buf);
            } else {
                LFS_LOG_DEBUG("[F] %-20s %8lu bytes\r\n",
                              fullpath,
                              (unsigned long)info.size);
            }
           // LFS_LOG_DEBUG("[F] %s (%lu bytes)\r\n", fullpath, (unsigned long)info.size);
        } else if (info.type == LFS_TYPE_DIR) {
            LFS_LOG_DEBUG("[D] %s/", fullpath);
			 // 尝试读取目录的 mtime 属性
			time_struct_t mtime;
			char time_str[32] = {0};
			int has_time = lfs_getattr(&lfs, fullpath, LFS_ATTR_MTIME, &mtime, sizeof(mtime));
			if (has_time > 0) {
				rtc_to_string(&mtime, time_str, sizeof(time_str));  // 你已有的格式化函数
				LFS_LOG_DEBUG(" date %s\r\n", time_str);
			} else {
				LFS_LOG_DEBUG("\r\n");
			}
            // 递归进入子目录
            lfs_list_dir_internal(fullpath, level + 1);
        }
    }

    lfs_dir_close(&lfs, &dir);
}

void lfs_list_dir(const char *path) {
#if (LFS_INIT_IN_TASK_LATER)
    uint32_t result = osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
    if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
           log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
        } else {
          log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
        }
        return;
    }
#endif    
    	   if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return;
    }
    LFS_LOG_DEBUG("Listing FS from: %s\r\n", path);
    lfs_list_dir_internal(path, 0);
    osMutexRelease(lfs_mutex);
	
}

// 删除文件
int fs_delete_file(const char *path) {	
	
#if (LFS_INIT_IN_TASK_LATER)
    uint32_t result = osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
    if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
           	log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
        } else {
             log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
        }
        return -254;
    }
#endif  
	if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -255;
    }
    int err = lfs_remove(&lfs, path);
    if (err < 0) {
         osMutexRelease(lfs_mutex);
        // 删除失败，err 为负数错误码
        return err;
    } osMutexRelease(lfs_mutex);
    return 0; // 成功
}
/**
 * @brief 删除指定文件内的一段数据（用后续内容前移覆盖）
 * 
 * @param path 文件路径（含目录）
 * @param offset 起始偏移（字节）
 * @param length 要删除的字节数
 * @return int 0=成功, 负数=错误码
 */
int lfs_delete_bytes(const char *path, lfs_off_t offset, lfs_size_t length) {
	
		LFS_LOG_DEBUG("lfs_delete_bytes\r\n");
#if (LFS_INIT_IN_TASK_LATER)
    uint32_t result = osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
    if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
          	log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
        } else {
             log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
        }
        return -254;
    }
#endif  
    lfs_file_t file;
    int err;
	 if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -255;
    }
    // 打开文件（读写）
    err = lfs_file_open(&lfs, &file, path, LFS_O_RDWR);
    if (err < 0) {
        osMutexRelease(lfs_mutex);
        return err; // 打开失败
    }

    // 获取文件大小
    lfs_soff_t filesize = lfs_file_size(&lfs, &file);
    if (filesize < 0) {
        lfs_file_close(&lfs, &file);
        osMutexRelease(lfs_mutex);
        return 0;
    }

    if (offset >= filesize) {
        lfs_file_close(&lfs, &file);
        osMutexRelease(lfs_mutex);
        return 0; // 偏移超出文件大小
    }

    if (offset + length > filesize) {
        length = filesize - offset; // 删除到末尾
    }

    // 分配缓存（把后面的数据搬上来覆盖要删的区域）
    lfs_size_t remain = filesize - (offset + length);
    uint8_t buffer[128]; // 临时缓存，可调大

    lfs_off_t read_pos = offset + length;
    lfs_off_t write_pos = offset;

    while (remain > 0) {
        lfs_size_t chunk = remain > sizeof(buffer) ? sizeof(buffer) : remain;

        // 读剩余内容
        lfs_file_seek(&lfs, &file, read_pos, LFS_SEEK_SET);
        int r = lfs_file_read(&lfs, &file, buffer, chunk);
         if (r < 0) goto cleanup;

        // 写回覆盖
        lfs_file_seek(&lfs, &file, write_pos, LFS_SEEK_SET);
        int w = lfs_file_write(&lfs, &file, buffer, r);
        if (w < 0) goto cleanup;

        read_pos += r;
        write_pos += r;
        remain   -= r;
    }

    // 截断文件，去掉最后的 length 字节
    err = lfs_file_truncate(&lfs, &file, filesize - length);
    if (err < 0) goto cleanup;
      // 特殊处理空文件
    if (filesize - length == 0) {
        lfs_file_close(&lfs, &file);
        lfs_remove(&lfs, path);
        lfs_file_open(&lfs, &file, path, LFS_O_RDWR | LFS_O_CREAT);
    }
cleanup:
    lfs_file_close(&lfs, &file);
    osMutexRelease(lfs_mutex);
    return err;
}
/**
 * @brief 统计目录下的文件数量（不包括子目录）
 * 
 * @param dir_path 目录路径（含目录）
 * @return int 文件数量，负数为错误码
 */
int lfs_count_files(const char *dir_path)
{
    int count = 0;
    struct lfs_info info;
    lfs_dir_t dir;

#if (LFS_INIT_IN_TASK_LATER)
    // 等待 LFS 准备
    int32_t result = osEventFlagsWait(sys_events,
                                      EVENT_LFS_READY | EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear,
                                      LFS_WAIT_TIMEOUT_MS);
    if (result < 0) {
        LFS_LOG_DEBUG("[LFS] wait flag error: %ld\r\n", result);
        return -1;
    }
#endif

    // 加锁保护
    if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire failed\r\n");
        return -2;
    }

    int ret = lfs_dir_open(&lfs, &dir, dir_path);
    if (ret < 0) {
        osMutexRelease(lfs_mutex);
        return ret;  // -2 目录不存在
    }

    // 遍历目录
    while (true) {
        ret = lfs_dir_read(&lfs, &dir, &info);
        if (ret <= 0) break; // 结束或出错

        if (info.name[0] == 0) continue; // 跳过空项

        // 排除 "." 和 ".."
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
            continue;

        // 只统计文件（非目录）
        if (info.type == LFS_TYPE_REG) {
            count++;
        }
    }

    lfs_dir_close(&lfs, &dir);
    osMutexRelease(lfs_mutex);
    return count;
}

/*
会遍历 /data/ 目录

返回每个日志文件的：

名称

大小

修改时间（mtime，自定义属性）

结果写入你提供的数组 list[] 中

返回实际文件个数
*/
int log_list_files(LogFileInfo *list, int max_files) {
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			  log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			
		}
		return -255;
	} 
#endif
	if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
    if (!list || max_files <= 0) return 0;

    int count = 0;

    lfs_dir_t dir;
    struct lfs_info info;

    if (lfs_dir_open(&lfs, &dir, "/data") == 0) {
        while (lfs_dir_read(&lfs, &dir, &info) > 0) {
            if (info.type != LFS_TYPE_REG) continue;
            if (count >= max_files) break;

            // 拷贝文件信息
            strncpy(list[count].name, info.name, MAX_LOG_NAME_LEN - 1);
            list[count].name[MAX_LOG_NAME_LEN - 1] = '\0';
            list[count].size = info.size;

            // 获取时间戳属性（mtime）
            char fullpath[64];
            snprintf(fullpath, sizeof(fullpath), "/data/%s", info.name);
            uint32_t mtime = 0;
            if (lfs_getattr(&lfs, fullpath, LFS_ATTR_MTIME, &mtime, sizeof(mtime)) > 0) {
                list[count].mtime = mtime;
            } else {
                list[count].mtime = 0;
            }

            count++;
        }
        lfs_dir_close(&lfs, &dir);
    }

    osMutexRelease(lfs_mutex);
    return count;
}
//获取系统时钟
uint32_t get_current_unix_time(void) {
	
	return get_timestamp_date(&littlefs_date);
	
	   // 假设你系统启动时间是 2024-01-01 00:00:00
   // return rtc_get_unix_time();  // 替换为你自己的 RTC 接口
    // 从系统启动后时间估算（tick 转秒）
 //   return osKernelGetTickCount() / 1000;
}
//读取文件时间
// 读取文件的修改时间，成功返回0，mtime通过指针返回；失败返回负值
int lfs_read_file_mtime(const char *filepath, uint32_t *mtime) {
    if (!mtime) return -1;
	time_struct_t s_littlefs_tim;
#if (LFS_INIT_IN_TASK_LATER)
		uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			  log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			
		}
		return -255;
	} 
#endif
	 if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
    int ret = lfs_getattr(&lfs, filepath, LFS_ATTR_MTIME, &s_littlefs_tim, sizeof(time_struct_t));
    if (ret < 0) {
        // 读取失败（文件不存在或没有mtime属性）
		osMutexRelease(lfs_mutex);
        return ret;
    }
	char read_tim[30]={0};
	rtc_to_string(&s_littlefs_tim,read_tim,sizeof(read_tim));
	LFS_LOG_DEBUG("read %s mtime = %s\r\n",filepath,read_tim);
	  osMutexRelease(lfs_mutex);
    return 0;
}

int log_write_json(const char *filename,LogLevel level, const char *msg) {
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			  log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			
		}
		return -255;
	} 
#endif
    const char *level_str[] = {"INFO", "WARN", "ERROR"};
    if (level < 0 || level > 2) return -1;

    char line[256];
	char str_tim[30]={0};
    uint32_t now = get_current_unix_time();
	rtc_to_string(&littlefs_date,str_tim,sizeof(str_tim));
    // 组装 JSON 字符串（单行）
    snprintf(line, sizeof(line),
             "{\"time\":%s, \"level\":\"%s\", \"msg\":\"%s\"}\r\n",
             str_tim, level_str[level], msg);

    return log_write_with_rotation(filename,line,strlen(line));  // 使用已有的轮换日志写入
}
/*
*删除dir目录下的prefix前缀文件，例如：lfs_delete_by_prefix("/data", "log.");
*/
int lfs_delete_by_prefix(const char *dir, const char *prefix) {
    lfs_dir_t dir_iter;
    struct lfs_info info;
    char files_to_delete[20][64]; // 最多20个匹配文件，每个文件名64字节
    int match_count = 0;
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			  log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			
		}
		return -255;
	} 
#endif
	if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
	
    // 打开目录
    if (lfs_dir_open(&lfs, &dir_iter, dir) < 0) {
		osMutexRelease(lfs_mutex);
        return -1;
    }

    // 先收集匹配文件名
    while (lfs_dir_read(&lfs, &dir_iter, &info) > 0) {
        if (info.type != LFS_TYPE_REG) continue;
        if (strncmp(info.name, prefix, strlen(prefix)) == 0) {
            snprintf(files_to_delete[match_count], sizeof(files_to_delete[match_count]),
                     "%s/%s", dir, info.name);
            match_count++;
        }
    }
    lfs_dir_close(&lfs, &dir_iter);

    // 再统一删除
    int deleted = 0;
    for (int i = 0; i < match_count; i++) {
        if (lfs_remove(&lfs, files_to_delete[i]) == 0) {
            deleted++;
        }
    }  
	osMutexRelease(lfs_mutex);
    return deleted;
}
/*
dir：目录路径（例如 "/logs"）

threshold_time：阈值时间（Unix 时间戳，单位：秒）

如果文件的修改时间 < threshold_time，则被删除

返回值

>= 0：成功删除的文件数量
< 0：失败（比如目录不存在）
*/
int lfs_delete_older_than(const char *dir, time_struct_t threshold_time) {
    lfs_dir_t dir_iter;
    struct lfs_info info;
    int deleted = 0;
    char fullpath[128];
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("evt wait error error code %ld\r\n", result);
			
		}
		return -255;
	} 
#endif
	if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
		log_debug("[LFS] mutex acquire timeout!:%s:%d\r\n",__FILE__,__LINE__);
   //     LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
	
    if (lfs_dir_open(&lfs, &dir_iter, dir) < 0) return -1;

    while (lfs_dir_read(&lfs, &dir_iter, &info) > 0) {
        if (info.type != LFS_TYPE_REG) continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, info.name);

        uint32_t mtime = 0;
		time_struct_t s_littlefs_tim;
        int ret = lfs_getattr(&lfs, fullpath, 0x01, &s_littlefs_tim, sizeof(time_struct_t));
		
	//	tim_comper(s_littlefs_tim,threshold_time);
        if (ret >= 0 && tim_comper(s_littlefs_tim,threshold_time)<0) {
            lfs_remove(&lfs, fullpath);
            deleted++;
        }
    }

    lfs_dir_close(&lfs, &dir_iter);
	 osMutexRelease(lfs_mutex);
    return deleted;
}/**
 * @brief 安全删除指定文件（如果存在）
 * @param path 文件完整路径，如 "/data/log.txt"
 * @return 0: 成功删除或文件不存在；<0: 删除失败（如 I/O 错误）
 */
int lfs_remove_file_if_exists(const char *path) {
    struct lfs_info info;
#if (LFS_INIT_IN_TASK_LATER)
	uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			  log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			
		}
		return -255;
	} 
#endif
	if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
    // 判断文件是否存在
    int ret = lfs_stat(&lfs, path, &info);
    if (ret < 0) {
        if (ret == LFS_ERR_NOENT) {	 
			osMutexRelease(lfs_mutex);
            return 0;  // 文件不存在，不算错误
        }	
		osMutexRelease(lfs_mutex);
        return ret;    // 其他错误
    }
	 osMutexRelease(lfs_mutex);
    // 删除文件
    return lfs_remove(&lfs, path);
}

/**
 * 判断文件是否存在且为空
 * @param lfs LittleFS实例指针
 * @param path 文件路径
 * @return 0:文件不存在, 1:文件存在且为空, 2:文件存在但不为空, -1:错误
 */
int lfs_file_exists_and_empty( const char *path) {
    struct lfs_info info;
    lfs_file_t file;
    int ret;
#if (LFS_INIT_IN_TASK_LATER)
    uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
		if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
			//LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
			  log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			
		}
		return -255;
	} 
#endif
		  if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
    // 1. 检查文件是否存在
    ret = lfs_stat(&lfs, path, &info);
    if (ret < 0) {
        if (ret == LFS_ERR_NOENT) {
            // 文件不存在
			   osMutexRelease(lfs_mutex);
            return 0;
        }
        // 其他错误
        LFS_LOG_DEBUG("lfs_stat failed for %s: %d", path, ret);
		   osMutexRelease(lfs_mutex);
        return -1;
    }
    
    // 2. 检查是否为文件（不是目录）
    if (info.type != LFS_TYPE_REG) {
        LFS_LOG_DEBUG("%s is not a regular file", path);
		   osMutexRelease(lfs_mutex);
        return -1;
    }
    
    // 3. 检查文件大小
    if (info.size == 0) {
        // 文件存在且大小为0，直接返回
		   osMutexRelease(lfs_mutex);
        return 1;
    }
    
    // 4. 进一步验证：打开文件检查内容
    ret = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (ret < 0) {
        LFS_LOG_DEBUG("lfs_file_open failed for %s: %d", path, ret);
		   osMutexRelease(lfs_mutex);
        return -1;
    }
    
    // 尝试读取一个字节来确认文件是否真的为空
    uint8_t test_byte;
    lfs_ssize_t bytes_read = lfs_file_read(&lfs, &file, &test_byte, 1);
    
    lfs_file_close(&lfs, &file);
    
    if (bytes_read == 0) {
        // 文件存在但读取不到数据（实际为空）
		   osMutexRelease(lfs_mutex);
        return 1;
    }
       osMutexRelease(lfs_mutex);
    // 文件存在且不为空
    return 2;
}

/**
 * 清空文件内容但不删除文件
 * @param path 文件路径
 * @return 0成功，其他失败
 */
int lfs_clear_file_content(const char *path) {
    lfs_file_t file;
    int ret;
	LFS_LOG_DEBUG("lfs_clear_file_content\r\n");
    #if (LFS_INIT_IN_TASK_LATER)
    uint32_t result = osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY,
                                      osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
    if (result & osFlagsError) {
        if (result == osFlagsErrorTimeout) {
          	log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
        } else {
              log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
        }
        return -254;
    }
#endif  
	 if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -255;
    }
	
	
    // 以读写方式打开文件
    ret = lfs_file_open(&lfs, &file, path, LFS_O_RDWR | LFS_O_CREAT);
    if (ret < 0) {
        LFS_LOG_DEBUG("Failed to open file: %s, error: %d", path, ret);
		 osMutexRelease(lfs_mutex);
        return ret;
    }
    
    // 截断文件大小为0（清空内容）
    ret = lfs_file_truncate(&lfs, &file, 0);
    if (ret < 0) {
        LFS_LOG_DEBUG("Failed to truncate file: %s, error: %d", path, ret);
        lfs_file_close(&lfs, &file);
		 osMutexRelease(lfs_mutex);
        return ret;
    }
    
    // 确保更改写入存储
    ret = lfs_file_sync(&lfs, &file);
    if (ret < 0) {
        LFS_LOG_DEBUG("Failed to sync file: %s, error: %d", path, ret);
    }
    
    lfs_file_close(&lfs, &file);
	 osMutexRelease(lfs_mutex);
    return ret;
}

int lfs_delete_all_in_dir(const char *dir) {
    lfs_dir_t dir_iter;
    struct lfs_info info;
    char fullpath[128];
    int deleted = 0;
#if (LFS_INIT_IN_TASK_LATER)
uint32_t result =osEventFlagsWait(sys_events, EVENT_LFS_READY|EXFLASH_INIT_READY, osFlagsWaitAll | osFlagsNoClear, LFS_WAIT_TIMEOUT_MS);
	if (result & osFlagsError) {
		if (result == osFlagsErrorTimeout) {
			log_debug("LFS TIMEOUT%s:%d\r\n",__FILE__,__LINE__);
		//	LFS_LOG_DEBUG("wait timeoutEXFLASH_INIT_READY flag\r\n");
		} else {
		  log_debug("LFS ERROR CODE:%d,%s:%d\r\n",result,__FILE__,__LINE__);
			
		}
		return -255;
	} 
#endif
	 if (osMutexAcquire(lfs_mutex, LFS_MUTEX_TIMEOUT_MS) != osOK) {
        LFS_LOG_DEBUG("[LFS] mutex acquire timeout!\r\n");
        return -254;
    }
    // 打开目录
    if (lfs_dir_open(&lfs, &dir_iter, dir) < 0) {
		osMutexRelease(lfs_mutex);
        return -1; // 目录不存在或打开失败
    }

    // 遍历目录
    while (lfs_dir_read(&lfs, &dir_iter, &info) > 0) {
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) {
            continue; // 跳过特殊目录
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, info.name);

        if (info.type == LFS_TYPE_REG) {
            // 删除文件
            if (lfs_remove(&lfs, fullpath) == 0) {
                deleted++;
            }
        }
        else if (info.type == LFS_TYPE_DIR) {
            // 如果需要递归删除子目录文件，可以调用自己
            // deleted += lfs_delete_all_in_dir(fullpath);
            // 删除空子目录
            // lfs_remove(&lfs, fullpath);
        }
    }

    lfs_dir_close(&lfs, &dir_iter);
	osMutexRelease(lfs_mutex);
    return deleted; // 返回删除的文件数量
}


#if (TEST_TASK_LITTLEFS)
static void vLittleFsScheduleTask(void *argument)
{
	//osEventLittleFsFlagsInit();
//	LFS_LOG_DEBUG("vLittleFsScheduleTask\r\n");
	lfs_init();
	//user_exflash_quad_init();
//	lfs_port_test2();
//	char testdf[100]={0};
//	uint8_t tttttyyyy[500]={0};
//	for(int i =0;i<100;i++)
//	{
//		testdf[i]=i+1;
//	}
//	//	lfs_read_file("/data/log.txt",tttttyyyy,(lfs_size_t)200);
//	//LFS_LOG_DEBUG_ARRAY_EX("read_data = ",tttttyyyy,200);
//	RecordLastLogFileInfo latest;
////	
////			 	log_write_with_rotation("/data/log.txt", testdf);	
////	//读目录列表
////			 lfs_list_dir("/data");
//	/*
//	lfs_list_dir("/data");
//	
//	log_write_with_rotation("/data/log.txt", testdf);
//	
//	
//	 lfs_list_dir("/data");
//	int tmprr= get_latest_log_file(&latest);
//if ( tmprr== 0) {
//   LFS_LOG_DEBUG("file = : %s\r\n", latest.name);
//}
//else
//{
//	LFS_LOG_DEBUG("get_latest_log_file error = %d\r\n",tmprr);
//}
//	int count = lfs_delete_all_in_dir("/data");
//LFS_LOG_DEBUG("dell %d num file\r\n", count);
//	
//	lfs_list_dir("/data");*/
//	
//	/*
//	int count = lfs_delete_all_in_dir("/data");
//	
//	LFS_LOG_DEBUG("dell %d num file\r\n", count);
//	
//	lfs_list_dir("/data");
//	
//	log_write_with_rotation("/data/log.txt", testdf,100);

//	lfs_list_dir("/data");

//	lfs_read_file("/data/log.txt",tttttyyyy,(lfs_size_t)109);
//LFS_LOG_DEBUG_ARRAY_EX("read_data = ",tttttyyyy,109);*/


//littlefs_power_loss_auto_test();
//verify_lfs_after_reset();
//test_lfs_powercut();
	while (1) 
	{		
		osDelay(5000);
		// log_write_with_rotation("/data/log.txt", testdf);
	
	//读目录列表
		
		//lfs_read_file("/data/log.txt",tttttyyyy,(lfs_size_t)100);
	//LFS_LOG_DEBUG_ARRAY_EX("read_data = ",tttttyyyy,100);

	}
}
void vStartLittleFsTask(void)
{
    const osThreadAttr_t littleFsThreadAttr = {
        .name = "LittleFs",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 8192,
        .priority = osPriorityNone,
        .tz_module = 0,
    };

    // Create ble Task
   osThreadId_t thread_id =  osThreadNew(vLittleFsScheduleTask, NULL, &littleFsThreadAttr);
	if (thread_id == NULL) {
    // 线程创建失败处理
    LFS_LOG_DEBUG("osThreadId_t error =%d\r\n",thread_id);
		 LFS_LOG_DEBUG("heap: %d bytes\r\n", xPortGetFreeHeapSize());
    LFS_LOG_DEBUG("stack: %d\r\n", osThreadGetCount());
    // 可以添加错误恢复逻辑，如释放资源或重启系统
} else {
    // 线程创建成功
    LFS_LOG_DEBUG("osThreadId_t finish: %p\r\n", thread_id);
}
}
#else

#endif
