# ABI 兼容性

## 原错误

Keil 链接器 `L6242E` 报告 `wchar_t-16 clashes with wchar_t-32`。旧库同时采用 soft-float，和目标工程的 Cortex-M4F hard-float 设置不一致。

## 当前对象属性

| ELF 属性 | 当前值 |
|---|---|
| CPU | Arm v7E-M / Cortex-M4 |
| Thumb ISA | Thumb-2 |
| FPU | VFPv4-D16 |
| `Tag_ABI_PCS_wchar_t` | 2 bytes |
| `Tag_ABI_HardFP_use` | SP only |
| `Tag_ABI_VFP_args` | VFP registers |
| 枚举 ABI | small |
| 未解析外部符号 | 0 |

本包修复的是对象 ABI，不要求修改算法接口或 SDK 调用代码。若 SDK 后续切换为 soft-float、32 位 `wchar_t`、其他 CPU 或其他 FPU，必须重新编译库。
