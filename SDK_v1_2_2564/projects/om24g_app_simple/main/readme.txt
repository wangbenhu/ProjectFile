
为了优化接收端对包的处理速度，防止发射端发包频率过快，导致接收端出现新包覆盖旧包问题。24G驱动改用双buffer模式，定义om24g_rx_payload[64]数组大小时，
必须为此应用程序最大接收字节的两倍及以上。

例如：业务代码最大需要接收32字节的数据， om24g_rx_payload大小就必须大于等于64bytes.因为接收数据时，驱动会将第一包数据先给前32字节，第二包数据给后
32字节，交替接收。

中断模式接收：
若调用中断函数om24g_read_int(uint8_t *rx_payload, uint16_t max_rx_num)收包，第二个参数必须为业务代码最大需要接收的数据大小32。客户应用程序不需要关
心对两者的交替接收，驱动会将两包数据均传递给同一个形参指针，只需从回调函数om24g_callback(void *om_reg, drv_event_t drv_event, void *buff, void *num)
形参指针*buff获取接收到的数据即可，中断模式不可以直接操作om24g_rx_payload取数。具体参考om24g_app_simple 工程。

轮询模式接收：
若调用轮询函数om24g_read(uint8_t *rx_payload, uint32_t timeout_ms)接收数据，可以直接操作om24g_rx_payload取数。轮询收完一包会关掉射频，不会出现上述包覆盖的问题。