HAL_CMSIS_DIR = $(HAL_DIR)/CMSIS

HAL_CMSIS_DEFS =
HAL_CMSIS_INCS = $(HAL_CMSIS_DIR)/Core/Include
HAL_CMSIS_SRCS =

ifeq ($(CONFIG_CMSIS_DSP), y)
HAL_CMSIS_DEFS += ARM_MATH_LOOPUNROLL=1

HAL_CMSIS_INCS += $(HAL_CMSIS_DIR)/DSP/Include
HAL_CMSIS_INCS += $(HAL_CMSIS_DIR)/DSP/Include/dsp
HAL_CMSIS_INCS += $(HAL_CMSIS_DIR)/DSP/Privateinclude

HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/BasicMathFunctions/BasicMathFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/BayesFunctions/BayesFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/CommonTables/CommonTables.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/ComplexMathFunctions/ComplexMathFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/ControllerFunctions/ControllerFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/DistanceFunctions/DistanceFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/FastMathFunctions/FastMathFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/FilteringFunctions/FilteringFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/InterpolationFunctions/InterpolationFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/MatrixFunctions/MatrixFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/QuaternionMathFunctions/QuaternionMathFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/StatisticsFunctions/StatisticsFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/SupportFunctions/SupportFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/SVMFunctions/SVMFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/TransformFunctions/TransformFunctions.c
HAL_CMSIS_SRCS += $(HAL_CMSIS_DIR)/DSP/Source/WindowFunctions/WindowFunctions.c
endif

ifeq ($(CONFIG_CMSIS_NN), y)
HAL_CMSIS_INCS += $(HAL_CMSIS_DIR)/NN/Include
HAL_CMSIS_INCS += $(HAL_CMSIS_DIR)/NN/Include/Internal

HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/ActivationFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/BasicMathFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/ConcatenationFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/ConvolutionFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/FullyConnectedFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/LSTMFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/NNSupportFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/PoolingFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/ReshapeFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/SoftmaxFunctions/*.c)
HAL_CMSIS_SRCS += $(wildcard $(HAL_CMSIS_DIR)/NN/Source/SVDFunctions/*.c)
endif
