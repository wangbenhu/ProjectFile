/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    FLASH driver
 * @details  FLASH driver
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_FLASH1)
#include "om_device.h"
#include "om_driver.h"
#include <stddef.h>


/*******************************************************************************
 * MACROS
 */
#if RTE_FLASH1_XIP
#define __OFLASH_CODE   __RAM_CODE
#else
#define __OFLASH_CODE
#endif

#define M_SPI           0 /* Do not modify, must be 0 */
#define M_QPI           1 /* Do not modify, must be 1 */
#define M_3BA           0 /* Do not modify, must be 0 */
#define M_4BA           1 /* Do not modify, must be 1 */
#define XBUS(xpi)       ((xpi == M_QPI) ? BUS_4BIT : BUS_1BIT)
#define FCFG            OSPI_FRAME_CONFIG


#define CMD_FRAME_SET(pframe, cfg)                                  \
    do {                                                            \
        *(flash_frame_t *)(pframe) = (flash_frame_t)cfg;            \
    } while (0)

/*
 * This is a list of commands used in flash operations.
 * parameter1 is used for SPI mode or QPI mode.
 * parameter2 is used for 3 bytes address or 4 bytes address.
 * parameter3 is used for dummy cycles, only for QPI read commands.
 * Parameters instruction:
 *   NA: Not applicable, it is not effective.
 *   xpi: if this parameter is M_QPI, it means the command is used in QPI mode,
 *       and if this parameter is M_SPI, it means the command is used in SPI mode,
 *       do not use any other value.
 *   xba: the address type of command, if this parameter is M_3BA, it means the address is 3 bytes,
 *       if this parameter is M_4BA, it means the address is 4 bytes.
 *   xdmy: the dummy cycles of command, only for QPI read commands.
 */
// Read cmd for SPI mode
#define FLASH_READ_CFG(xba)                 {0x03U, FCFG(BUS_1BIT, 8, BUS_1BIT, (24 + 8 * xba), 0, BUS_1BIT)}
#define FLASH_FAST_READ_CFG(xba)            {0x0BU, FCFG(BUS_1BIT, 8, BUS_1BIT, (24 + 8 * xba), 8, BUS_1BIT)}
#define FLASH_FAST_READ_DO_CFG(xba)         {0x3BU, FCFG(BUS_1BIT, 8, BUS_1BIT, (24 + 8 * xba), 8, BUS_2BIT)}
#define FLASH_FAST_READ_DIO_CFG(xba)        {0xBBU, FCFG(BUS_1BIT, 8, BUS_2BIT, (32 + 8 * xba), 0, BUS_2BIT)}
#define FLASH_FAST_READ_QO_CFG(xba)         {0x6BU, FCFG(BUS_1BIT, 8, BUS_1BIT, (24 + 8 * xba), 8, BUS_4BIT)}
#define FLASH_FAST_READ_QIO_CFG(xba)        {0xEBU, FCFG(BUS_1BIT, 8, BUS_4BIT, (32 + 8 * xba), 4, BUS_4BIT)}
// Read cmd for QPI mode
#define FLASH_FAST_READ_QPI_XDMY_CFG(xba, xdmy)     \
                                            {0x0BU, FCFG(BUS_4BIT, 8, BUS_4BIT, (24 + 8 * xba), xdmy, BUS_4BIT)}
#define FLASH_FAST_READ_QIO_QPI_XDMY_CFG(xba, xdmy) \
                                            {0xEBU, FCFG(BUS_4BIT, 8, BUS_4BIT, (24 + 8 * xba), xdmy, BUS_4BIT)}
// Write cmd for SPI & QPI mode
#define FLASH_PAGE_PROG_CFG(xpi, xba)       {0x02U, FCFG(XBUS(xpi), 8, XBUS(xpi), (24 + 8 * xba), 0, XBUS(xpi))}
// Write cmd for SPI mode only
#define FLASH_PAGE_PROG_QI_CFG(xba)         {0x32U, FCFG(BUS_1BIT, 8, BUS_1BIT, (24 + 8 * xba), 0, BUS_4BIT)}
// Erase
#define FLASH_SEC_ERASE_4K_CFG(xpi, xba)    {0x20U, FCFG(XBUS(xpi), 8, XBUS(xpi), (24 + 8 * xba), 0, XBUS(xpi))}
#define FLASH_BLK_ERASE_32K_CFG(xpi, xba)   {0x52U, FCFG(XBUS(xpi), 8, XBUS(xpi), (24 + 8 * xba), 0, XBUS(xpi))}
#define FLASH_BLK_ERASE_64K_CFG(xpi, xba)   {0xD8U, FCFG(XBUS(xpi), 8, XBUS(xpi), (24 + 8 * xba), 0, XBUS(xpi))}
#define FLASH_CHIP_ERASE_CFG(xpi, NA)       {0x60U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
// Read ID
#define FLASH_READ_ID_CFG(xpi)              {0x9FU, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
// Read UID
#define FLASH_READ_UID_CFG(xpi, xba)        {0x4BU, FCFG(XBUS(xpi), 8, XBUS(xpi), (24 + 8 * xba), 8, XBUS(xpi))}
// Read & Write Reg
#define FLASH_READ_STA_REG1_CFG(xpi)        {0x05U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_WRITE_STA_REG1_CFG(xpi)       {0x01U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_READ_STA_REG2_CFG(xpi)        {0x35U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_WRITE_STA_REG2_CFG(xpi)       {0x31U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_READ_CFG_REG_CFG(xpi)         {0x15U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_WRITE_CFG_REG_CFG(xpi)        {0x11U, FCFG(XBUS(xpi), 8, XBUS(xpi), 8, 0, XBUS(xpi))}
// Write enable/disable
#define FLASH_WRIET_ENABLE_VSR_CFG(xpi)     {0x50U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_WRITE_ENABLE_CFG(xpi)         {0x06U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_WRITE_DISABLE_CFG(xpi)        {0x04U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
// Reset
#define FLASH_RESET_ENABLE_CFG(xpi)         {0x66U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_RESET_CFG(xpi)                {0x99U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
// Deep power down
#define FLASH_DEEP_PWR_DWN_CFG(xpi)         {0xB9U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_RELEASE_PWR_DWN_CFG(xpi)      {0xABU, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
// Suspend/Resume
#define FLASH_SUSPEND_CFG(xpi)              {0x75U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_RESUME_CFG(xpi)               {0x7AU, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
// QPI enable/disable
#define FLASH_QPI_ENABLE_CFG(NA)            {0x38U, FCFG(BUS_1BIT, 8, BUS_1BIT, 0,  0, BUS_1BIT)}
#define FLASH_QPI_DISABLE_CFG(NA)           {0xFFU, FCFG(BUS_4BIT, 8, BUS_4BIT, 0,  0, BUS_4BIT)}
// Set read param
#define FLASH_SET_READ_PARAM_CFG(NA)        {0xC0U, FCFG(BUS_4BIT, 8, BUS_4BIT, 0,  0, BUS_4BIT)}
// 4-Byte address enable/disable
#define FLASH_4BADR_ENABLE_CFG(xpi)         {0xB7U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
#define FLASH_4BADR_DISABLE_CFG(xpi)        {0xE9U, FCFG(XBUS(xpi), 8, XBUS(xpi), 0, 0, XBUS(xpi))}
// secure register
#define FLASH_SECURE_REG_ERASE_CFG(xpi)     {0x44U, FCFG(XBUS(xpi), 8, XBUS(xpi), 24, 0, XBUS(xpi))}
#define FLASH_SECURE_REG_READ_CFG(xpi)      {0x48U, FCFG(XBUS(xpi), 8, XBUS(xpi), 24, 8, XBUS(xpi))}
#define FLASH_SECURE_REG_WRITE_CFG(xpi)     {0x42U, FCFG(XBUS(xpi), 8, XBUS(xpi), 24, 0, XBUS(xpi))}


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    drv_isr_callback_t isr_cb;              /*!< Interrupt callback */
    uint32_t addr;                          /*!< Flash address to write */
    uint8_t *data;                          /*!< Data buffer to write */
    uint32_t data_len;                      /*!< Total length of data to write */
    uint32_t data_cnt;                      /*!< Length of data written */
    flash_read_t read_cmd;                  /*!< Flash command used in reading data, see@flash_read_t */
    flash_write_t write_cmd;                /*!< Flash command used in writing data, see@flash_write_t */
    uint8_t xpi_mode;                       /*!< Indicate flash is in QPI or SPI mode */
    uint8_t xba_mode;                       /*!< Indicate flash is in 4Byte or 3Byte address mode */
    flash_trans_state_t write_int_s;        /*!< Write state, used for write interrupt */
    flash_state_t state;                    /*!< Indicate flash is state on reading or writing */
    flash_id_t id;                          /*!< Flash id, see@flash_id_t */
    flash_delay_info_t delay_info;          /*!< Delay information */
} oflash_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
// flash models that not supported modify status register 1 and 2 by one command(0x01)
static const flash_id_t no_dual_reg_modify_flash[] = {
    {{0xC8, 0x40, 0x17}},     /* GD25Q64 */
};

static oflash_env_t flash1_env = {
    .isr_cb = NULL,
    .addr = 0,
    .data = NULL,
    .data_len = 0,
    .data_cnt = 0,
    .id.id = 0,
    .state = FLASH_STATE_UNINIT,
};

__OFLASH_CODE static om_error_t oflash_write_reg(flash_frame_t *write_reg_frame,
                                                 flash_frame_t *wip_frame,
                                                 uint8_t *data,
                                                 uint8_t data_len,
                                                 flash_reg_write_en_t wr_en_type);

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
void flash_read_frame_get(flash_read_t read_cmd, flash_frame_t *frame)
{
    uint8_t xba;

    xba = flash1_env.xba_mode;
    switch(read_cmd) {
        case FLASH_FAST_READ:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_CFG(xba));
            break;
        case FLASH_FAST_READ_DO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_DO_CFG(xba));
            break;
        case FLASH_FAST_READ_DIO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_DIO_CFG(xba));
            break;
        case FLASH_FAST_READ_QO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_QO_CFG(xba));
            break;
        case FLASH_FAST_READ_QIO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_QIO_CFG(xba));
            break;
        case FLASH_FAST_READ_QPI_4_DUMMY:
        case FLASH_FAST_READ_QPI_6_DUMMY:
        case FLASH_FAST_READ_QPI_8_DUMMY:
        case FLASH_FAST_READ_QPI_10_DUMMY:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_QPI_XDMY_CFG(xba,
                        (4 + (read_cmd - FLASH_FAST_READ_QPI_4_DUMMY) * 2)));
            break;
        case FLASH_FAST_READ_QIO_QPI_4_DUMMY:
        case FLASH_FAST_READ_QIO_QPI_6_DUMMY:
        case FLASH_FAST_READ_QIO_QPI_8_DUMMY:
        case FLASH_FAST_READ_QIO_QPI_10_DUMMY:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_QIO_QPI_XDMY_CFG(xba,
                        (4 + (read_cmd - FLASH_FAST_READ_QIO_QPI_4_DUMMY) * 2)));
            break;
        case FLASH_READ:
        default:
            CMD_FRAME_SET(frame, FLASH_READ_CFG(xba));
            break;
    }
}

void flash_write_frame_get(flash_write_t write_cmd, flash_frame_t *frame)
{
    switch (write_cmd) {
        case FLASH_PAGE_PROGRAM_QI:
            CMD_FRAME_SET(frame, FLASH_PAGE_PROG_QI_CFG(flash1_env.xba_mode)); // only spi mode
            break;
        case FLASH_PAGE_PROGRAM:
        case FLASH_PAGE_PROGRAM_QPI:
        default:
            CMD_FRAME_SET(frame, FLASH_PAGE_PROG_CFG(flash1_env.xpi_mode, flash1_env.xba_mode));
            break;
    }
}

void flash_erase_frame_get(flash_erase_t erase_type, flash_frame_t *frame)
{
    uint8_t xba = flash1_env.xba_mode;
    uint8_t xpi = flash1_env.xpi_mode;

    switch (erase_type) {
        case FLASH_ERASE_32K:
            CMD_FRAME_SET(frame, FLASH_BLK_ERASE_32K_CFG(xpi, xba));
            break;
        case FLASH_ERASE_64K:
            CMD_FRAME_SET(frame, FLASH_BLK_ERASE_64K_CFG(xpi, xba));
            break;
        case FLASH_ERASE_CHIP:
            CMD_FRAME_SET(frame, FLASH_CHIP_ERASE_CFG(xpi, xba));
            break;
        case FLASH_ERASE_4K:
        default:
            CMD_FRAME_SET(frame, FLASH_SEC_ERASE_4K_CFG(xpi, xba));
            break;
    }
}

__OFLASH_CODE static om_error_t oflash_read_reg(flash_frame_t *frame, uint8_t *data, uint8_t data_len)
{
    om_error_t error;
    uint32_t cmd[2];
    uint32_t read_cfg[2];
    uint32_t sec_cfg;

    cmd[0] = frame->cmd << 24U;
    cmd[1] = 0;
    OM_CRITICAL_BEGIN();
    // Store frame config
    drv_ospi_sec_cfg_get(OM_OSPI1, &sec_cfg);
    drv_ospi_read_cfg_get(OM_OSPI1, read_cfg);
    // Set read reg frame config
    drv_ospi_sec_cfg_set(OM_OSPI1, 0);
    drv_ospi_read_cfg_set(OM_OSPI1, frame->frame_cfg);
    // Start read
    error = drv_ospi_read(OM_OSPI1, cmd, data, data_len);
    // Restore frame config
    drv_ospi_sec_cfg_set(OM_OSPI1, sec_cfg);
    drv_ospi_read_cfg_set(OM_OSPI1, read_cfg);
    OM_CRITICAL_END();

    return error;
}
/* XM25QH128A:
 * tDP   = 3 us
 * tRES1 = 3 us
 * 这里统一使用 5 us，留出一定裕量。
 */
#define OFLASH_DPD_DELAY_US             (5U)
#define OFLASH_READY_POLL_INTERVAL_US   (10U)
#define OFLASH_READY_POLL_MAX_COUNT     (100000U) /* 最长约1秒 */

/**
 *******************************************************************************
 * @brief  外部 Flash 进入 Deep Power-down
 *
 * @param  om_flash OSPI控制器，只支持OM_OSPI1
 *
 * @return
 *  - OM_ERROR_OK        进入成功
 *  - OM_ERROR_PARAMETER 参数错误
 *  - OM_ERROR_TIMEOUT   Flash持续忙
 *  - 其他值             OSPI通信错误
 *******************************************************************************
 */
 om_error_t drv_oflash_deep_power_down(
    OM_OSPI_Type *om_flash)
{
    flash_frame_t status_frame;
    flash_frame_t power_down_frame;
    om_error_t error;
    uint8_t status;
    uint32_t poll_count;

    if (om_flash != OM_OSPI1) {
        return OM_ERROR_PARAMETER;
    }

    /*
     * 进入Deep Power-down前必须确认Flash没有正在执行
     * Program、Erase或者Write Status操作。
     */
    CMD_FRAME_SET(&status_frame,
                  FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode));

    for (poll_count = 0U;
         poll_count < OFLASH_READY_POLL_MAX_COUNT;
         poll_count++) {

        status = 0xFFU;

        error = oflash_read_reg(&status_frame, &status, 1U);
        if (error != OM_ERROR_OK) {
            return error;
        }

        if ((status & FLASH_STATUS_1_WIP_MASK) == 0U) {
            break;
        }

        drv_dwt_delay_us(OFLASH_READY_POLL_INTERVAL_US);
    }

    if (poll_count >= OFLASH_READY_POLL_MAX_COUNT) {
        return OM_ERROR_TIMEOUT;
    }

    /*
     * 发送B9h。
     *
     * 注意：
     * 这里只能发送命令，发送后绝对不能调用oflash_poll_wip()，
     * 因为Flash进入Deep Power-down后会忽略05h状态读取命令。
     */
    CMD_FRAME_SET(&power_down_frame,
                  FLASH_DEEP_PWR_DWN_CFG(flash1_env.xpi_mode));

    error = oflash_read_reg(&power_down_frame, NULL, 0U);
    if (error != OM_ERROR_OK) {
        return error;
    }

    /*
     * XM25QH128A的tDP为3us。
     * 等待完成后，Flash才真正进入最低功耗状态。
     */
    drv_dwt_delay_us(OFLASH_DPD_DELAY_US);

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief  外部 Flash 退出 Deep Power-down
 *
 * @param  om_flash OSPI控制器，只支持OM_OSPI1
 *
 * @return
 *  - OM_ERROR_OK        唤醒成功
 *  - OM_ERROR_PARAMETER 参数错误
 *  - 其他值             OSPI通信错误
 *******************************************************************************
 */
 om_error_t drv_oflash_release_deep_power_down(
    OM_OSPI_Type *om_flash)
{
    flash_frame_t release_frame;
    om_error_t error;

    if (om_flash != OM_OSPI1) {
        return OM_ERROR_PARAMETER;
    }

    /*
     * 发送ABh，只执行命令，不读取Device ID。
     */
    CMD_FRAME_SET(&release_frame,
                  FLASH_RELEASE_PWR_DWN_CFG(flash1_env.xpi_mode));

    error = oflash_read_reg(&release_frame, NULL, 0U);
    if (error != OM_ERROR_OK) {
        return error;
    }

    /*
     * XM25QH128A的tRES1为3us。
     * 在此期间CS必须保持高电平，不能发送其他Flash命令。
     */
    drv_dwt_delay_us(OFLASH_DPD_DELAY_US);

    /*
     * 延时结束以后，才允许读取状态、ID或者Flash数据。
     * 这里不需要轮询WIP。
     */
    return OM_ERROR_OK;
}

// // 睡眠flash
// om_error_t drv_oflash_deep_power_down(void)
// {
//     // WIP帧（读取状态寄存器1）
//     flash_frame_t wip_frame;
//     CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode)); 
    
//     // 深度掉电命令帧
//     flash_frame_t deep_pwr_dwn_frame = FLASH_DEEP_PWR_DWN_CFG(flash1_env.xpi_mode);
    
// 	om_error_t error = oflash_write_reg(&deep_pwr_dwn_frame, &wip_frame, NULL, 0, 0);
	
// 	return error;
     
// }
// // 唤醒flash
// om_error_t drv_oflash_release_deep_power_down(OM_OSPI_Type *om_flash)
// {
// 	// WIP帧（读取状态寄存器1）
//     flash_frame_t wip_frame;
//     CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode)); 
    
//     // 唤醒命令帧
//     flash_frame_t deep_pwr_dwn_frame = FLASH_RELEASE_PWR_DWN_CFG(flash1_env.xpi_mode);
	
//     om_error_t error = oflash_write_reg(&deep_pwr_dwn_frame, &wip_frame, NULL, 0, 0);
	
// 	return error;
// }

//// 使用现有的函数组合实现
//om_error_t wakeup_and_reconfigure(void)
//{
//    // 唤醒
//    drv_oflash_release_deep_power_down(OM_OSPI1);
//    DRV_DELAY_US(50);
//    return(0);
//    // 通过重新设置命令来恢复模式
//   // return drv_oflash_read_cmd_set(OM_OSPI1, flash1_env.read_cmd);
//}
__OFLASH_CODE static om_error_t oflash_write_enable(OM_OSPI_Type *om_flash)
{
    flash_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_WRITE_ENABLE_CFG(flash1_env.xpi_mode));
    return oflash_read_reg(&frame, NULL, 0);
}

__OFLASH_CODE static om_error_t oflash_write_enable_vsr(OM_OSPI_Type *om_flash)
{
    flash_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_WRIET_ENABLE_VSR_CFG(flash1_env.xpi_mode));
    return oflash_read_reg(&frame, NULL, 0);
}

__OFLASH_CODE static om_error_t oflash_poll_wip(flash_frame_t *wip_frame)
{
    uint8_t status = 0xFF;
    om_error_t error;

    do {
        error = oflash_read_reg(wip_frame, &status, 1);
    } while ((error != OM_ERROR_OK) || (status & FLASH_STATUS_1_WIP_MASK));
    return error;
}

__OFLASH_CODE static om_error_t oflash_write_reg(flash_frame_t *write_reg_frame,
                                                 flash_frame_t *wip_frame,
                                                 uint8_t *data,
                                                 uint8_t data_len,
                                                 flash_reg_write_en_t wr_en_type)
{
    om_error_t error;
    oflash_env_t *env = &flash1_env;
    uint32_t cmd[2];
    uint32_t read_cfg[2], frame_cfg[2];
    uint32_t sec_cfg;

    if (data_len >= 4) {
        return OM_ERROR_PARAMETER;
    }
    cmd[0] = write_reg_frame->cmd << 24U;
    // the data[0] is transfered first
    cmd[1] = 0;
    for (uint8_t i = 0; i < data_len; i++) {
        cmd[1] |= (data[i] << (8 * ((env->xba_mode ? 3 : 2) - i)));
    }
    OM_CRITICAL_BEGIN();
    // Store frame config
    drv_ospi_sec_cfg_get(OM_OSPI1, &sec_cfg);
    drv_ospi_read_cfg_get(OM_OSPI1, read_cfg);
    // Set read reg frame config, the data length should switch to bits
    frame_cfg[0] = write_reg_frame->frame_cfg[0] | ((data_len << 3) << OSPI_SW_CFG0_P1_BIT_CNT_POS);
    frame_cfg[1] = write_reg_frame->frame_cfg[1];
    drv_ospi_sec_cfg_set(OM_OSPI1, 0);
    drv_ospi_read_cfg_set(OM_OSPI1, frame_cfg);
    // Set write enable
    if (wr_en_type == SR_WRITE_EN_PERMANENT) {
        oflash_write_enable(OM_OSPI1);
    } else if (wr_en_type == SR_WRITE_EN_VOLATILE) {
        oflash_write_enable_vsr(OM_OSPI1);
    }
    // Start Read
    error = drv_ospi_read(OM_OSPI1, cmd, NULL, 0);
    // Restore frame config
    drv_ospi_sec_cfg_set(OM_OSPI1, sec_cfg);
    drv_ospi_read_cfg_set(OM_OSPI1, read_cfg);
    OM_CRITICAL_END();
    // wait done
    return error == OM_ERROR_OK ? oflash_poll_wip(wip_frame) : error;
}

__OFLASH_CODE static om_error_t oflash_read_frame_set(OM_OSPI_Type *om_flash,
                                                      flash_frame_t *qpi_frame,
                                                      flash_frame_t *read_frame)
{
    om_error_t error = OM_ERROR_OK;

    // if qpi frame is not null, set qpi mode
    if (qpi_frame->cmd) {
        if ((error = oflash_read_reg(qpi_frame, NULL, 0)) != OM_ERROR_OK) {
            return error;
        }
    }
    // Update read frame config
    drv_ospi_read_frame_set(om_flash, (const ospi_frame_t *)read_frame);
    return error;
}

__OFLASH_CODE static om_error_t oflash_secure_reg_read(OM_OSPI_Type *om_flash,
                                                       uint32_t addr,
                                                       uint8_t *data,
                                                       uint16_t data_len,
                                                       flash_frame_t *read_frame)
{
    om_error_t error;
    uint32_t cmd[2];
    uint32_t read_cfg[2];
    uint32_t sec_cfg;

    cmd[0] = read_frame->cmd << 24U;
    cmd[1] = addr;
    OM_CRITICAL_BEGIN();
    // Store frame config
    drv_ospi_sec_cfg_get(OM_OSPI1, &sec_cfg);
    drv_ospi_read_cfg_get(OM_OSPI1, read_cfg);
    // Set read reg frame config
    drv_ospi_sec_cfg_set(OM_OSPI1, 0);
    drv_ospi_read_cfg_set(OM_OSPI1, read_frame->frame_cfg);
    // Start read
    error = drv_ospi_read(OM_OSPI1, cmd, data, data_len);
    // Restore frame config
    drv_ospi_sec_cfg_set(OM_OSPI1, sec_cfg);
    drv_ospi_read_cfg_set(OM_OSPI1, read_cfg);
    OM_CRITICAL_END();
    return error;
}

#if (RTE_FLASH1_XIP)
__OFLASH_CODE static om_error_t oflash_suspend(flash_frame_t *suspend_frame)
{
    om_error_t error;

    if ((error = oflash_read_reg(suspend_frame, NULL, 0)) != OM_ERROR_OK) {
        return error;
    }
    // CS# High To Next Command After Suspend
    switch (flash1_env.id.man_id) {
        case FLASH_MID_PUYA:        // (0x856014) tPSL = 30us
        case FLASH_MID_GIGADEVICE:  // (0xc86514) tSUS = 40us
            DRV_DELAY_US(50);
            break;
        default:
            DRV_DELAY_US(50);
            break;
    }
    return error;
}

__OFLASH_CODE static om_error_t oflash_resume(flash_frame_t *resume_frame)
{
    om_error_t error;

    if ((error = oflash_read_reg(resume_frame, NULL, 0)) != OM_ERROR_OK) {
        return error;
    }
    // Latency between Program/Erase Resume and next Suspend
    switch (flash1_env.id.man_id) {
        case FLASH_MID_PUYA:        // (0x856014) tPRS = 20us
            DRV_DELAY_US(20 * 2);
            break;
        case FLASH_MID_GIGADEVICE:  // (0xc86514) tRS = 100us
            DRV_DELAY_US(110);
            break;
        default:
            DRV_DELAY_US(110);
            break;
    }
    return error;
}

__OFLASH_CODE static om_error_t oflash_poll_wip_with_suspend(flash_frame_t *wip_frame,
                                                             flash_frame_t *suspend_frame,
                                                             flash_frame_t *resume_frame,
                                                             uint32_t irq_save)
{
    om_error_t error, ret;
    uint8_t status = 0;

    while ((error = oflash_read_reg(wip_frame, &status, 1)) == OM_ERROR_OK &&
           status & FLASH_STATUS_1_WIP_MASK) {
        // Delay 100us
        DRV_WAIT_US_UNTIL_TO(!(drv_irq_is_any_ext_pending() && !irq_save), 100, ret);
        // is pending
        if (ret == OM_ERROR_OK) {
            OM_ASSERT(drv_irq_is_any_ext_pending());
            // suspend
            oflash_suspend(suspend_frame);
            // like call __enable_irq()
            OM_CRITICAL_END_EX(irq_save);
            // ENTER irq

            // like call __disable_irq()
            OM_CRITICAL_BEGIN_EX(irq_save);
            // resume
            oflash_resume(resume_frame);
        }
    }
    return error;
}

__OFLASH_CODE static om_error_t oflash_write_with_suspend(OM_OSPI_Type *om_flash,
                                                          uint32_t addr,
                                                          uint8_t *data,
                                                          uint32_t data_len,
                                                          flash_frame_t *write_frame,
                                                          flash_frame_t *wip_frame,
                                                          flash_frame_t *suspend_frame,
                                                          flash_frame_t *resume_frame)
{
    uint32_t cmd[2];
    om_error_t error;
    uint32_t irq_save;

    cmd[0] = write_frame->cmd << 24U;
    cmd[1] = addr;

    OM_CRITICAL_BEGIN_EX(irq_save);
    // Start Write
    error = drv_ospi_write(om_flash, cmd, data, data_len);
    if (error != OM_ERROR_OK) {
        OM_CRITICAL_END_EX(irq_save);
        return error;
    }
    error = oflash_poll_wip_with_suspend(wip_frame, suspend_frame, resume_frame, irq_save);
    OM_CRITICAL_END_EX(irq_save);
    return error;
}

__OFLASH_CODE static om_error_t oflash_erase_with_suspend(OM_OSPI_Type *om_flash,
                                                          uint32_t addr,
                                                          flash_frame_t *erase_frame,
                                                          flash_frame_t *wip_frame,
                                                          flash_frame_t *suspend_frame,
                                                          flash_frame_t *resume_frame)
{
    om_error_t error;
    uint32_t cmd[2];
    uint32_t read_cfg[2];
    uint32_t sec_cfg;
    uint32_t irq_save;

    cmd[0] = erase_frame->cmd << 24U;
    cmd[1] = addr;

    OM_CRITICAL_BEGIN_EX(irq_save);
    // Store frame config
    drv_ospi_sec_cfg_get(om_flash, &sec_cfg);
    drv_ospi_read_cfg_get(om_flash, read_cfg);
    // Set read reg frame config
    drv_ospi_sec_cfg_set(om_flash, 0);
    drv_ospi_read_cfg_set(om_flash, erase_frame->frame_cfg);
    // Start read
    error = drv_ospi_read(om_flash, cmd, NULL, 0);
    // Restore frame config
    drv_ospi_sec_cfg_set(om_flash, sec_cfg);
    drv_ospi_read_cfg_set(om_flash, read_cfg);

    error = oflash_poll_wip_with_suspend(wip_frame, suspend_frame, resume_frame, irq_save);
    OM_CRITICAL_END_EX(irq_save);
    return error;
}

#else

__OFLASH_CODE static om_error_t oflash_write(OM_OSPI_Type *om_flash,
                                             uint32_t addr,
                                             uint8_t *data,
                                             uint32_t data_len,
                                             flash_frame_t *write_frame,
                                             flash_frame_t *wip_frame)
{
    uint32_t cmd[2];
    om_error_t error;

    cmd[0] = write_frame->cmd << 24U;
    cmd[1] = addr;
    // Start Write
    error = drv_ospi_write(om_flash, cmd, data, data_len);
    if (error != OM_ERROR_OK) {
        return error;
    }
    return oflash_poll_wip(wip_frame);
}

__OFLASH_CODE static om_error_t oflash_erase_send_cmd(OM_OSPI_Type *om_flash,
                                                      uint32_t addr,
                                                      flash_frame_t *erase_frame)
{
    om_error_t error;
    uint32_t cmd[2];
    uint32_t read_cfg[2];
    uint32_t sec_cfg;

    cmd[0] = erase_frame->cmd << 24U;
    cmd[1] = addr;

    // Store frame config
    drv_ospi_sec_cfg_get(om_flash, &sec_cfg);
    drv_ospi_read_cfg_get(om_flash, read_cfg);
    // Set read reg frame config
    drv_ospi_sec_cfg_set(om_flash, 0);
    drv_ospi_read_cfg_set(om_flash, erase_frame->frame_cfg);
    // Start read
    error = drv_ospi_read(om_flash, cmd, NULL, 0);
    // Restore frame config
    drv_ospi_sec_cfg_set(om_flash, sec_cfg);
    drv_ospi_read_cfg_set(om_flash, read_cfg);
    return error;
}

__OFLASH_CODE static om_error_t oflash_erase(OM_OSPI_Type *om_flash,
                                             uint32_t addr,
                                             flash_frame_t *erase_frame,
                                             flash_frame_t *wip_frame)
{
    om_error_t error;

    error = oflash_erase_send_cmd(om_flash, addr, erase_frame);
    if (error != OM_ERROR_OK) {
        return error;
    }
    return oflash_poll_wip(wip_frame);
}
#endif /* RTE_FLASH1_XIP */

__OFLASH_CODE static om_error_t oflash_secure_reg_write(OM_OSPI_Type *om_flash,
                                                        uint32_t addr,
                                                        uint8_t *data,
                                                        uint16_t data_len,
                                                        flash_frame_t *write_frame,
                                                        flash_frame_t *wip_frame)
{
    om_error_t error;
    uint32_t cmd[2];
    uint32_t write_cfg[2];
    uint32_t sec_cfg;

    cmd[0] = write_frame->cmd << 24U;
    cmd[1] = addr;
    OM_CRITICAL_BEGIN();
    // Store frame config
    drv_ospi_sec_cfg_get(OM_OSPI1, &sec_cfg);
    drv_ospi_write_cfg_get(OM_OSPI1, write_cfg);
    // Set read reg frame config
    drv_ospi_sec_cfg_set(OM_OSPI1, 0);
    drv_ospi_write_cfg_set(OM_OSPI1, write_frame->frame_cfg);
    // Start write
    error = drv_ospi_write(OM_OSPI1, cmd, data, data_len);
    // Restore frame config
    drv_ospi_sec_cfg_set(OM_OSPI1, sec_cfg);
    drv_ospi_write_cfg_set(OM_OSPI1, write_cfg);

    if (error == OM_ERROR_OK) {
        // wait done
        error = oflash_poll_wip(wip_frame);
    }
    OM_CRITICAL_END();
    return error;
}

__OFLASH_CODE static om_error_t oflash_read_id(flash_frame_t *id_frame, flash_id_t *id)
{
    om_error_t error;
    flash_id_t id_read = {.id = 0};

    if ((error = oflash_read_reg(id_frame, (uint8_t*)&id_read, 3)) != OM_ERROR_OK) {
        return error;
    }
    if ((id_read.id & 0x00FFFFFF) == 0x00FFFFFF || id_read.id == 0x0) {
        return OM_ERROR_FAIL;
    }
    *id = id_read;
    return OM_ERROR_OK;
}

__OFLASH_CODE static om_error_t oflash_auto_delay_init(flash_frame_t *id_frame,
                                                       ospi_config_t *config)
{
    flash_id_t id1, id2;
    int32_t delayi, delay1 = -1, delay2 = FLASH_DELAY_MAX;
    ospi_config_t ospicfg = *config;
    uint8_t retry_cnt = 0;
    om_error_t error = OM_ERROR_FAIL;

    if (drv_rcc_clock_get(RCC_CLK_OSPI1) == 0) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_OSPI1, 1);
    }
    OM_CRITICAL_BEGIN();
    // set flash low frequency so it can work
    ospicfg.clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / FLASH_FREQ_HZ_DEFAULT;
    ospicfg.sample_cfg.sdr_async.sdr_async_dly = FLASH_DELAY_DEFAULT;
    drv_ospi_init(OM_OSPI1, &ospicfg);
	
	// 如果flash进入睡眠，需要发送唤醒指令，否则无法正常读取flash的ID
	drv_oflash_release_deep_power_down(OM_OSPI1);
	drv_dwt_delay_us(30);
    // read id twice, save the true id to id1
    while (1) {
        if ((oflash_read_id(id_frame, &id1) == OM_ERROR_OK) && (oflash_read_id(id_frame, &id2) == OM_ERROR_OK) && (id1.id == id2.id)) {
            break;
        }

        retry_cnt++;
        if (retry_cnt >= FLASH_AUTO_DLY_RETYR_CNT) {
            goto EXIT;
        }
    };

    // poll, delayi from 0 to DRV_SF_DELAY_MAX
    for (delayi = 0; delayi <= FLASH_DELAY_MAX; ++delayi) {
        ospicfg.clk_div = config->clk_div;
        ospicfg.sample_cfg.sdr_async.sdr_async_dly = delayi;
        drv_ospi_init(OM_OSPI1, &ospicfg);
        if ((oflash_read_id(id_frame, &id2) == OM_ERROR_OK) && (id1.id == id2.id)) {
            if (delay1 == -1) {
                delay1 = delayi;
            }
            delay2 = delayi;
        } else {
            if (delay1 != -1) {
                break;
            }
        }
    }
    if (delay1 == -1) {
        // faild, then set flash low frequency so it can work
        ospicfg.clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / FLASH_FREQ_HZ_DEFAULT;
        ospicfg.sample_cfg.sdr_async.sdr_async_dly = FLASH_DELAY_DEFAULT;
    } else {
        ospicfg.sample_cfg.sdr_async.sdr_async_dly = (delay1 + delay2) / 2;
        error = OM_ERROR_OK;
    }
    drv_ospi_init(OM_OSPI1, &ospicfg);

EXIT:
    OM_CRITICAL_END();
    flash1_env.delay_info.auto_delay = ospicfg.sample_cfg.sdr_async.sdr_async_dly;
    flash1_env.delay_info.valid_delay_max = delay2;
    flash1_env.delay_info.valid_delay_min = delay1;
    flash1_env.delay_info.auto_delay_en = 1;
    return error;
}

static om_error_t oflash_write_status(OM_OSPI_Type *om_flash, uint8_t status[2], uint8_t is_volatile)
{
    flash_frame_t reg_frame, wip_frame;
    oflash_env_t *env = &flash1_env;
    om_error_t error;

    CMD_FRAME_SET(&reg_frame, FLASH_WRITE_STA_REG1_CFG(env->xpi_mode));
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(env->xpi_mode));
    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    error = oflash_write_reg(&reg_frame, &wip_frame, status, 2,
                is_volatile ? SR_WRITE_EN_VOLATILE : SR_WRITE_EN_PERMANENT);
    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_END();
    #endif
    return error;
}

static om_error_t oflash_write_status_reg1(OM_OSPI_Type *om_flash, uint8_t *status, uint8_t is_volatile)
{
    om_error_t error;
    flash_frame_t reg_frame, wip_frame;

    CMD_FRAME_SET(&reg_frame, FLASH_WRITE_STA_REG1_CFG(flash1_env.xpi_mode));
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode));

    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    error = oflash_write_reg(&reg_frame, &wip_frame, status, 1,
                is_volatile ? SR_WRITE_EN_VOLATILE : SR_WRITE_EN_PERMANENT);
    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_END();
    #endif
    return error;
}


static om_error_t oflash_write_status_reg2(OM_OSPI_Type *om_flash, uint8_t *status, uint8_t is_volatile)
{
    om_error_t error;
    flash_frame_t reg_frame, wip_frame;
    uint8_t rd_status;

    CMD_FRAME_SET(&reg_frame, FLASH_WRITE_STA_REG2_CFG(flash1_env.xpi_mode));
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode));

    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    error = oflash_write_reg(&reg_frame, &wip_frame, status, 1,
                is_volatile ? SR_WRITE_EN_VOLATILE : SR_WRITE_EN_PERMANENT);
    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_END();
    #endif
    if (error != OM_ERROR_OK) {
        return error;
    }
    // read back to verify
    if ((error = drv_oflash_read_status_reg2(om_flash, &rd_status)) != OM_ERROR_OK) {
        return error;
    }
    if (rd_status != *status) {
        return OM_ERROR_VERIFY;
    }
    return OM_ERROR_OK;
}

static om_error_t oflash_write_config_reg(OM_OSPI_Type *om_flash, uint8_t *config, uint8_t is_volatile)
{
    om_error_t error;
    flash_frame_t reg_frame, wip_frame;
    CMD_FRAME_SET(&reg_frame, FLASH_WRITE_CFG_REG_CFG(flash1_env.xpi_mode));
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode));

    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    error = oflash_write_reg(&reg_frame, &wip_frame, config, 1,
                is_volatile ? SR_WRITE_EN_VOLATILE : SR_WRITE_EN_PERMANENT);
    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_END();
    #endif
    return error;
}

static om_error_t oflash_modifiy_status_bits(OM_OSPI_Type *om_flash, uint8_t status[2], uint8_t mask[2], uint8_t is_volatile)
{
    uint8_t s1[2];
    uint8_t s2[2];
    uint8_t need_write[2] = {1, 1};
    uint8_t modify_by_one_cmd = 1;
    om_error_t error = OM_ERROR_OK;

    // Read status reg1 and modify
    if ((error = drv_oflash_read_status_reg1(om_flash, &s1[0]) ) != OM_ERROR_OK) {
        goto EXIT;
    }
    if ((error = drv_oflash_read_status_reg1(om_flash, &s2[0])) != OM_ERROR_OK) {
        goto EXIT;
    }
    if (s1[0] != s2[0]) {
        return OM_ERROR_VERIFY;
    }
    if ((s1[0] & mask[0]) != (status[0] & mask[0])) {
        s1[0] &= ~mask[0];
        s1[0] |= (status[0] & mask[0]);
    } else if (is_volatile) {
        need_write[0] = 0;
    }
    // Read status reg2 and modify
    if ((error = drv_oflash_read_status_reg2(om_flash, &s1[1])) != OM_ERROR_OK) {
        goto EXIT;
    }
    if ((error = drv_oflash_read_status_reg2(om_flash, &s2[1])) != OM_ERROR_OK) {
        goto EXIT;
    }
    if (s1[1] != s2[1]) {
        return OM_ERROR_VERIFY;
    }
    if ((s1[1] & mask[1]) != (status[1] & mask[1])) {
        s1[1] &= ~mask[1];
        s1[1] |= (status[1] & mask[1]);
    } else if (is_volatile) {
        need_write[1] = 0;
    }
    // check if the current flash support dual register modify by one command
    for (uint16_t i = 0; i < sizeof(no_dual_reg_modify_flash) / sizeof(no_dual_reg_modify_flash[0]); i++) {
        if (flash1_env.id.id == no_dual_reg_modify_flash[i].id) {
            modify_by_one_cmd = 0;
            break;
        }
    }
    if (modify_by_one_cmd) {
        // write status reg 1 and reg 2 by one command
        if (need_write[0] || need_write[1]) {
            error = oflash_write_status(om_flash, s1, is_volatile);
        }
    } else {
        // write status reg 1, then write status reg 2
        if (need_write[0] && ((error = oflash_write_status_reg1(om_flash, &s1[0], is_volatile)) != OM_ERROR_OK)) {
            goto EXIT;
        }
        if (need_write[1] && ((error = oflash_write_status_reg2(om_flash, &s1[1], is_volatile)) != OM_ERROR_OK)) {
            goto EXIT;
        }
    }

EXIT:
    return error;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t drv_oflash_init(OM_OSPI_Type *om_flash, const flash_config_t *config)
{
    om_error_t error = OM_ERROR_OK;
    oflash_env_t *env = &flash1_env;
    flash_frame_t read_frame, write_frame;
    flash_config_t flash_cfg;

    if (drv_rcc_clock_get(RCC_CLK_OSPI1) == 0) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_OSPI1, 1);
    }
    env->xba_mode = M_3BA;
    env->xpi_mode = M_SPI;
    flash_read_frame_get(config->read_cmd, &read_frame);
    flash_write_frame_get(config->write_cmd, &write_frame);
    if (!config) {
        // If config is NULL, set default config, freq is 8MHz, delay is 2, bus width is 2
        flash_cfg.clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / FLASH_FREQ_HZ_DEFAULT;
        flash_cfg.delay = FLASH_DELAY_DEFAULT;
        flash_cfg.spi_mode = FLASH_SPI_MODE_0;
        flash_cfg.read_cmd = FLASH_FAST_READ_DO;
        flash_cfg.write_cmd = FLASH_PAGE_PROGRAM;
    } else {
        flash_cfg = *config;
    }
    ospi_config_t ospi_config = {
        .cs_config          = NULL,
        .read_frame_cfg     = {
            read_frame.frame_cfg[0],
            read_frame.frame_cfg[1],
        },
        .write_frame_cfg    = {
            write_frame.frame_cfg[0],
            write_frame.frame_cfg[1],
        },
        .read_opcode        = read_frame.cmd,
        .write_opcode       = write_frame.cmd,
        .mode               = (ospi_mode_t)flash_cfg.spi_mode,
        .page_cross_en      = 0,
        .page_size          = FLASH_PAGE_SIZE,
        .clk_div            = flash_cfg.clk_div,
        .sdr_async_en       = 1,
        .sample_cfg.sdr_async.sdr_async_dly
                            = flash_cfg.delay,
        .rw_data_width      = 0,
        .opcode_bypass_en   = 0,
        .is_normal_protocol = 1,
        .is_4bytes_addr     = 0,
        .encrypt_en         = 0,
        .decrypt_en         = 0,
    };

    // 1. Flash only support SPI mode 0/3.
    // 2. In SPI mode 3, clock division must be at least 2
    if (flash_cfg.spi_mode == FLASH_SPI_MODE_1 ||
        flash_cfg.spi_mode == FLASH_SPI_MODE_2 ||
        (flash_cfg.spi_mode == FLASH_SPI_MODE_3 && flash_cfg.clk_div < 2)) {
            return OM_ERROR_PARAMETER;
    }
    // auto delay
    if (config->delay == FLASH_DELAY_AUTO) {
        flash_frame_t id_frame;
        CMD_FRAME_SET(&id_frame, FLASH_READ_ID_CFG(flash1_env.xpi_mode));
        if(oflash_auto_delay_init(&id_frame, &ospi_config) != OM_ERROR_OK) {
            return OM_ERROR_FAIL;
        }
    } else {
        drv_ospi_init(om_flash, &ospi_config);
    }
    // Store read and write command
    if ((error = drv_oflash_read_cmd_set(om_flash, flash_cfg.read_cmd)) != OM_ERROR_OK) {
        goto INIT_EXIT;
    }
    if ((error = drv_oflash_write_cmd_set(om_flash, flash_cfg.write_cmd)) != OM_ERROR_OK) {
        goto INIT_EXIT;
    }
    // Read FLASH id
    if ((error = drv_oflash_read_id(om_flash, &env->id)) != OM_ERROR_OK) {
        goto INIT_EXIT;
    }
    #if (RTE_FLASH1_REGISTER_CALLBACK)
    drv_ospi_register_isr_callback(om_flash, (drv_isr_callback_t)drv_oflash_isr_callback);
    #endif
    env->state = FLASH_STATE_INIT;
    return OM_ERROR_OK;

INIT_EXIT:
    // set state uninit
    env->state = FLASH_STATE_UNINIT;
    return error;
}

om_error_t drv_oflash_read_uid(OM_OSPI_Type *om_flash, uint8_t *uid, uint32_t len)
{
    oflash_env_t *env = &flash1_env;
    flash_frame_t frame;
    om_error_t error;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    CMD_FRAME_SET(&frame, FLASH_READ_UID_CFG(env->xpi_mode, env->xba_mode));
    env->state = FLASH_STATE_READING;
    error = oflash_read_reg(&frame, uid, len);
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_oflash_id_get(OM_OSPI_Type *om_flash, flash_id_t *id)
{
    if ((flash1_env.id.id & 0x00FFFFFF) == 0x00FFFFFF || flash1_env.id.id == 0x0) {
        return OM_ERROR_FAIL;
    }
    *id = flash1_env.id;
    return OM_ERROR_OK;
}

om_error_t drv_oflash_read_cmd_set(OM_OSPI_Type *om_flash, flash_read_t read_cmd)
{
    oflash_env_t *env = &flash1_env;
    om_error_t error;
    flash_frame_t read_frame, qpi_frame = {0};
    uint8_t xpi_mode = env->xpi_mode;

    // Update quad mode
    if (read_cmd >= FLASH_FAST_READ_QO) {
        if ((error = drv_oflash_quad_enable(om_flash, 1)) != OM_ERROR_OK) {
            return error;
        }
    }
    // Update qpi frame
    if (read_cmd >= FLASH_FAST_READ_QPI_4_DUMMY && env->xpi_mode != M_QPI) {
        CMD_FRAME_SET(&qpi_frame, FLASH_QPI_ENABLE_CFG(0));
        xpi_mode = M_QPI;
    } else if (read_cmd < FLASH_FAST_READ_QPI_4_DUMMY && env->xpi_mode != M_SPI) {
        CMD_FRAME_SET(&qpi_frame, FLASH_QPI_DISABLE_CFG(0));
        xpi_mode = M_SPI;
    }
    // Set read frame and qpi mode
    flash_read_frame_get(read_cmd, &read_frame);
    OM_CRITICAL_BEGIN();
    error = oflash_read_frame_set(om_flash, &qpi_frame, &read_frame);
    if (error == OM_ERROR_OK) {
        // Update env
        env->read_cmd = read_cmd;
        env->xpi_mode = xpi_mode;
    }
    OM_CRITICAL_END();
    return error;
}

om_error_t drv_oflash_write_cmd_set(OM_OSPI_Type *om_flash, flash_write_t write_cmd)
{
    oflash_env_t *env = &flash1_env;
    om_error_t error = OM_ERROR_OK;
    flash_frame_t write_frame;

    // Update quad mode
    if (write_cmd >= FLASH_PAGE_PROGRAM_QI) {
        if ((error = drv_oflash_quad_enable(om_flash, 1)) != OM_ERROR_OK) {
            return error;
        }
    }
    flash_write_frame_get(write_cmd, &write_frame);
    // Update write frame config
    drv_ospi_write_frame_set(om_flash, (const ospi_frame_t *)&write_frame);
    // Update write frame of env
    env->write_cmd = write_cmd;
    return error;
}

om_error_t drv_oflash_read(OM_OSPI_Type *om_flash, uint32_t addr, uint8_t *data, uint32_t data_len)
{
    oflash_env_t *env = &flash1_env;
    uint32_t cmd[2];
    flash_frame_t read_frame;
    om_error_t error;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    flash_read_frame_get(env->read_cmd, &read_frame);
    cmd[0] = read_frame.cmd << 24;
    cmd[1] = addr;

    env->state = FLASH_STATE_READING;
    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    error = drv_ospi_read(om_flash, cmd, data, data_len);
    #if (RTE_FLASH1_XIP)
    OM_CRITICAL_END();
    #endif
    env->state = FLASH_STATE_INIT;

    return error;
}

om_error_t drv_oflash_read_int(OM_OSPI_Type *om_flash,
                               uint32_t addr,
                               uint8_t *data,
                               uint32_t data_len)
{
    oflash_env_t *env = &flash1_env;
    uint32_t cmd[2];
    flash_frame_t read_frame;

    // Do not use read/write with interrupt interfaces for internal flash
    #if (RTE_FLASH1_XIP)
    return OM_ERROR_PERMISSION;
    #endif

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    flash_read_frame_get(env->read_cmd, &read_frame);
    cmd[0] = read_frame.cmd << 24;
    cmd[1] = addr;
    env->state = FLASH_STATE_READING;
    env->addr = addr;
    env->data = data;
    env->data_len = data_len;
    drv_ospi_read_int(om_flash, cmd, data, data_len);
    return OM_ERROR_OK;
}

om_error_t drv_oflash_write(OM_OSPI_Type *om_flash,
                            uint32_t addr,
                            volatile uint8_t *data,
                            uint32_t data_len)
{
    oflash_env_t *env = &flash1_env;
    uint32_t data_remain = data_len;
    uint32_t addr_write = addr;
    uint8_t* data_write = (uint8_t *)data;
    om_error_t error = OM_ERROR_OK;
    flash_frame_t write_frame, wip_frame;

    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(env->xpi_mode));
    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    env->state = FLASH_STATE_WRITING;
    flash_write_frame_get(env->write_cmd, &write_frame);
    while (data_remain) {
        error = oflash_write_enable(om_flash);
        if (error == OM_ERROR_OK) {
            // Write to the next Page boundary
            uint32_t write_len = FLASH_PAGE_SIZE - (addr_write & (FLASH_PAGE_SIZE - 1));
            write_len = (data_remain >= write_len) ? write_len : data_remain;
            #if (RTE_FLASH1_XIP)
            flash_frame_t suspend_frame, resume_frame;
            CMD_FRAME_SET(&suspend_frame, FLASH_SUSPEND_CFG(env->xpi_mode));
            CMD_FRAME_SET(&resume_frame, FLASH_RESUME_CFG(env->xpi_mode));
            error = oflash_write_with_suspend(om_flash, addr_write, data_write, write_len,
                            &write_frame, &wip_frame, &suspend_frame, &resume_frame);
            #else
            error = oflash_write(om_flash, addr_write, data_write,
                        write_len, &write_frame, &wip_frame);
            #endif
            data_remain -= write_len;
            addr_write += write_len;
            data_write += write_len;
        } else {
            break;
        }
    }
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_oflash_write_int_start(OM_OSPI_Type *om_flash,
                                      uint32_t addr,
                                      volatile uint8_t *data,
                                      uint32_t data_len)
{
    oflash_env_t *env = &flash1_env;
    om_error_t error = OM_ERROR_OK;
    uint32_t write_len;
    uint32_t cmd[2];
    flash_frame_t write_frame;

    #if (RTE_FLASH1_XIP)
    return OM_ERROR_PERMISSION;
    #endif

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    flash_write_frame_get(env->write_cmd, &write_frame);
    env->state = FLASH_STATE_WRITING;
    error = oflash_write_enable(om_flash);
    if (error == OM_ERROR_OK) {
        // Write to the next Page boundary
        write_len = FLASH_PAGE_SIZE - (addr & (FLASH_PAGE_SIZE - 1));
        write_len = (data_len >= write_len) ? write_len : data_len;
        cmd[0] = write_frame.cmd << 24;
        cmd[1] = addr;
        env->write_int_s = FLASH_TRANS_BUSY;
        env->addr = addr + write_len;
        env->data = (uint8_t *)data + write_len;
        env->data_cnt = write_len;
        env->data_len = data_len;
        drv_ospi_write_int(om_flash, cmd, data, write_len);
    }
    return error;
}

om_error_t drv_oflash_write_int_status_get(OM_OSPI_Type *om_flash, uint8_t *is_wip)
{
    flash_frame_t wip_frame;
    uint8_t status;
    om_error_t error;

    // Waiting for the command done interrupt
    if (flash1_env.write_int_s == FLASH_TRANS_BUSY) {
        *is_wip = 1;
        return OM_ERROR_OK;
    }
    // Check ospi busy status again
    if (drv_ospi_is_busy(om_flash)) {
        return OM_ERROR_BUSY;
    }
    // Send command to check wip status
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode));
    if ((error = oflash_read_reg(&wip_frame, &status, 1)) != OM_ERROR_OK) {
        return error;
    }
    *is_wip = (status & FLASH_STATUS_1_WIP_MASK) ? 1 : 0;
    return OM_ERROR_OK;
}

om_error_t drv_oflash_write_int_continue(OM_OSPI_Type *om_flash)
{
    oflash_env_t *env = &flash1_env;
    om_error_t error;
    flash_frame_t write_frame;

    if ((env->state == FLASH_STATE_WRITING) && (env->data_cnt < env->data_len)) {
        if ((error = oflash_write_enable(om_flash)) == OM_ERROR_OK) {
            // Write to the next Page boundary
            uint32_t data_remain = env->data_len - env->data_cnt;
            uint32_t write_len = (data_remain >= FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : data_remain;
            flash_write_frame_get(env->write_cmd, &write_frame);
            uint32_t cmd[2] = {write_frame.cmd << 24, env->addr};
            env->write_int_s = FLASH_TRANS_BUSY;
            if ((error = drv_ospi_write_int(om_flash, cmd, env->data, write_len)) != OM_ERROR_OK) {
                return error;
            }
            env->addr += write_len;
            env->data += write_len;
            env->data_cnt += write_len;
            return OM_ERROR_OK;
        } else {
            return error;
        }
    }
    return OM_ERROR_STATUS;
}

om_error_t drv_oflash_erase(OM_OSPI_Type *om_flash,
                            uint32_t addr,
                            flash_erase_t erase_type)
{
    oflash_env_t *env = &flash1_env;
    om_error_t error = OM_ERROR_OK;
    flash_frame_t erase_frame;
    flash_frame_t wip_frame;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    uint32_t cap = FLASH_ID2CAP(env->id);
    if (addr >= cap) {
        return OM_ERROR_PERMISSION;
    }
    flash_erase_frame_get(erase_type, &erase_frame);
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(env->xpi_mode));
    env->state = FLASH_STATE_ERASING;
    error = oflash_write_enable(om_flash);
    if (error == OM_ERROR_OK) {
        #if (RTE_FLASH1_XIP)
        flash_frame_t suspend_frame, resume_frame;
        CMD_FRAME_SET(&suspend_frame, FLASH_SUSPEND_CFG(env->xpi_mode));
        CMD_FRAME_SET(&resume_frame, FLASH_RESUME_CFG(env->xpi_mode));
        error = oflash_erase_with_suspend(om_flash, addr,
                        &erase_frame, &wip_frame, &suspend_frame, &resume_frame);
        #else
        error = oflash_erase(om_flash, addr, &erase_frame, &wip_frame);
        #endif
    }
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_oflash_erase_start(OM_OSPI_Type *om_flash, uint32_t addr, flash_erase_t erase_type)
{
    // if xip mode, use drv_oflash_erase function
    #if (RTE_FLASH1_XIP)
    OM_ASSERT(0);
    return OM_ERROR_PERMISSION;
    #else
    oflash_env_t *env = &flash1_env;
    om_error_t error = OM_ERROR_OK;
    flash_frame_t erase_frame;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    uint32_t cap = FLASH_ID2CAP(env->id);
    if (addr >= cap) {
        return OM_ERROR_PERMISSION;
    }
    flash_erase_frame_get(erase_type, &erase_frame);
    env->state = FLASH_STATE_ERASING;
    if ((error = oflash_write_enable(om_flash)) != OM_ERROR_OK) {
        return error;
    }
    return oflash_erase_send_cmd(om_flash, addr, &erase_frame);
    #endif
}

uint8_t drv_oflash_erase_is_done(OM_OSPI_Type *om_flash)
{
    // if xip mode, use drv_oflash_erase function
    #if (RTE_FLASH1_XIP)
    OM_ASSERT(0);
    return 1;
    #else
    oflash_env_t *env = &flash1_env;
    flash_frame_t wip_frame;
    uint8_t status;

    if (env->state != FLASH_STATE_ERASING) {
        return 1;
    }
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(env->xpi_mode));
    if (oflash_read_reg(&wip_frame, &status, 1) != OM_ERROR_OK) {
        env->state = FLASH_STATE_INIT;
        return 0;
    }
    if (!(status & FLASH_STATUS_1_WIP_MASK)) {
        env->state = FLASH_STATE_INIT;
    }
    return (status & FLASH_STATUS_1_WIP_MASK) ? 0 : 1;
    #endif
}

om_error_t drv_oflash_read_id(OM_OSPI_Type *om_flash, flash_id_t *id)
{
    flash_frame_t id_frame;
    om_error_t error;

    CMD_FRAME_SET(&id_frame, FLASH_READ_ID_CFG(flash1_env.xpi_mode));
    if ((error = oflash_read_id(&id_frame, id)) != OM_ERROR_OK) {
        return error;
    }
    return OM_ERROR_OK;
}

om_error_t drv_oflash_read_status_reg1(OM_OSPI_Type *om_flash, uint8_t *status)
{
    flash_frame_t reg_frame;

    CMD_FRAME_SET(&reg_frame, FLASH_READ_STA_REG1_CFG(flash1_env.xpi_mode));
    return oflash_read_reg(&reg_frame, status, 1);
}

om_error_t drv_oflash_read_status_reg2(OM_OSPI_Type *om_flash, uint8_t *status)
{
    flash_frame_t reg_frame;

    CMD_FRAME_SET(&reg_frame, FLASH_READ_STA_REG2_CFG(flash1_env.xpi_mode));
    return oflash_read_reg(&reg_frame, status, 1);
}

om_error_t drv_oflash_read_config_reg(OM_OSPI_Type *om_flash, uint8_t *config)
{
    flash_frame_t reg_frame;

    CMD_FRAME_SET(&reg_frame, FLASH_READ_CFG_REG_CFG(flash1_env.xpi_mode));
    return oflash_read_reg(&reg_frame, config, 1);
}

om_error_t drv_oflash_write_status(OM_OSPI_Type *om_flash, uint8_t status[2])
{
    return oflash_write_status(om_flash, status, 0);
}

om_error_t drv_oflash_write_status_volatile(OM_OSPI_Type *om_flash, uint8_t status[2])
{
    return oflash_write_status(om_flash, status, 1);
}

om_error_t drv_oflash_write_status_reg1(OM_OSPI_Type *om_flash, uint8_t *status)
{
    return oflash_write_status_reg1(om_flash, status, 0);
}

om_error_t drv_oflash_write_status_reg1_volatile(OM_OSPI_Type *om_flash, uint8_t *status)
{
    return oflash_write_status_reg1(om_flash, status, 1);
}

om_error_t drv_oflash_write_status_reg2(OM_OSPI_Type *om_flash, uint8_t *status)
{
    return oflash_write_status_reg2(om_flash, status, 0);
}

om_error_t drv_oflash_write_status_reg2_volatile(OM_OSPI_Type *om_flash, uint8_t *status)
{
    return oflash_write_status_reg2(om_flash, status, 1);
}

om_error_t drv_oflash_write_config_reg(OM_OSPI_Type *om_flash, uint8_t *config)
{
    return oflash_write_config_reg(om_flash, config, 0);
}

om_error_t drv_oflash_write_config_reg_volatile(OM_OSPI_Type *om_flash, uint8_t *config)
{
    return oflash_write_config_reg(om_flash, config, 1);
}

om_error_t drv_oflash_modifiy_status_bits(OM_OSPI_Type *om_flash, uint8_t status[2], uint8_t mask[2])
{
    return oflash_modifiy_status_bits(om_flash, status, mask, 0);
}

om_error_t drv_oflash_modifiy_status_bits_volatile(OM_OSPI_Type *om_flash, uint8_t status[2], uint8_t mask[2])
{
    return oflash_modifiy_status_bits(om_flash, status, mask, 1);
}

om_error_t drv_oflash_write_protect_set(OM_OSPI_Type *om_flash, flash_protect_t protect)
{
    // Attention:
    // This function support the following flash:
    // GD25WQ16E, GD25WQ80E, GD25WQ40E,
    // P25Q40SU, P25Q80SU, P25Q16SU,
    // GT25Q40D, GT25Q80A
    // Other flash without testing,
    // if not supported, please use drv_oflash_modifiy_status_bits to set the status register

    uint8_t mask[2] = {FLASH_STATUS_1_BP_MASK, FLASH_STATUS_2_CMP_MASK};
    uint8_t status[2] = {protect, FLASH_STATUS_2_CMP_MASK};

    return drv_oflash_modifiy_status_bits(om_flash, status, mask);
}

om_error_t drv_oflash_write_protect_set_volatile(OM_OSPI_Type *om_flash, flash_protect_t protect)
{
    uint8_t mask[2] = {FLASH_STATUS_1_BP_MASK, FLASH_STATUS_2_CMP_MASK};
    uint8_t status[2] = {protect, FLASH_STATUS_2_CMP_MASK};

    return drv_oflash_modifiy_status_bits_volatile(om_flash, status, mask);
}

om_error_t drv_oflash_quad_enable(OM_OSPI_Type *om_flash, bool enable)
{
    uint8_t mask[2] = {0, FLASH_STATUS_2_QE_MASK};
    uint8_t status[2] = {0, 0};

    if (enable) {
        status[1] = FLASH_STATUS_2_QE_MASK;
    }

    return drv_oflash_modifiy_status_bits(om_flash, status, mask);
}

om_error_t drv_oflash_quad_enable_volatile(OM_OSPI_Type *om_flash, bool enable)
{
    uint8_t mask[2] = {0, FLASH_STATUS_2_QE_MASK};
    uint8_t status[2] = {0, 0};

    if (enable) {
        status[1] = FLASH_STATUS_2_QE_MASK;
    }

    return drv_oflash_modifiy_status_bits_volatile(om_flash, status, mask);
}

om_error_t drv_oflash_read_param_set(OM_OSPI_Type *om_flash, uint8_t param)
{
    oflash_env_t *env = &flash1_env;
    flash_frame_t set_param_frame, wip_frame;

    CMD_FRAME_SET(&set_param_frame, FLASH_SET_READ_PARAM_CFG(0));
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(env->xpi_mode));

    // QPI mode only
    if (env->xpi_mode != M_QPI) {
        return OM_ERROR_PERMISSION;
    }
    return oflash_write_reg(&set_param_frame, &wip_frame, &param, 1, SR_WRITE_EN_NONE);
}

om_error_t drv_oflash_4byte_addr_enable(OM_OSPI_Type *om_flash, uint8_t enable)
{
    oflash_env_t *env = &flash1_env;
    om_error_t error;
    flash_frame_t rw_frame;
    flash_frame_t addr_frame;

    if (enable) {
        CMD_FRAME_SET(&addr_frame, FLASH_4BADR_ENABLE_CFG(env->xpi_mode));
    } else {
        CMD_FRAME_SET(&addr_frame, FLASH_4BADR_DISABLE_CFG(env->xpi_mode));
    }
    OM_CRITICAL_BEGIN();
    error = oflash_read_reg(&addr_frame, NULL, 0);
    if (error == OM_ERROR_OK) {
        env->xba_mode = enable ? M_4BA : M_3BA;
        drv_ospi_set_addr_4byte(om_flash, env->xba_mode);
        flash_read_frame_get(env->read_cmd, &rw_frame);
        drv_ospi_read_frame_set(om_flash, (const ospi_frame_t *)&rw_frame);
        flash_write_frame_get(env->write_cmd, &rw_frame);
        drv_ospi_write_frame_set(om_flash, (const ospi_frame_t *)&rw_frame);
    }
    OM_CRITICAL_END();
    return error;
}

__OFLASH_CODE om_error_t drv_oflash_reset(OM_OSPI_Type *om_flash)
{
    om_error_t error = OM_ERROR_OK;
    flash_frame_t rst_en_frame, rst_frame, rw_frame;
    oflash_env_t *env = &flash1_env;

    CMD_FRAME_SET(&rst_en_frame, FLASH_RESET_ENABLE_CFG(flash1_env.xpi_mode));
    CMD_FRAME_SET(&rst_frame, FLASH_RESET_CFG(flash1_env.xpi_mode));

    if ((error = oflash_read_reg(&rst_en_frame, NULL, 0)) != OM_ERROR_OK) {
        return error;
    }
    if ((error = oflash_read_reg(&rst_frame, NULL, 0)) != OM_ERROR_OK) {
        return error;
    }
    // reset env spi mode
    env->xpi_mode = M_SPI;
    if (env->read_cmd >= FLASH_FAST_READ_QPI_4_DUMMY) {
        env->read_cmd = FLASH_FAST_READ;
    }
    // reset env 3-byte address mode
    env->xba_mode = M_3BA;
    drv_ospi_set_addr_4byte(om_flash, env->xba_mode);
    flash_read_frame_get(env->read_cmd, &rw_frame);
    drv_ospi_read_frame_set(om_flash, (const ospi_frame_t *)&rw_frame);
    flash_write_frame_get(env->write_cmd, &rw_frame);
    drv_ospi_write_frame_set(om_flash, (const ospi_frame_t *)&rw_frame);
    return error;
}

__OFLASH_CODE om_error_t drv_oflash_encrypt_enable(OM_OSPI_Type *om_flash, uint8_t enable)
{
    #if (RTE_EFUSE)
    if (enable) {
        drv_efuse_control(EFUSE_CONTROL_FETCH_UID, (void *)0);
    }
    #endif /* RTE_EFUSE */
    drv_ospi_set_crypt_control(om_flash, enable, enable);
    return OM_ERROR_OK;
}

om_error_t drv_oflash_list_start(OM_OSPI_Type *om_flash, flash_list_node_t *list_head)
{
    om_error_t error;
    oflash_env_t *env = &flash1_env;
    uint8_t dummy;

    if ((error = drv_oflash_read_cmd_set(om_flash, env->read_cmd)) != OM_ERROR_OK) {
        return error;
    }
    drv_ospi_list_start(om_flash, list_head);
    // dummy read to trigger list start
    return drv_oflash_read(om_flash, 0, &dummy, 1);
}

om_error_t drv_oflash_secure_register_erase(OM_OSPI_Type *om_flash, uint8_t secure_register)
{
    oflash_env_t *env = &flash1_env;
    flash_frame_t erase_sec_reg_frame;
    flash_frame_t wip_frame;
    om_error_t error;

    CMD_FRAME_SET(&erase_sec_reg_frame, FLASH_SECURE_REG_ERASE_CFG(env->xpi_mode));
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(env->xpi_mode));

    env->state = FLASH_STATE_ERASING;
    OM_CRITICAL_BEGIN();
    error = oflash_write_enable(om_flash);
    if (error == OM_ERROR_OK) {
        error = oflash_erase(om_flash, FLASH_SECURE_REG_ADDR_HIGH(secure_register),
            &erase_sec_reg_frame, &wip_frame);
    }
    OM_CRITICAL_END();
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_oflash_secure_register_write(OM_OSPI_Type *om_flash,
                                            uint8_t secure_register,
                                            uint16_t addr,
                                            uint8_t *data,
                                            uint16_t data_len)
{
    oflash_env_t *env = &flash1_env;
    flash_frame_t write_sec_reg_frame;
    flash_frame_t wip_frame;
    om_error_t error;

    CMD_FRAME_SET(&write_sec_reg_frame, FLASH_SECURE_REG_WRITE_CFG(env->xpi_mode));
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STA_REG1_CFG(env->xpi_mode));

    env->state = FLASH_STATE_WRITING;
    OM_CRITICAL_BEGIN();
    error = oflash_write_enable(om_flash);
    if (error == OM_ERROR_OK) {
        error = oflash_secure_reg_write(om_flash, FLASH_SECURE_REG_ADDR(secure_register, addr), data,
                    data_len, &write_sec_reg_frame, &wip_frame);
    }
    OM_CRITICAL_END();
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_oflash_secure_register_read(OM_OSPI_Type *om_flash,
                                           uint8_t secure_register,
                                           uint16_t addr,
                                           uint8_t *data,
                                           uint16_t data_len)
{
    oflash_env_t *env = &flash1_env;
    flash_frame_t read_sec_reg_frame;
    om_error_t error;

    CMD_FRAME_SET(&read_sec_reg_frame, FLASH_SECURE_REG_READ_CFG(env->xpi_mode));

    env->state = FLASH_STATE_READING;

    error = oflash_secure_reg_read(om_flash,
        FLASH_SECURE_REG_ADDR(secure_register, addr), data, data_len, &read_sec_reg_frame);
    env->state = FLASH_STATE_INIT;
    return error;
}

#if (RTE_FLASH1_REGISTER_CALLBACK)
om_error_t drv_oflash_register_isr_callback(OM_OSPI_Type *om_flash, drv_isr_callback_t isr_cb)
{
    oflash_env_t *env = &flash1_env;

    env->isr_cb = isr_cb;
    return OM_ERROR_OK;
}

__WEAK void drv_oflash_isr_callback(OM_OSPI_Type *om_flash, drv_event_t event)
{
    oflash_env_t *env = &flash1_env;
    flash_state_t state = env->state;
    drv_event_t isr_evt = DRV_EVENT_COMMON_NONE;

    if (event & DRV_EVENT_COMMON_TRANSFER_COMPLETED) {
        if (state == FLASH_STATE_WRITING) {
            env->write_int_s = FLASH_TRANS_IDLE;
            if (env->data_cnt >= env->data_len) {
                env->data -= env->data_cnt;
                isr_evt |= DRV_EVENT_FLASH_WRITE_TRANSFER_COMPLETED;
            } else {
                // Writing not finished, wait for write continue
                return;
            }
        } else if (state == FLASH_STATE_READING) {
            isr_evt |= DRV_EVENT_COMMON_READ_COMPLETED;
        }
    }
    if (event & DRV_EVENT_OSPI_LIST_COMPLETED) {
        isr_evt |= DRV_EVENT_FLASH_LIST_COMPLETED;
    }
    if (event & DRV_EVENT_OSPI_LIST_NODE_COMPLETED) {
        isr_evt |= DRV_EVENT_FLASH_LIST_NODE_COMPLETED;
    }
    env->state = FLASH_STATE_INIT;
    if (isr_evt && env->isr_cb) {
        env->isr_cb(om_flash, isr_evt, (void *)env->data, (void *)env->data_len);
    }
}
#endif /* RTE_FLASH1_REGISTER_CALLBACK */
#endif /* (RTE_FLASH1) */

/** @} */
