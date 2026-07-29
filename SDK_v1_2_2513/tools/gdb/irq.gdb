#vector
printf "VTOR reg = 0x%08x, vector table base = 0x%08x\n", *((unsigned int *)0xE000ED08), *((unsigned int *)0xE000ED08)

set $nvic_num = EXTERNAL_IRQn_Num

#nvic
printf "Num   En    Pend   Prio    Act    Ns   Type\n"
set $i = 0
set $NVIC_BASE = 0xE000E100U
while $i < $nvic_num
    set $group = ($i >> 5)
    set $index = ($i & 0x1F)
    set $mask = (1 << $index)
    set $en = (((NVIC_Type *)$NVIC_BASE)->ISER[$group] & $mask) ? 1 : 0
    set $pending = (((NVIC_Type *)$NVIC_BASE)->ISPR[$group] & $mask) ? 1 : 0
    set $active = (((NVIC_Type *)$NVIC_BASE)->IABR[$group] & $mask) ? 1 : 0
    set $prio = (((NVIC_Type *)$NVIC_BASE)->IPR[$i])

    printf "%02d    %d     %d      %02d      %d      ", $i, $en, $pending, $prio, $active
    output (IRQn_Type)$i
    printf "\n"
    set $i = $i + 1
end

#special registers
if $primask
    printf "PRIMASK reg = %d, All exceptions except NMI and HardFault are DISABLED.\n", $primask
else
    printf "PRIMASK reg = %d, All exceptions except NMI and HardFault are ENABLED.\n", $primask
end
if $faultmask
    printf "FAULTMASK reg = %d, All exceptions are DISABLED.\n", $faultmask
else
    printf "FAULTMASK reg = %d, All exception are ENABLED.\n", $faultmask
end
if $basepri
    printf "BASEPRI reg = %d, All exceptions with priority > %02d would DISABLED.\n", $basepri, $basepri
else
    printf "BASEPRI reg = %d, All exceptions are ENABLED.\n", $basepri
