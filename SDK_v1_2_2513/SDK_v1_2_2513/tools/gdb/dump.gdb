# setting
set confirm off
set verbose off
set pagination off
set logging enabled off
set logging file dump.log
set logging overwrite on
set logging enabled on

# stop WDT
set *0x400e00e8 = 0x6666
printf "WDT is stoped!  0x400e00e8(WDT) = 0x%08X\n", *0x400e00e8

echo \n------------------\n
echo ANA IF Dump:\n
x/1x 0x4000105C
x/1x 0x40001064

# ana clock
set $cpm_ana_if_apb = *0x4000105C
set $cpm_ana_if     = *0x40001064
set *0x4000105C     = 0
set *0x40001064     = 0
set *0x4000100C     = 0x00000008
set *0x4000100C     = 0x00000003
set *0x4000100C     = 0x00000003

# dump ANA IF REG
x/105wx 0x400A0000
x/6wx 0x400A1000
x/24wx 0x400A2000
x/10wx 0x400A3000

# enable ana clock
set $daif_clk_en    = *0x400A00D0
set *0x400A00D0     = 0xFFFFFFFF

echo \n------------------\n
echo ANA IF DEBUG Dump:\n
set $daif_dbg_reg = *0x400A0100
set $i=0
while ($i < 16)
    set $value=0x10 | $i
    set *0x400A0100 = $value
    set $a = *0x400A0100
    set $b = *0x400A0100
    set $c = *0x400A0100
    set $d = *0x400A0100
    set $e = *0x400A0100
    set $f = *0x400A0100
    set $g = *0x400A0100
    set $h = *0x400A0100
    printf "%08X %08X %08X %08X %08X %08X %08X %08X\n", $a,$b,$c,$d,$e,$f,$g,$h
    set $i=$i+1
end
set *0x400A0100 = $daif_dbg_reg

echo \n------------------\n
echo ANA IF PLL LUT:\n
set $save=*0x400A00D4
set $i=0
while ($i < 80)
    set *0x400A00D4 = (($i+0) << 16) | (1<<28)
    set $a = *0x400A00D4
    set *0x400A00D4 = (($i+1) << 16) | (1<<28)
    set $b = *0x400A00D4
    set *0x400A00D4 = (($i+2) << 16) | (1<<28)
    set $c = *0x400A00D4
    set *0x400A00D4 = (($i+3) << 16) | (1<<28)
    set $d = *0x400A00D4
    set *0x400A00D4 = (($i+4) << 16) | (1<<28)
    set $e = *0x400A00D4
    set *0x400A00D4 = (($i+5) << 16) | (1<<28)
    set $f = *0x400A00D4
    set *0x400A00D4 = (($i+6) << 16) | (1<<28)
    set $g = *0x400A00D4
    set *0x400A00D4 = (($i+7) << 16) | (1<<28)
    set $h = *0x400A00D4
    printf "%08X %08X %08X %08X %08X %08X %08X %08X\n", $a,$b,$c,$d,$e,$f,$g,$h
    set $i=$i+8
end
set *0x400A00D4=$save

echo \n------------------\n
echo ANA IF DCOC LUT:\n
x/64wx 0x400A1000

echo \n------------------\n
echo ANA IF TIA DCOC LUT:\n
x/10wx 0x400A4000

echo \n------------------\n
echo RC32K PPM:\n
define rc32k_accuracy_check
    set $__rc32k_calib_ms = $arg0
    set $__win_32k_num = 32768 * $__rc32k_calib_ms / 1000
    set *0x400A019C = 7 | ($__win_32k_num << 8)
    while (*0x400A019C & 1) == 1
    end
    monitor sleep 1
    set $__clk_num_32m_std = $__win_32k_num * (long long)32000000 / 32768
    set $__clk_num_32m = *0x400A01A0
    set $__rc32k_ppm = (long long)1000000 * ($__clk_num_32m - $__clk_num_32m_std) / $__clk_num_32m_std
    printf "rc32k=%dppm (%dms)\n", $__rc32k_ppm, $__rc32k_calib_ms
end
rc32k_accuracy_check 10
rc32k_accuracy_check 100
rc32k_accuracy_check 1000

# restore
set *0x400A00D0=$daif_clk_en
set *0x4000105C=$cpm_ana_if_apb
set *0x40001064=$cpm_ana_if
set *0x4000100C = 0x00000008
set *0x4000100C = 0x00000003

echo \n------------------\n
echo PMU Dump:\n
x/128wx 0x400e0000

echo \n------------------\n
echo CPM Dump:\n
x/50wx 0x40001000

echo \n------------------\n
echo GPIO0 Dump:\n
x/18wx 0x41200000
echo GPIO1 Dump:\n
x/18wx 0x41202000

echo \n------------------\n
echo PHY Dump:\n
x/1x 0x4000104C
set $save = *0x4000104C
set *0x4000104C = 0
    x/89wx 0x40020000
set *0x4000104C = $save

echo \n------------------\n
echo 2.4G Dump:\n
x/1x 0x40001060
set $save = *0x40001060
set *0x40001060 = 0
    x/88x 0x4000b000
set *0x40001060 = $save

echo \n------------------\n
echo SYS Dump:\n
x/33wx 0x40000000

echo \n------------------\n
echo SF Dump:\n
x/22wx 0x51000000

echo \n------------------\n
echo NVIC Dump EN:\n
x/8wx (0xE000E100 + 0x000)

echo NVIC Dump Pending:\n
x/8wx (0xE000E100 + 0x100)

echo NVIC Dump Active:\n
x/8wx (0xE000E100 + 0x200)

echo NVIC Dump Priority:\n
x/16wx (0xE000E100 + 0x300)

echo NVIC Dump Soft_Trig:\n
x/wx (0xE000E100 + 0xE00)

echo \n------------------\n
echo SCB Dump:\n
x/36wx 0xE000ED00

echo \n------------------\n
echo Baseband Dump:\n
if (*0x40001030 & 0x00000004)
    echo IP:\n
    x/160wx 0x41300000
    echo Exchange table:\n
    x/64wx 0x41310000
    dump memory em.ebin 0x41310000 0x41310800

    echo Debug Diag:\n
    set $i=0
    while ($i < 0x80)
        printf "0x%02X: ", $i
        set $j=0
        while ($j < 0x10)
            set *0x41300050 = (1<<7) | $i
            set $a = *(unsigned char *)0x41300054
            printf "%02X ", $a
            set $j=$j+1
            set $i=$i+1
        end
        printf "\n"
    end
    set *0x41300050 = 0
else
    echo Sleeped\n
end

echo \n------------------\n
echo CPU REG Dump:\n
info registers

# dump ram / flash
echo \n------------------\n
echo Dump SRAM (128kB)...\n
dump memory sram.bin 0x20000000 0x20020000
echo Dump IFLASH (1024kB)...\n
dump memory iflash.bin 0x00400000 0x00500000
