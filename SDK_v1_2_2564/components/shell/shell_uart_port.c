#include "shell_uart_port.h"
#include "shell.h"
#include "common_def.h"
extern void test_sn_set(char *sn,uint16_t len);
extern void evt_app_adv_stop(void);
extern void evt_app_adv_start(void);
extern void test_auth_code_set(char *auth_code,uint16_t len);

extern void ltepower_test(void);

extern void product_set_addr(uint8_t *address);
extern void user_initiative_reboot_fun(void);
extern int string_mac_to_bytes(const char *mac_str, uint8_t *mac_buf);
 #define CRC32_FRAME_SIZE_4K 4096

static uint32_t motion_dfu_crc32(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc)
{
    uint32_t crc;
    crc = (p_crc == NULL) ? 0xFFFFFFFF : ~(*p_crc);
    for (uint32_t i = 0; i < size; i++){
        crc = crc ^ p_data[i];
        for (uint32_t j = 8; j > 0; j--) crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
    }
    return ~crc;
}
/**
 * @brief 计算内部 Flash 指定区域的 CRC32
 * 
 * @param start_addr  起始地址 (相对于 Flash 基地址的偏移)
 * @param length      总长度 (字节)
 * @return uint32_t   计算得到的 CRC32 值
 */
uint32_t calculate_flash_crc(uint32_t start_addr, uint32_t length)
{
    uint8_t read_buffer[CRC32_FRAME_SIZE_4K]; // 4KB 临时缓冲区
    uint32_t current_crc = 0xFFFFFFFF;  // CRC 初始值
    uint32_t bytes_read = 0;            // 当前实际读取的字节数
    uint32_t current_addr = start_addr; // 当前读取地址指针
    uint32_t remaining_len = length;    // 剩余需要处理的长度

    // 循环读取 Flash，直到处理完所有数据
    while (remaining_len > 0)
    {
        // 确定本次读取的大小：
        // 如果剩余长度大于 4K，则读 4K；否则读取剩余的字节数
        uint32_t read_size = (remaining_len > CRC32_FRAME_SIZE_4K) ? CRC32_FRAME_SIZE_4K : remaining_len;

        // 从 Flash 读取数据到缓冲区
        // 注意：确保 drv_flash_read 返回的是实际读取的字节数，或者处理其返回值以确保读取成功
        bytes_read = drv_flash_read(OM_FLASH0, current_addr, read_buffer, read_size);
        // 简单的错误检查：如果读取失败（返回0或其他非预期值），应中断
        if (bytes_read != 0) {
            // 这里可以根据实际情况添加错误处理，例如返回 0 或 -1
            break; 
        }

        // 计算当前缓冲区内数据的 CRC
        // 注意：传入 &current_crc 以进行增量计算
        current_crc = motion_dfu_crc32(read_buffer, read_size, &current_crc);

        // 更新地址和剩余长度
        current_addr += read_size;
        remaining_len -= read_size;
    }

    return current_crc;
}

static void cmd_shell_uart_tx_mac(int argc, char *argv[])
{
	uint8_t ble_mac_bytes[6] = {0};
	uint8_t mac_len = strlen(argv[1]);
	if(mac_len == 12)
	{
		bool all_zero = true;
		bool all_ff = true;
		bool char_error = false;
		for (int i = 0; i < mac_len; i++) {
			
		//	log_debug("2 = %c %d %d \r\n",all_zero,all_ff,char_error);
			if (argv[1][i] != '0') {
				all_zero = false;
			}
			if (argv[1][i] != 'F') {
				all_ff = false;
			}
			if(!((argv[1][i] >= '0'&& argv[1][i]<='9')
				|| (argv[1][i] >= 'a'&& argv[1][i]<='f')
					|| (argv[1][i] >= 'A'&& argv[1][i]<='F')))
			{
				char_error = true;
			}
		}
		if (all_zero || all_ff || char_error) {
			 // 全0或全F表示无效
			DRV_DELAY_MS(50);
			char send_error[]="MAC_SET_ERROR!!!\r\n";
			drv_uart_write(OM_UART0,(uint8_t *)send_error,strlen(send_error),10);
			DRV_DELAY_MS(50);
		}
		
		string_mac_to_bytes(argv[1],ble_mac_bytes);
		product_set_addr(ble_mac_bytes);
		DRV_DELAY_MS(50);
		char send_buffer[]="MAC_SET_OK!!!\r\n";
		drv_uart_write(OM_UART0,(uint8_t *)send_buffer,strlen(send_buffer),10);
		DRV_DELAY_MS(50);
		user_initiative_reboot_fun();
	}
	else
	{DRV_DELAY_MS(50);
		char send_error[]="MAC_SET_ERROR!!!\r\n";
		drv_uart_write(OM_UART0,(uint8_t *)send_error,strlen(send_error),10);
		DRV_DELAY_MS(50);
	}
	
	
//log_debug("cmd_shell_uart_tx_mac = %d %s %d\r\n",mac_len,argv[1],strlen(argv[1]));
//	for(int i=0;i<6;i++)
//	{
//		log_debug("%02x ",ble_mac_bytes[i]);
//	}

}
extern uint8_t Audio_IFlash_Play_Start(uint8_t index,uint8_t audio_reset);


static void cmd_shell_crc32_get(int argc, char *argv[])
{
	uint32_t crc32_vlue = 0;
	log_debug("CRC32_GET start\r\n");
	//crc32_vlue = calculate_flash_crc(0x4000,460*1024);
	log_debug("CRC32_GET = %#X\r\n",crc32_vlue);
}
static void cmd_shell_audio_play(int argc, char *argv[])
{
	log_debug("cmd_shell_audio_play\r\n");

	Audio_IFlash_Play_Start((argv[1][0]-'0'),1);
}
static void cmd_shell_uart_tx_ltetest(int argc, char *argv[])
{
//	log_debug("cmd_shell_uart_tx_ltetest\r\n");
    if (argc < 1) {
     //   log_debug("USBD: invalid params\r\n");
        return;
    }
//	ltepower_test();
}

static void cmd_shell_uart_tx_auth_code(int argc, char *argv[])
{
	log_debug("cmd_shell_uart_tx_sn\r\n");
    if (argc < 1) {
        log_debug("USBD: invalid params\r\n");
        return;
    }
	test_auth_code_set(argv[1],strlen(argv[1]));
}
static void cmd_shell_uart_tx_wifi_mac(int argc, char *argv[])
{
	log_debug("cmd_shell_uart_tx_sn\r\n");
    if (argc < 1) {
        log_debug("USBD: invalid params\r\n");
        return;
    }
	test_sn_set(argv[1],strlen(argv[1]));
}
static void cmd_shell_uart_tx_gps_addr(int argc, char *argv[])
{
	log_debug("cmd_shell_uart_tx_sn\r\n");
    if (argc < 1) {
        log_debug("USBD: invalid params\r\n");
        return;
    }
	test_sn_set(argv[1],strlen(argv[1]));
}
static void cmd_shell_uart_tx_ble(int argc, char *argv[])
{
	log_debug("cmd_shell_uart_tx_ble_stop = %c %d\r\n",argv[1][0],argc);
    if (argc < 1) {
        log_debug("USBD: invalid params\r\n");
        return;
    }
	if(argv[1][0]=='1')
	{
		evt_app_adv_start();
	}
	if(argv[1][0]=='0')
	{
		evt_app_adv_stop();
	}
	
}

static void cmd_shell_uart_tx_sn(int argc, char *argv[])
{
	log_debug("cmd_shell_uart_tx_sn\r\n");
    if (argc < 1) {
        log_debug("USBD: invalid params\r\n");
        return;
    }
	test_sn_set(argv[1],strlen(argv[1]));
}
static void cmd_shell_uart_tx_Battery(int argc, char *argv[])
{
    if (strcmp(argv[0], "set") == 0) {
        if (argc < 1) {
            log_debug("USBD: invalid params\r\n");
            return;
        }
		uint8_t tese=0;
		tese = strtod(argv[1],NULL);
		SetModePare_SetBattery(tese);
		log_debug("cmd_shell_uart_tx = %d %s %d\r\n",argc,argv[1],tese);
      //  drv_uart_write(OM_UART1, (uint8_t *)argv[1], strlen(argv[1]), 200);
      //  usbd_cdc_acm_data_send(argv[1], strlen(argv[1]));
    } else {
        log_debug("USBD: invalid command\r\n");
    }
}
static void cmd_shell_uart_tx_charge(int argc, char *argv[])
{
    if (strcmp(argv[0], "set") == 0) {
        if (argc < 1) {
            log_debug("USBD: invalid params\r\n");
            return;
        }
		uint8_t tese=0;
		tese = strtod(argv[1],NULL);
		SetModePare_SetCharge(tese);
		log_debug("cmd_shell_uart_tx = %d %s %f\r\n",argc,argv[1],tese);
      //  drv_uart_write(OM_UART1, (uint8_t *)argv[1], strlen(argv[1]), 200);
      //  usbd_cdc_acm_data_send(argv[1], strlen(argv[1]));
    } else {
        log_debug("USBD: invalid command\r\n");
    }
}
static void cmd_shell_uart_tx_user(int argc, char *argv[])
{
    if (strcmp(argv[0], "set") == 0) {
        if (argc < 1) {
            log_debug("USBD: invalid params\r\n");
            return;
        }
		uint8_t tese=0;
		tese = strtod(argv[1],NULL);
		SetModePare_SetUser(tese);
		log_debug("cmd_shell_uart_tx = %d %s %f\r\n",argc,argv[1],tese);
      //  drv_uart_write(OM_UART1, (uint8_t *)argv[1], strlen(argv[1]), 200);
      //  usbd_cdc_acm_data_send(argv[1], strlen(argv[1]));
    } else {
        log_debug("USBD: invalid command\r\n");
    }
}

static void cmd_shell_mode_set(int argc, char *argv[])
{
	 if (strcmp(argv[0], "set") == 0) {
        if (argc < 1) {
            log_debug("USBD: invalid params\r\n");
            return;
        }
		uint8_t tese=0;
		tese = strtod(argv[1],NULL);
		SetModePare_SetBattery(tese);
		log_debug("cmd_shell_uart_tx = %d %s %d\r\n",argc,argv[1],tese);
      //  drv_uart_write(OM_UART1, (uint8_t *)argv[1], strlen(argv[1]), 200);
      //  usbd_cdc_acm_data_send(argv[1], strlen(argv[1]));
    } else {
        log_debug("USBD: invalid command\r\n");
    }
}

static void cmd_shell_sport_set(int argc, char *argv[])
{
	 if (strcmp(argv[0], "set") == 0) {
        if (argc < 1) {
            log_debug("USBD: invalid params\r\n");
            return;
        }
		uint8_t tese=0;
		tese = strtod(argv[1],NULL);
		SetModePare_SetBattery(tese);
		log_debug("cmd_shell_uart_tx = %d %s %d\r\n",argc,argv[1],tese);
      //  drv_uart_write(OM_UART1, (uint8_t *)argv[1], strlen(argv[1]), 200);
      //  usbd_cdc_acm_data_send(argv[1], strlen(argv[1]));
    } else {
        log_debug("USBD: invalid command\r\n");
    }
}
const shell_cmd_t shell_uart_cmd[] = {
    { "battery",     	cmd_shell_uart_tx_Battery,  "set <0-100>" },
    { "charge",     	cmd_shell_uart_tx_charge,  "set <0~3>" },
    { "user",     		cmd_shell_uart_tx_user,  "set <0~2>" },
    { "sn",     		cmd_shell_uart_tx_sn,  "set <sn>" },
	{ "auth_code",     	cmd_shell_uart_tx_auth_code,  "set <sn>" },
	{ "wifi_mac",     	cmd_shell_uart_tx_wifi_mac,  "set <sn>" },
	{ "gps_addr",     	cmd_shell_uart_tx_gps_addr,  "set <sn>" },
	{ "ble",     	cmd_shell_uart_tx_ble,  "set <sn>" },
	{ "lte",     	cmd_shell_uart_tx_ltetest,  "set <sn>" },
	{ "ble_mac",     	cmd_shell_uart_tx_mac,  "set <MAC>" },
	{ "audio_play",     	cmd_shell_audio_play,  "set <audio>" },
	{ "soft_crc32",     	cmd_shell_crc32_get,  "get" },
	
	//CJX
	{ "mode change",     	cmd_shell_mode_set,  "set <mode>" },
	{ "sport change",     	cmd_shell_sport_set,  "set <sport>" },
	
    { NULL,      NULL,               NULL},     /* donot deleted */
};
void shell_uart_init(void)
{
	shell_init(shell_uart_cmd);
}