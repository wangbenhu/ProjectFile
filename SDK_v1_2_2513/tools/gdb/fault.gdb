#fault--view Cortex Mx fault info

#CSFR--including MFSR, BFSR and UFSR
set $cfsr_reg = *(unsigned int*)0xE000ED28

#MFSR
if $cfsr_reg & (1<<7)
    printf "MemManage Fault: MMFAR = 0x%08x\n", *(unsigned int*)0xE000ED34
end
if $cfsr_reg & (1<<5)
    printf "MemMange Fault: Floating point lazy stacking error.\n"
end
if $cfsr_reg & (1<<4)
    printf "MemMange Fault: Stack error.\n"
end
if $cfsr_reg & (1<<3)
    printf "MemMange Fault: Unstacking error.\n"
end
if $cfsr_reg & (1<<1)
    printf "MemMange Fault: Data access violation.\n"
end
if $cfsr_reg & (1<<0)
    printf "MemMange Fault: Instruction access violation.\n"
end

#BFSR
if $cfsr_reg & (1<<15)
    printf "Bus Fault: BFAR = 0x%08x\n", *(unsigned int*)0xE000ED38
end
if $cfsr_reg & (1<<13)
    printf "Bus Fault: Floating point lazy stacking error.\n"
end
if $cfsr_reg & (1<<12)
    printf "Bus Fault: Stacking error.\n"
end
if $cfsr_reg & (1<<11)
    printf "Bus Fault: Unstacking error.\n"
end
if $cfsr_reg & (1<<10)
    printf "Bus Fault: Imprecise data access error.\n"
end
if $cfsr_reg & (1<<9)
    printf "Bus Fault: Precise data access error.\n"
end
if $cfsr_reg & (1<<8)
    printf "Bus Fault: Instructin access error.\n"
end

#UFSR
if $cfsr_reg & (1<<25)
    printf "Usage Fault: Indicates a divide by zero.\n"
end
if $cfsr_reg & (1<<24)
    printf "Usage Fault: Indicates that an unaligned access fault.\n"
end
if $cfsr_reg & (1<<20)
    printf "Usage Fault: Indicates stack overflow error(new in Cortex-M33).\n"
end
if $cfsr_reg & (1<<19)
    printf "Usage Fault: Attempts to execute a coprocessor instruction.\n"
end
if $cfsr_reg & (1<<18)
    printf "Usage Fault: Attempts to carry out an exception return with a bad value in EXC_RETURN number.\n"
end
if $cfsr_reg & (1<<17)
    printf "Usage Fault: Attempts to switch to an invalid state(e.g. ARM).\n"
end
if $cfsr_reg & (1<<16)
    printf "Usage Fault: Attempts to execute an undefined instruction.\n"
end

#HFSR
set $hfsr_reg = *(unsigned int*)0xE000ED2C
if $hfsr_reg & (1<<31)
    printf "Hard Fault: Indicate hard fault is triggered by debug event.\n"
end
if $hfsr_reg & (1<<30)
    printf "Hard Fault: Indicate hard fault is taken because of BusFault, MemoryManagementFault or UsageFault.\n"
end
if $hfsr_reg & (1<<1)
    printf "Hard Fault: Indicate hard fault is caused by failed vectior fetch.\n"
end

#DFSR
set $dfsr_reg = *(unsigned int*)0xE000ED30
if $dfsr_reg & (1<<4)
    printf "Debug Fault: Indicate the debug event is caused by an external signal.\n"
end
if $dfsr_reg & (1<<3)
    printf "Debug Fault: Indicate the debug event is caused by a vector catch.\n"
end
if $dfsr_reg & (1<<2)
    printf "Debug Fault: Indicate the debug event is caused by a watchpoint.\n"
end
if $dfsr_reg & (1<<1)
    printf "Debug Fault: Indicate the debug event is caused by a breakpoint.\n"
end
if $dfsr_reg & (1<<0)
    printf "Debug Fault: Indicate the processor is halted by debugger request.\n"
end

#fault context
if $lr & (0x04)
    set $sp_pos = $psp
else
    set $sp_pos = $msp
end
printf "SP is 0x%08x\n", $sp_pos
x/32xw $sp_pos
printf "CPU context: \n"
set $cpu_r0 = *(unsigned int*)$sp_pos
set $cpu_r1 = *(unsigned int*)($sp_pos+4)
set $cpu_r2 = *(unsigned int*)($sp_pos+8)
set $cpu_r3 = *(unsigned int*)($sp_pos+12)
set $cpu_r12 = *(unsigned int*)($sp_pos+16)
set $cpu_lr = *(unsigned int*)($sp_pos+20)
set $cpu_pc = *(unsigned int*)($sp_pos+24)
set $cpu_xpsr = *(unsigned int*)($sp_pos+28)

printf "R0=0x%08x,  R1=0x%08x, R2=0x%08x,  R3=0x%08x\n", $cpu_r0, $cpu_r1, $cpu_r2, $cpu_r3
printf "R12=0x%08x, LR=0x%08x, PC=0x%08x,  XPSR=0x%08x\n", $cpu_r12, $cpu_lr, $cpu_pc, $cpu_xpsr
