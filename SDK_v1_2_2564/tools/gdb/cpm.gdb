#
# This script use list clock information
#

# CPU
if (OM_PMU->MISC_CTRL & (1 << 24)) == 0
    echo CPU:       32M, Source: RC32M\n
else
    set $xtal32m = (OM_PMU->XTAL32M_CNS0 & (1 << 16)) == 0 ? 0 : 1
    set $syspll  = (OM_DAIF->SYSPLL_CNS0 & (1 << 21)) == 0 ? 0 : 2

    if ($xtal32m + $syspll) == 0
        echo CPU:       32M, Source: XTAL32M\n
    end
    if ($xtal32m + $syspll) == 1
        echo CPU:       64M, Source: CLK64M\n
    end
    if ($xtal32m + $syspll) == 2
        echo CPU:       96M, Source: SYSPLL96M\n
    end
    if ($xtal32m + $syspll) == 3
        echo CPU:       48M, Source: SYSPLL48M\n
    end
end

# Peripheral
if (OM_CPM->CPU_CFG & (1 << 26)) == 0
    echo Peri:      32M, Source: RC32M\n
else
    set $val = (OM_DAIF->SYSPLL_CNS0 & (3 << 24)) >> 24

    if $val == 0
        echo Peri:      32M, Source: XTALC32M\n
    end
    if $val == 1
        echo Peri:      48M, Source: SYSPLL48M\n
    end
    if $val == 2
        echo Peri:      64M, Source: CLK64M\n
    end
end

# DAIF
if OM_CPM->ANA_IF_APB_CFG & 1
    echo DAIF:      off\n
else
    echo DAIF:      on\n
end

# SF0
if OM_CPM->SF0_CFG & 1
    echo SF0:       off\n
else
    echo SF0:       on\n
end

# SF1
if OM_CPM->OSPI1_CFG & 1
    echo SF1:       off\n
else
    echo SF1:       on\n
end

# GPIO0
if OM_CPM->GPIO_CFG & 1
    echo GPIO0:     off\n
else
    echo GPIO0:     on\n
end

# TIM0
if OM_CPM->TIM0_CFG & 1
    echo TIM0:      off\n
else
    echo TIM0:      on\n
end

# TIM1
if OM_CPM->TIM1_CFG & 1
    echo TIM1:      off\n
else
    echo TIM1:      on\n
end

# TIM2
if OM_CPM->TIM2_CFG & 1
    echo TIM2:      off\n
else
    echo TIM2:      on\n
end

# LPTIM
if OM_PMU->BASIC & (1 << 11)
    echo LPTIM 32K:     off\n
else
    echo LPTIM 32K:     on\n
end
if OM_CPM->LPTIM_CFG & 1
    echo LPTIM:      off\n
else
    echo LPTIM:      on\n
end

# UART0
if OM_CPM->UART0_CFG & 1
    echo UART0:     off\n
else
    echo UART0:     on\n
end

# UART1
if OM_CPM->UART1_CFG & 1
    echo UART1:     off\n
else
    echo UART1:     on\n
end

# I2C0
if OM_CPM->I2C0_CFG & 1
    echo I2C0:      off\n
else
    echo I2C0:      on\n
end

# I2C1
if OM_CPM->I2C1_CFG & 1
    echo I2C1:      off\n
else
    echo I2C1:      on\n
end

# SPI0
if OM_CPM->SPI0_CFG & 1
    echo SPI0:      off\n
else
    echo SPI0:      on\n
end

# SPI1
if OM_CPM->SPI1_CFG & 1
    echo SPI1:      off\n
else
    echo SPI1:      on\n
end

# BLE
if OM_CPM->BLE_CFG & 1
    echo BLE AHB:   off\n
else
    echo BLE AHB:   on\n
end
if OM_CPM->BLE_CFG & 2
    echo BLE MST:   on\n
else
    echo BLE MST:   off\n
end
if OM_CPM->BLE_CFG & 4
    echo BLE CLK:   on\n
else
    echo BLE CLK:   off\n
end
if OM_CPM->BLE_CFG & (1 << 9)
    echo BLE MAC:   off\n
else
    echo BLE MAC:   on\n
end

# 24G
if OM_CPM->MAC_24G_CFG & 1
    echo 24G:       off\n
else
    echo 24G:       on\n
end

# PHY
if OM_CPM->PHY_CFG & 3
    echo PHY:       off\n
else
    echo PHY:       on\n
end

# GPDMA
if OM_CPM->GPDMA_CFG & 1
    echo GPDMA:     off\n
else
    echo GPDMA:     on\n
end

# AES
if OM_CPM->AES_CFG & 1
    echo AES:       off\n
else
    echo AES:       on\n
end

# RNG
if OM_CPM->RNG_CFG & 1
    echo RNG:       off\n
else
    echo RNG:       on\n
end

# TRNG
if OM_CPM->TRNG_CFG & 1
    echo TRNG:      off\n
else
    echo TRNG:      on\n
end

# SHA256
if OM_CPM->SHA256_CFG & 1
    echo SHA256:    off\n
else
    echo SHA256:    on\n
end

# EFUSE
if OM_CPM->EFUSE_CFG & 1
    echo EFUSE:     off\n
else
    echo EFUSE:     on\n
end

# RTC
if OM_PMU->BASIC & (1 << 8)
    echo RTC PMU:   off\n
else
    echo RTC PMU:   on\n
end
if OM_CPM->APB_CFG & 2
    echo RTC APB:   off\n
else
    echo RTC APB:   on\n
end

# QDEC
if OM_CPM->QDEC_CFG & 1
    echo QDEC:      off\n
else
    echo QDEC:      on\n
end

# IRTX
if OM_CPM->IRTX_CFG & 1
    echo IRTX:      off\n
else
    echo IRTX:      on\n
end

# LCD
if OM_CPM->LCD_CFG & 1
    echo LCD:       off\n
else
    echo LCD:       on\n
end

# RGB
if OM_CPM->RGB_CFG & 1
    echo RGB:       off\n
else
    echo RGB:       on\n
end

# USB
if OM_CPM->USB_CFG & 0xa
    echo USB:       off\n
else
    echo USB:       on\n
end

# I2S
if OM_CPM->I2S_CFG & 0x1
    echo I2S:       off\n
else
    echo I2S:       on\n
end

# AUDIO
if OM_CPM->AUDIO_CFG & 0x1
    echo AUDIO:     off\n
else
    echo AUDIO:     on\n
end