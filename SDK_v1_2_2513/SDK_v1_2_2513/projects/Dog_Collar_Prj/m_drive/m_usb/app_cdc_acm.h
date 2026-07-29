#ifndef __APP_CDC_ACM_H
#define __APP_CDC_ACM_H


#include <stdint.h>
void usbd_cdc_acm_init(void);
void usbd_cdc_acm_data_send(const char *pdata, uint32_t length);


#endif 
