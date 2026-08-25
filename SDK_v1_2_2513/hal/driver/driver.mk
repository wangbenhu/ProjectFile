HAL_DRV_DIR = $(HAL_DIR)/driver

HAL_DRV_SRCS = $(HAL_DRV_DIR)/source/drv_mpu.c                                 \
               $(HAL_DRV_DIR)/source/drv_om24g.c                               \
               $(HAL_DRV_DIR)/source/drv_gpadc.c                               \
               $(HAL_DRV_DIR)/source/drv_aes.c                                 \
               $(HAL_DRV_DIR)/source/drv_calib_repair.c                        \
               $(HAL_DRV_DIR)/source/drv_calib.c                               \
               $(HAL_DRV_DIR)/source/drv_gpdma.c                               \
               $(HAL_DRV_DIR)/source/drv_efuse.c                               \
               $(HAL_DRV_DIR)/source/drv_gpio.c                                \
               $(HAL_DRV_DIR)/source/drv_i2c.c                                 \
               $(HAL_DRV_DIR)/source/drv_isr.c                                 \
               $(HAL_DRV_DIR)/source/drv_lptim.c                               \
               $(HAL_DRV_DIR)/source/drv_pin.c                                 \
               $(HAL_DRV_DIR)/source/drv_pmu_timer.c                           \
               $(HAL_DRV_DIR)/source/drv_pmu.c                                 \
               $(HAL_DRV_DIR)/source/drv_radio.c                               \
               $(HAL_DRV_DIR)/source/drv_rcc.c                                 \
               $(HAL_DRV_DIR)/source/drv_rng.c                                 \
               $(HAL_DRV_DIR)/source/drv_rtc.c                                 \
               $(HAL_DRV_DIR)/source/drv_spi.c                                 \
               $(HAL_DRV_DIR)/source/drv_tim.c                                 \
               $(HAL_DRV_DIR)/source/drv_uart.c                                \
               $(HAL_DRV_DIR)/source/drv_wdt.c                                 \
               $(HAL_DRV_DIR)/source/drv_i2s.c                                 \
               $(HAL_DRV_DIR)/source/drv_lcd.c                                 \
               $(HAL_DRV_DIR)/source/drv_irtx.c                                \
               $(HAL_DRV_DIR)/source/drv_sha256.c                              \
               $(HAL_DRV_DIR)/source/drv_audio.c                               \
               $(HAL_DRV_DIR)/source/drv_audio_inside.c                        \
               $(HAL_DRV_DIR)/source/drv_audio_outside.c                       \
               $(HAL_DRV_DIR)/source/drv_ecdsa/drv_ecdsa.c                     \
               $(HAL_DRV_DIR)/source/drv_ecdsa/uECC/uECC.c                     \
               $(HAL_DRV_DIR)/source/drv_flash/drv_iflash.c                    \
               $(HAL_DRV_DIR)/source/drv_flash/drv_oflash.c                    \
               $(HAL_DRV_DIR)/source/drv_flash/drv_flash.c                     \
               $(HAL_DRV_DIR)/source/drv_ospi.c                                \
               $(HAL_DRV_DIR)/source/drv_psram.c                               \
               $(HAL_DRV_DIR)/source/drv_qdec.c                                \
               $(HAL_DRV_DIR)/source/drv_rgb.c                                 \
               $(HAL_DRV_DIR)/source/drv_sm/drv_sm2.c                          \
               $(HAL_DRV_DIR)/source/drv_sm/drv_sm3.c                          \
               $(HAL_DRV_DIR)/source/drv_sm/drv_sm4.c                          \
               $(HAL_DRV_DIR)/source/drv_sm/gm.c                          	   \

HAL_DRV_INCS = $(HAL_DRV_DIR)/include
