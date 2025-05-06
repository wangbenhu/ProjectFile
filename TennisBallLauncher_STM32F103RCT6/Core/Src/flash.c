#include <stdio.h>
#include "flash.h"
#include  <stdlib.h>
#include  <string.h>
/*main.c中的代码*/


USER_DATA USER_DATA_SET;

/**
  * @brief  在FLASH中存储变量值，目前并不清楚这个程序有多大，能写到多少页，先定义在第127页中
  * @param  用STM32中FLASH存储空间模拟EEPROM的读写
  * @retval 参数：写入要存储的值
  */
	
	FLASH_EraseInitTypeDef EraseInitStruct={
		FLASH_TYPEERASE_PAGES,
		FLASH_BANK_1,
		USER_CUSTOM_DATA_FLASH,
		1,
	};
uint8_t FLASH_EEPROM_Write_test(uint8_t *write_data,uint32_t write_data_len,uint8_t id)
{
	uint8_t return_daata = FLASH_WRITE_OK;
	uint32_t tmp_data=0;
	uint8_t count_tmp=4;
	int i;
	uint32_t write_data_bit=0;
	RETURN_ERROR:
	if(return_daata!=FLASH_WRITE_OK)
	{
		return return_daata;
	}
	if(write_data_len>PAGE_SPACE_MAX)
		{
			return_daata = ERROR_WRITE_LEN;
			goto RETURN_ERROR;
	}
	if(id<Flash_User_Name_ID || id>Flash_END_ID)
	{
					return_daata = ERROR_ID;
			goto RETURN_ERROR;
	}
		
    HAL_FLASH_Unlock();     //解锁
	
	
    uint32_t PageError = 0;
    if (HAL_FLASHEx_Erase(&EraseInitStruct,&PageError) == HAL_OK) //如果结构体中的起始地址USER_CUSTOM_DATA_FLASH，这一页的数据擦除成功，返回OK
    {
      // debug_uart_printf("擦除 成功\r\n");
    }
		else{
			return_daata = ERROR_ERASE_FLASH;
			goto RETURN_ERROR;
		}

		switch(id)
		{
			case Flash_User_Name_ID:
			{
				for(i=0;i<write_data_len;i++)
				{
					
					tmp_data|=(write_data[i]<<((4-count_tmp)*8));
					count_tmp--;
					if(count_tmp<=0)
					{
						
						HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,USER_CUSTOM_DATA_FLASH+write_data_bit++*4, tmp_data); //向FLASH中写入
							count_tmp=4;
						tmp_data=0;
					}						
				}
				
			}
			if(count_tmp!=4)
			{
					HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,USER_CUSTOM_DATA_FLASH+write_data_bit++*4, tmp_data); //向FLASH中写入
				count_tmp=4;
			}
			break;
			default:
				break;
		}
    HAL_FLASH_Lock();
		
		return return_daata;
}


/**
  * @brief  读出存储地址中的内容
  * @param  用STM32中FLASH存储空间模拟EEPROM的读写
  * @retval 返回值：从FLASH中读出数据
  */
uint8_t FLASH_EEPROM_Read(uint8_t *read_data,uint32_t read_data_len,uint32_t addr)
{
    HAL_FLASH_Unlock();
	uint32_t tmp=0; 
	uint32_t Page = 0;
	uint32_t read_bit_count=0;
	for(int i=0;i<read_data_len;i+=4)
	{ 
		Page=*(__IO uint32_t*)(addr+(read_bit_count++*4));
		
		read_data[tmp++]=Page;
		read_data[tmp++]=Page>>8;
		read_data[tmp++]=Page>>16;
		read_data[tmp++]=Page>>24;
		if(read_data_len-i<4)
		{
			Page=*(__IO uint32_t*)(addr+(read_bit_count++*4));
			for(int j=0;j<read_data_len-i;j++)
			{
				read_data[tmp++]=Page>>(j*8);
			}
			
			break;
		}
	}  
    return 0;
}



