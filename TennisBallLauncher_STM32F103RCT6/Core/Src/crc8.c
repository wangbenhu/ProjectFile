#include "crc8.h"
#include "usart.h"

//校验值计算
uint8_t CheckSum_Count_Get(uint8_t *data,uint32_t len)
{
	uint8_t return_check_sum=0;
	for(int i=0;i<len;i++)
	{
		return_check_sum+=data[i];
		//debug_uart_printf(" %x ",return_check_sum);
	}
	return_check_sum ^= CHECKSUM_FACTOR;
	return return_check_sum;
}