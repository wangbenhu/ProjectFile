#ifndef __FLASH_H
#define __FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/*
Q01--代表01号程序，例：Q02代表02号程序
24--代表2024年，简写24, 2029年简写29
0841--代表月日08月01日，在01日+40=41日
 001--代表序列号,该序列号到200为止，序号201就写不进
 */
#define USER_NAME_MAX 11//例子01240841001//默认初始值为FFFFFFFFFFF



#define USER_CUSTOM_DATA_FLASH 		(0X08000000|0X0003F800)//254k
#define PAGE_SPACE_MAX						(2*1024)//B





enum FLASH_ERROR_CODE{
	FLASH_WRITE_OK		=0,
	ERROR_ID					=1,
	ERROR_WRITE_LEN		=2,
	ERROR_ERASE_FLASH =3,
	
	
	ERROR_END		 ,
};
enum FLASH_DATA_ID{
	Flash_User_Name_ID 			= 0,	
	
	Flash_END_ID,
	
	Flash_MAX_ID						=255,
};

typedef struct MODE_DATA
{
	uint8_t USER_NAME[USER_NAME_MAX];//BLE NAME
	
}USER_DATA;

uint8_t FLASH_EEPROM_Write_test(uint8_t *write_data,uint32_t write_data_len,uint8_t id);
uint8_t FLASH_EEPROM_Read(uint8_t *read_data,uint32_t read_data_len,uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

