# Keil AC6 接入说明

## 文件

- 头文件：`include/pet_behavior_ai.h`
- 静态库：`lib/keil/ac6/cortex-m4f-hardfp/libpet_behavior.a`

该归档由 GNU Arm Embedded GCC 13.2.1 生成，但对象的 Arm EABI 属性按目标 SDK 设置为 Cortex-M4F、FPv4-SP-D16、hard-float 和 16 位 `wchar_t`。Keil ARM Compiler 6 链接器能够读取 GNU `ar` 归档；文件必须保持 `.a` 后缀，不能改名伪装成 `.lib`。

## 对应 SDK 设置

- ARM Compiler：6.19
- CPU：Cortex-M4F
- FPU：FPv4-SP-D16 / Single Precision
- 浮点 ABI：hard
- 运行库：MicroLIB
- 目标字节序：Little Endian

对象的等效编译参数为：`-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fshort-wchar -mabi=aapcs`。

调用顺序、状态区分配和断流复位规则见 `protocol.md`。库没有未解析外部符号，不调用标准 C 库、浮点运行时或数学库。
