# 内存报告

构建条件：GNU Arm Embedded GCC 13.2.1，Cortex-M4F / Thumb-2 / FPv4-SP-D16 / hard-float，16 位 `wchar_t`，`-Os`，C99。

| 项目 | 大小 |
|---|---:|
| 静态库归档文件 | 3,452 bytes |
| Cortex-M4F 可执行代码段 | 1,310 bytes |
| `pet_ai_context_size()` | 6,016 bytes |
| 其中六轴环形缓存 | 6,000 bytes |
| 库全局静态 RAM | 0 bytes |
| 保守静态调用链栈上限 | 324 bytes |

归档大小包含对象格式、符号表和元数据，不等于最终固件 Flash 增量。算法不引用浮点运行时、`libm`、堆、文件系统或打印函数。最终 Flash 和栈占用以 Keil 工程生成的 `.map` 文件复核。
