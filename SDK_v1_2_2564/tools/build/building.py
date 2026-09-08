#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import os
import shutil
from SCons.Script import *
import build.tools_chain as tools_chain
import platform
from pathlib import Path

#config option
ConfigOptions = dict()
Projects = list()
Env = None
Project_name = None

def GetCurrentDir():
    conscript = File('SConscript')
    fn = conscript.rfile()
    path = os.path.dirname(fn.abspath)
    return path

def GetCurrentRelDir():
    path = os.path.relpath(GetCurrentDir(), str(Dir('#')))
    return path

#
# List
#
def List(defs):
    # srcs
    print('-'*75 + '\n[SRCS]:')
    for group in Projects:
        if 'srcs' in group:
            srcs = group['srcs']
            for src in srcs:
                print(os.path.relpath(src))
    # incs
    print('-'*75 + '\n[INCS]:')
    for group in Projects:
        if 'incs' in group:
            incs = group['incs']
            for inc in incs:
                print(os.path.relpath(inc))
    # libs
    print('-'*75 + '\n[LIBS]:')
    for group in Projects:
        if 'libs' in group:
            libs = group['libs']
            for lib in libs:
                print(os.path.relpath(lib))
    # defs
    print('-'*75 + '\n[DEFS]:')
    for define in defs:
        print(define)

    # link script
    print('-'*75 + '\n[LINK_SCRIPT]:')
    for group in Projects:
        if 'LINKER_SCRIPT' in group:
            scripts = group['LINKER_SCRIPT']
            print(os.path.relpath(scripts))

#
# Distclean
#
def DistClean(dir=None):
    print('clean: .build, *.elf, *.bin, *.map, *.dis')
    if dir is None:
        dir = GetCurrentDir()
    filenames = os.listdir(dir)
    del_files = []
    del_dir = ".build"

    # delete specify directory
    if os.path.exists(del_dir):
        shutil.rmtree(del_dir)

    # deleted *.elf, *.bin, *.map, *.dis
    for filename in filenames:
        if filename.endswith(('.elf', '.bin', '.map', '.dis')):
            del_files += [filename]

    for del_file in del_files:
        os.remove(del_file)

#
# Read Config Option
#
def ReadConfigOption(config_file):
    # check files
    if not os.path.isfile(config_file):
        raise Exception("No config file")

    f_cfg=open(config_file)
    cfg_txt = f_cfg.readlines()
    cfg_used_txt=[]
    for i in cfg_txt:
        if i[0] in ['#', '\r','\n']:
            pass
        else:
            cfg_used_txt.append(i)
    contents = ''
    config_defs = ''
    for i in cfg_used_txt:
        element = i.split('=')
        if('y' == element[1].strip()):
            contents += "#define {} {}".format(element[0], '1\n')
            config_defs += "{}={}".format(element[0], '1\n')
        else:
            contents += "#define {} {}\n".format(element[0], element[1])
            config_defs += "{}={}\n".format(element[0], element[1])
    prep = SCons.cpp.PreProcessor()
    prep.process_contents(contents)
    options = prep.cpp_namespace
    ConfigOptions.update(options)
    return config_defs

#
# GET Depend
#
def GetDepend(depend):
    if isinstance(depend, str):
        if (depend not in ConfigOptions) or (ConfigOptions[depend] == 0):
            return False
        elif ConfigOptions[depend] != '':
            return ConfigOptions[depend]
        else:
            return True
    else:
        raise Exception('Value Error')

#
# Delete Depend
#
def DeleteDepend(depend):
    if isinstance(depend, str):
        if depend in ConfigOptions:
            del ConfigOptions[depend]
    else:
        raise Exception('Value Error')

#
# Set Depend
#
def SetDepend(depend, value):
    if isinstance(depend, str):
        ConfigOptions[depend] = value
    else:
        raise Exception('Value Error')

#
# Build library file handler
#
def GroupLibName(name):
    if tools_chain.ToolsChain['PLATFORM'] == 'gcc':
        return name + '_gcc'
    elif tools_chain.ToolsChain['PLATFORM'] == 'armclang':
        return name + '_rvds'

    return name

def GroupLibFullName(name):
    return tools_chain.ToolsChain['LIBPREFIX'] + GroupLibName(name) + tools_chain.ToolsChain['LIBSUFFIX']

def BuildLibAction(target, source, env):
    print('Generate ' + GroupLibFullName(Project_name))

def GetKeilIarOptim():
    if GetDepend('CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL'):
        opt_level = GetDepend('CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL')
    else:
        opt_level = 3

    gcc_opt = ['-O0', '-O1', '-O2', '-O3', '-Os', '-Ofast', '-Og']
    keil_opt = ['-O0', '-O1', '-O2', '-O3', '-Ofast', '-Os balanced', '-Oz image size']
    iar_opt = ['None', 'Low', 'Medium', 'High Balanced', 'High Size', 'High Speed']
    keil_opt_dict = {gcc_opt[0]: keil_opt[0], gcc_opt[1]: keil_opt[1],
                     gcc_opt[2]: keil_opt[2], gcc_opt[3]: keil_opt[3],
                     gcc_opt[4]: keil_opt[6], gcc_opt[5]: keil_opt[4],
                     gcc_opt[6]: keil_opt[5]}

    iar_opt_dict = {gcc_opt[0]: iar_opt[0], gcc_opt[1]: iar_opt[1],
                    gcc_opt[2]: iar_opt[2], gcc_opt[3]: iar_opt[3],
                    gcc_opt[4]: iar_opt[4], gcc_opt[5]: iar_opt[5],
                    gcc_opt[6]: iar_opt[3]}
    keil_optim = keil_opt_dict[gcc_opt[opt_level]]
    iar_optim = iar_opt_dict[gcc_opt[opt_level]]

    return keil_optim, iar_optim
#
# Define Group Dictionary
#
def DefineGroupByDir():
    cwd = GetCurrentDir()
    dirs = os.listdir(cwd)
    group = []
    for d in dirs:
        path = os.path.join(cwd, d)
        if os.path.isfile(os.path.join(path, 'SConscript')):
            group += SConscript(os.path.join(d, 'SConscript'))
    return [group]

# parameter CPPDEFINES CCFLAGS  ASFLAGS LINKFLAGS LINKER_SCRIPT
def DefineGroup(name, srcs=None, incs=None, libs=None, glibs=None, **parameter):
    global Projects

    group_path = GetCurrentDir()
    # added parameter check
    # TODO:

    group = parameter
    group['name'] = name
    group['path'] = group_path
    root_dir = str(Dir('#'))

    # srcs
    if srcs is not None:
        group['srcs'] = []
        group['Srcs'] = []
        for src in srcs:
            norm_src = os.path.normpath(src)
            abs_src = os.path.join(group['path'], norm_src)
            group['srcs'].append(os.path.relpath(abs_src, root_dir))
            group['Srcs'] = File(srcs)

    # normpath incs
    if incs is not None:
        group['incs'] = []
        for inc in incs:
            norm_inc = os.path.normpath(inc)
            abs_inc = os.path.join(group['path'], norm_inc)
            group['incs'].append(os.path.relpath(abs_inc, root_dir))
    # normpath libs
    if libs is not None:
        group['libs'] = []
        if isinstance(libs, str):    # str type
            norm_lib = os.path.normpath(libs)
            abs_lib = os.path.join(group['path'], norm_lib)
            group['libs'].append(os.path.relpath(abs_lib, root_dir))
        else :    # tuple or list type
            for lib in libs:
                norm_lib = os.path.normpath(lib)
                abs_lib = os.path.join(group['path'], norm_lib)
                group['libs'].append(os.path.relpath(abs_lib, root_dir))
    # # generated libs
    # if glibs == True:
    #     lib_name = GroupLibName(group['name'])
    #     objects = Env.Object(group['Srcs'])
    #     program = Env.Library(lib_name, objects)
    #     # Env.BuildLib(lib_name, program)

    # normpath
    if 'LINKER_SCRIPT' in parameter and parameter['LINKER_SCRIPT']:
        norm_ld = os.path.normpath(parameter['LINKER_SCRIPT'])
        abs_ld = os.path.join(group['path'], norm_ld)
        group['LINKER_SCRIPT'] = os.path.relpath(abs_ld, root_dir)

    # deleted SDK file if repeat in projects
    appended = False
    for i in range(len(Projects)):
        check_group = Projects[i]
        if group['name'] == check_group['name']:
            appended = True
            if check_group['name'].startswith(os.environ['SDK_DIR']):  # check_group is locate in SDK
                Projects[i]=group
    if not appended:
        Projects.append(group)

    return [group]

#
# Prepared Building Operate
#
def PrepareBuild(sdk_dir=None):
    global Env
    global Project_name

    # Assignment SDK directory
    if sdk_dir is None:
        sdk_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    os.environ['SDK_DIR'] = sdk_dir

    # Read device supported list
    with open(os.path.join(sdk_dir, 'tools/build/supported_devices.txt'), 'r') as file:
        supported_devices = file.read().split()

    # Add options
    AddOption('--menuconfig',
            dest = 'menuconfig',
            action = 'store_true',
            default = False,
            help = 'menu config')
    AddOption('--verbose',
            dest = 'verbose',
            action = 'store_true',
            default = False,
            help = 'verbose mode')
    AddOption('--list',
            dest = 'list',
            action = 'store_true',
            default = False,
            help = 'list sources, includes, defines and libraries')
    AddOption('--keil5',
            dest = 'keil5',
            action = 'store_true',
            default = False,
            help = 'generate & build keil5 project')
    AddOption('--iar',
            dest = 'iar',
            action = 'store_true',
            default = False,
            help = 'generate & build iar project')
    AddOption('--distclean',
            dest = 'distclean',
            action = 'store_true',
            default = False,
            help = 'clean generated files in project build, maybe used with --keil5/iar')
    AddOption('--device',
            dest = 'device',
            action = 'store',
            default = supported_devices[0],
            help = 'Choice devices in: ' + ' '.join(supported_devices) + ', default is ' + supported_devices[0])
    AddOption('--gen',
            dest = 'gen',
            action = 'store_true',
            default = False,
            help = 'generate keil5/iar project, must be used with --keil5/iar')

    # Handle device option
    device = GetOption('device')
    if (device.lower() not in supported_devices) :
        raise Exception(f'the {device} device not in $(SDK_DIR)/tools/build/supported_devices.txt, please selected in {supported_devices}')

    # Handle project name
    ReadConfigOption('.config')
    Project_name = os.path.split(os.getcwd())[1]

    # Handle menuconfig option
    if GetOption('menuconfig'):
        os.environ['KCONFIG_CONFIG'] = '.config'
        cmd = 'menuconfig ' + 'Kconfig'
        os.system(cmd)
        cmd = 'genconfig ' + 'Kconfig' + ' --header-path ' + 'config/autoconf.h'
        os.system(cmd)
        exit(0)

    # Handle distclean option
    if GetOption('distclean'):
        if GetOption('keil5') or GetOption('iar'):
            pass
        else:
            DistClean()
            exit(0)

    # Defines toolchain
    if GetOption('keil5'):
        SetDepend('CONFIG_TOOL_CHAIN_ARMCLANG', 1)
    elif GetOption('iar'):
        SetDepend('CONFIG_TOOL_CHAIN_ICCARM', 1)
    else:
        SetDepend('CONFIG_TOOL_CHAIN_GCC', 1)

    cpu = ''
    defines = [device.upper() + '=1']

    variant_dir = '.build'
    SConscript('SConscript', variant_dir=variant_dir, duplicate=False)
    variant_dir = os.path.join(variant_dir, 'SDK')
    SConscript(os.path.join(sdk_dir, 'SConscript'), variant_dir=variant_dir, duplicate=False)

    if GetDepend('CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL'):
        opt_level = GetDepend('CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL')
    else:
        opt_level = 3

    if not GetOption('keil5') and not GetOption('iar'):
        with os.popen('svnversion -n ' + sdk_dir) as version:
            try:
                version = version.readlines()[0]
                if version != 'Unversioned directory':
                    defines += ['__SDK_VERSION=' + '\\"' + version + '\\"']
            except Exception:
                print('warning: can not get SDK subversion')
        with os.popen('svnversion -n ' + os.getcwd()) as version:
            try:
                version = version.readlines()[0]
                if version != 'Unversioned directory':
                    defines += [('__PROJECT_VERSION=' + '\\"' + version + '\\"')]
            except Exception:
                print('warning: can not get PROJECT subversion')

    tools_chain.SetToolsChain(cpu=cpu, project_name=Project_name, cross_compiler='gcc', cc_opt_level=opt_level, SDK_DIR=sdk_dir)
    tools=tools_chain.GetToolsChain()
    Env = Environment(CC=tools['CC'], CFLAGS=tools['CFLAGS'],
                      CXX=tools['CXX'], CXXFLAGS=tools['CXXFLAGS'],
                      AS=tools['AS'], ASFLAGS=tools['ASFLAGS'],
                      AR=tools['AR'], ARFLAGS=tools['ARFLAGS'],
                      LINK=tools['LINK'], LINKFLAGS=tools['LINKFLAGS'],
                      PROGSUFFIX=tools['TARGET_SUFFIX'], PLATFORM='posix')
    Env.Append(CPPDEFINES=defines)

    if not Env.GetOption('verbose'):
        Env['CCCOMSTR'] = 'CC    $SOURCE'
        Env['ASCOMSTR'] = 'AS    $SOURCE'
        Env['ASPPCOMSTR'] = 'AS    $SOURCE'
        Env['LINKCOMSTR'] = 'Linking $TARGET'

    # add library(gcc) build action
    act = SCons.Action.Action(BuildLibAction, 'Install compiled library... $TARGET')
    bld = Builder(action = act)
    Env.Append(BUILDERS = {'BuildLib': bld})

    if GetDepend("CONFIG_NON_RTOS"):
        shell_port = os.path.join(sdk_dir, 'components/shell/shell_port_nonrtos.c')
    else:
        shell_port = os.path.join(sdk_dir, 'components/shell/shell_port_rtos.c')
    Execute(Touch(shell_port))
    Env.Decider('timestamp-match')

    # fixed long cmd lines on win32: https://github.com/SCons/scons/wiki/LongCmdLinesOnWin32
    if platform.system() == 'Windows':
        import win32file
        import win32event
        import win32process
        import win32security

        def my_spawn(sh, escape, cmd, args, spawnenv):
            for var in spawnenv:
                spawnenv[var] = spawnenv[var].encode('ascii', 'replace')

            sAttrs = win32security.SECURITY_ATTRIBUTES()
            StartupInfo = win32process.STARTUPINFO()
            newargs = ' '.join(map(escape, args[1:]))
            cmdline = cmd + " " + newargs

            # check for any special operating system commands
            if cmd == 'del':
                for arg in args[1:]:
                    win32file.DeleteFile(arg)
                exit_code = 0
            else:
                # otherwise execute the command.
                hProcess, hThread, dwPid, dwTid = win32process.CreateProcess(None, cmdline, None, None, 1, 0, spawnenv, None, StartupInfo)
                win32event.WaitForSingleObject(hProcess, win32event.INFINITE)
                exit_code = win32process.GetExitCodeProcess(hProcess)
                win32file.CloseHandle(hProcess)
                win32file.CloseHandle(hThread)
            return exit_code

        os.environ['SPAWN'] = 'my_spawn'

#
# Make Building Operate
#
def MakeBuilding():
    global Env
    global Projects

    sources = []
    linker_script = None

    Env.PrependENVPath('PATH', os.environ['PATH'])

    # Traverse group in projects
    for group in Projects:
        # Assignment link script
        if 'LINKER_SCRIPT' in group and not group['LINKER_SCRIPT'] is None:
            if linker_script is None:
                linker_script = group['LINKER_SCRIPT']
            else:
                print('the 1st defined linker script is', linker_script)
                print('The 2nd defined linker script is', group['LINKER_SCRIPT'])
                raise ValueError('multi-define linker script')

        # Assignment header files path
        if 'incs' in group:
            Env.Append(CPPPATH=group['incs'])

        # Assignment sources
        if 'srcs' in group:
            for src in group['Srcs']:
                sources.append(src)

        # Assignment library files
        if 'libs' in group:
            for lib in group['libs']:
                lib_path, lib_name = os.path.split(lib)
                Env.Append(LIBS=[lib_name])
                Env.Append(LIBPATH=[lib_path])

        # Assignment group defines
        if 'CPPDEFINES' in group:
            Env.Prepend(CPPDEFINES=group['CPPDEFINES'])
        if 'CFLAGS' in group:
            Env.Append(CFLAGS=group['CFLAGS'])
        if 'CCFLAGS' in group:
            Env.Append(CCFLAGS=group['CCFLAGS'])
        if 'ASFLAGS' in group:
            Env.Append(ASFLAGS=group['ASFLAGS'])
        if 'LINKFLAGS' in group:
            Env.Append(LINKFLAGS=group['LINKFLAGS'])

    includes = ''
    for include in Env['CPPPATH']:
        includes = includes + ' -I' + include
    defines = ''
    for define in Env['CPPDEFINES']:
        defines += ' -D' + define

    Env['ASFLAGS'] = Env['ASFLAGS'] + includes + defines

    # Basic library append to end
    Env.Append(LIBS=['libm.a'])

    # Check parameters validity
    if len(sources) == 0:
        raise ValueError('Donot have sources')
    if linker_script is None:
        raise ValueError('Please sets linker script')

    # Assignment linker script generate MAP file
    LINKFLAGS = ' -T ' + linker_script
    LINKFLAGS += ' -Wl,-Map=' + Project_name + '.map'
    Env.Append(LINKFLAGS=LINKFLAGS)

    # Handle option
    if Env.GetOption('list'):
        List(Env['CPPDEFINES'])
        exit(0)

    device = GetOption('device')
    keil_optim, iar_optim = GetKeilIarOptim()
    if Env.GetOption('keil5'):
        import build.keil_build.gen_keil_project as keil
        inifile_path = ''
        lto = 0
        if Env.GetOption('gen'):
            keil_project = keil.gen_keil5_project(device, Projects, Project_name, Env['CPPDEFINES'], inifile_path, keil_optim, lto, False)
        else:
            keil_project = os.path.join(os.getcwd(), 'keil5/' + Project_name + '.uvprojx')

        if Env.GetOption('distclean'):
            # os.system('embuild ' + keil_project + ' c')
            del_dir = os.path.join(os.path.dirname(keil_project), 'out')
            os.system(f'embuild {del_dir} delete')
            exit(0)

        print('building keil project...')
        os.system('embuild ' + keil_project)
        #os.system('axf2elf.sh keil5/out/' + Project_name + '.axf' + ' keil5/out_elf')
        print("generated .elf in keil5/out_elf")
        exit(0)

    if Env.GetOption('iar'):
        import build.iar_build.gen_iar_project as iar
        macfile_path = ''
        if Env.GetOption('gen'):
            iar_project = iar.gen_iar_project(device, Projects, Project_name, Env['CPPDEFINES'], macfile_path, 0, iar_optim, False)
        else:
            iar_project = os.path.join(os.getcwd(), 'iar/' + Project_name + '.ewp')

        if Env.GetOption('distclean'):
            del_dir = os.path.join(os.path.dirname(iar_project), Path(iar_project).stem)
            os.system(f'embuild {del_dir} delete')
            exit(0)
        print('building iar project...')
        os.system('embuild ' + iar_project)
        exit(0)

    # Assignment target elf
    target_elf = Env.Program(Project_name, sources)
    Env.Depends(target_elf, linker_script)
    Env.AddPostAction(target_elf, tools_chain.ToolsChain['POST_ACTION'])

    print('prepare build ' + Project_name + ' for ' + device)

    # Clean
    Clean(target_elf, Project_name + '.map')
    Clean(target_elf, Project_name + tools_chain.ToolsChain['TARGET_SUFFIX'])
    Clean(target_elf, Project_name + '.bin')
    Clean(target_elf, Project_name + '.hex')
    Clean(target_elf, '__pycache__')
    Clean(target_elf, '.sconsign.dblite')

#
# Make Building Lib Operate
#
def MakeLibBuilding():
    global Env
    global Projects

    sources = []

    Env.PrependENVPath('PATH', os.environ['PATH'])

    for group in Projects:
        # Assignment header files path
        if 'incs' in group:
            Env.Append(CPPPATH=group['incs'])

        # Assignment sources
        if 'srcs' in group:
            for src in group['Srcs']:
                sources.append(src)

        # Assignment library files
        if 'libs' in group:
            for lib in group['libs']:
                lib_path, lib_name = os.path.split(lib)
                Env.Append(LIBS=[lib_name])
                Env.Append(LIBPATH=[lib_path])

        # Assignment group defines
        if 'CPPDEFINES' in group:
            Env.Prepend(CPPDEFINES=group['CPPDEFINES'])
        if 'CFLAGS' in group:
            Env.Append(CFLAGS=group['CFLAGS'])
        if 'CCFLAGS' in group:
            Env.Append(CCFLAGS=group['CCFLAGS'])
        if 'ASFLAGS' in group:
            Env.Append(ASFLAGS=group['ASFLAGS'])
        if 'LINKFLAGS' in group:
            Env.Append(LINKFLAGS=group['LINKFLAGS'])

    device = GetOption('device')
    keil_optim, iar_optim = GetKeilIarOptim()
    # genarate keil lib
    if Env.GetOption('keil5'):
        import build.keil_build.gen_keil_project as keil
        lto = 1
        keil_project = keil.gen_keil5_project(device, Projects, Project_name, Env['CPPDEFINES'], '', 0, keil_optim, lto, True)
        print('keil project generated: ' +  keil_project)
        if Env.GetOption('distclean'):
            del_dir = os.path.join(os.path.dirname(keil_project), 'out')
            os.system(f'embuild {del_dir} delete')
            exit(0)
        if not (Env.GetOption('nobuild')):
            print('building keil project...')
            os.system('embuild ' + keil_project)
        exit(0)

    elif Env.GetOption('iar'):
        import build.iar_build.gen_iar_project as iar
        iar_project = iar.gen_iar_project(device, Projects, Project_name, Env['CPPDEFINES'], '', 0, iar_optim, True)
        if Env.GetOption('distclean'):
            del_dir = os.path.join(os.path.dirname(iar_project), Path(iar_project).stem)
            os.system(f'embuild {del_dir} delete')
            exit(0)
        if not (Env.GetOption('nobuild')):
            print('building iar project...')
            os.system('embuild ' + iar_project)
        exit(0)

    else:
        # generate gcc lib
        lib_name = GroupLibName(Project_name)
        objects = Env.Object(sources)
        program = Env.Library(lib_name, objects)
        # add library move action
        Env.BuildLib(lib_name, program)

#
# Main Entry
#
if __name__ == '__main__':
    print('test get config option')
