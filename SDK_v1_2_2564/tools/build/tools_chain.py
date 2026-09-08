import os
import platform
import re

ToolsChain=dict()

#
# Set dictionary variable
#
def SetToolsChain(cpu, project_name, cross_compiler='gcc', **cc_option):
    global ToolsChain

    # Initialize dictionary
    ToolsChain = {'CC': None, 'CFLAGS': None, 'CXX': None, 'CXXFLAGS': None,
                    'AS': None, 'ASFLAGS': None, 'LINK': None, 'LINKFLAGS':None,
                    'AR': None, 'ARFLAGS':None, 'EXEC_PATH':None, 'TARGET_SUFFIX':None,
                    'POST_ACTION':None, 'PLATFORM' :None, 'LIBPREFIX' :None, 'LIBSUFFIX' :None}

    # ARMGCC
    if cross_compiler == 'gcc':
        # KEY: PLATFORM
        ToolsChain['PLATFORM'] = 'gcc'
        # KEY: CC
        ToolsChain['CC'] = 'arm-none-eabi-gcc'
        # KEY: AS
        ToolsChain['AS'] = 'arm-none-eabi-gcc -x assembler-with-cpp -c'
        # KEY: LINK
        ToolsChain['LINK'] = 'arm-none-eabi-gcc'
        # KEY: AR
        ToolsChain['AR'] = 'arm-none-eabi-ar'
        # KEY: CXX
        ToolsChain['CXX'] = 'arm-none-eabi-g++'
        # KEY: TARGET_SUFFIX
        ToolsChain['TARGET_SUFFIX'] = '.elf'
        # KEY: LIBPREFIX
        ToolsChain['LIBPREFIX'] = 'lib'
        # KEY: LIBSUFFIX
        ToolsChain['LIBSUFFIX'] = '.a'

        # Bin utils
        OBJCOPY = 'arm-none-eabi-objcopy'
        OBJDUMP = 'arm-none-eabi-objdump'
        OBJSIZE = 'arm-none-eabi-size'

        # Device
        DEVICE = '-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard'

        # KEY: CFLAGS
        ToolsChain['CFLAGS'] = DEVICE + ' -std=gnu99 -fomit-frame-pointer -Wall -Wformat=0 -Wstrict-aliasing=0 -ffunction-sections -fdata-sections -g3 '
        # optimization level
        if 'cc_opt_level' in cc_option:
            cc_opt = ['-O0', '-O1', '-O2', '-O3', '-Os', '-Ofast', '-Og']
            if cc_option['cc_opt_level'] > len(cc_opt):
                raise Exception('Toolchain donot supported optimization level')
            ToolsChain['CFLAGS'] += cc_opt[cc_option['cc_opt_level']]
        else:
            ToolsChain['CFLAGS'] += '-Os'

        # KEY: CXXFLAGS
        ToolsChain['CXXFLAGS'] = ToolsChain['CFLAGS']
        # KEY: ASFLAGS
        ToolsChain['ASFLAGS'] = DEVICE + ' -nostartfiles'
        # KEY: ARFLAGS
        ToolsChain['ARFLAGS'] = '-rc'

        # KEY: LINKFLAGS
        ToolsChain['LINKFLAGS'] = ''
        if 'SDK_DIR' in cc_option:
            ToolsChain['LINKFLAGS'] += ' -L ' + cc_option['SDK_DIR'] + '/hal/device/GCC '
        ToolsChain['LINKFLAGS'] += DEVICE + ' -nostartfiles -static --specs=nano.specs -lc -lnosys -lm -Wl,--gc-sections '

        # fixeda LOAD segment with RWX permissions
        gcc_ver_info = os.popen(ToolsChain['CC'] + ' --version')
        gcc_ver_match = re.search(r'\b\d+\.\d+\b', gcc_ver_info.readlines()[0])
        if float(gcc_ver_match.group()) >= 12.2:
            ToolsChain['LINKFLAGS'] += ' -Wl,--no-warn-rwx-segments'

        # KEY: POST_ACTION
        if project_name:
            ToolsChain['POST_ACTION'] = OBJCOPY + ' -O binary $TARGET ' + project_name + '.bin \n' + OBJCOPY + ' -O ihex $TARGET ' + project_name + '.hex \n' + OBJDUMP + ' -d $TARGET >' + project_name + '.dis \n' + OBJSIZE + ' $TARGET \n'
        else:
            ToolsChain['POST_ACTION'] = OBJSIZE + ' $TARGET \n'
#
# Get dictionary variable
#
def GetToolsChain() :
    return ToolsChain

#
# Main Entry
#
if __name__ == '__main__':
    SetToolsChain()
    print(ToolsChain)
    SetToolsChain(cpu='cm4f')
    print(ToolsChain)
    print(GetToolsChain())
