#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import os
import shutil
try:
    from lxml import etree
except ImportError:
    print("ERROR:Not installed lxml module")

def keil_value_set(tree, path, regName0, modifyStr0, regName1, modifyStr1, modifyVal):
    text_node = tree.findall(path)
    for node in text_node:
        if node.text != regName0:
            continue
        pnode = node.getparent()
        cnode = pnode.findall(modifyStr0)
        if regName1 == 'NULL' or modifyStr1 == 'NULL':
            if len(cnode) == 0:
                print('modifyStr0 error')
                exit(0)
            cnode[0].text = modifyVal
        else:
            for node in cnode:
                if node.text != regName1:
                    continue
                pnode1 = node.getparent()
                cnode1 = pnode1.findall(modifyStr1)
                if len(cnode1) == 0:
                    print('modifyStr1 error')
                    exit(0)
                if type(modifyVal) == list:
                    if modifyVal == []:
                        break
                    else:
                        cnode1[0].text = modifyVal[0]
                        for i in range(len(modifyVal)-1):
                            add_code = etree.SubElement(pnode1, modifyStr1)
                            add_code.text = modifyVal[i+1]
                else:
                    cnode1[0].text = modifyVal

def gen_keil5_project(device, Projects, project, defs, ini_file, optim='-O2', lto=1, gen_lib=False):
    parser = etree.XMLParser(remove_blank_text=True)
    keil_template = os.path.join(os.path.dirname(__file__), 'keil_template.uvprojx')
    tree = etree.parse(keil_template, parser=parser)

    device = device.upper()
    rom_start_addr = '0x00404000'
    ram_start_addr = '0x20000000'
    flash_algo = device.upper() + '.FLM'
    cpu_text = 'IROM(0x00400000,0x400000) IRAM(0x20000000,0x1000) CPUTYPE("Cortex-M4") FPU2 DSP CLOCK(64000000) ELITTLE'

    #TargetName
    text_node = tree.find("Targets/Target/TargetName")
    text_node.text = project
    #Device
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/Device")
    text_node.text = device
    #Vendor
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/Vendor")
    text_node.text = 'Onmicro'
    #PackID
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/PackID")
    text_node.text = 'Onmicro.{}.1.0.0'.format(device)
    #Cpu
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/Cpu")
    text_node.text = cpu_text
    # optim
    keil_opt = ['-O0', '-O1', '-O2', '-O3', '-Ofast', '-Os balanced', '-Oz image size']
    if optim not in keil_opt:
        print('Paramater "optim" is error, keil optimization level must be: {}'.format(keil_opt))
    for index in range(len(keil_opt)):
        if optim == keil_opt[index]:
            opt_level = index + 1
    text_node=tree.find("Targets/Target/TargetOption/TargetArmAds/Cads/Optim")
    text_node.text = str(opt_level)
    # lto
    text_node = tree.find("Targets/Target/TargetOption/TargetArmAds/Cads/v6Lto")
    text_node.text = str(lto)
    #rom address
    text_node = tree.find("Targets/Target/TargetOption/TargetArmAds/LDads/TextAddressRange")
    text_node.text = rom_start_addr
    text_node = tree.find("Targets/Target/TargetOption/TargetArmAds/LDads/DataAddressRange")
    text_node.text = ram_start_addr
    #FlashDriverDll
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/FlashDriverDll")
    text_node.text = 'UL2CM3(-S0 -C0 -P0 -FD20000000 -FC20000 -FN1 -FF0{} -FS00 -FL0200000 -FP0($$Device:{}$Device\Onmicro\{}\{}))'.format(device,device,device,flash_algo)
    #RegisterFile
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/RegisterFile")
    text_node.text = '$$Device:{}$Device\Onmicro\{}\{}.h'.format(device,device,device)
    #SVD
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/SFDFile")
    text_node.text = ''
    #OutputName
    text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/OutputName")
    text_node.text = project

    #Define
    defines_str = ''
    for define in defs:
        defines_str += define+ ','
    text_node=tree.find("Targets/Target/TargetOption/TargetArmAds/Cads/VariousControls/Define")
    text_node.text = defines_str
    text_node=tree.find("Targets/Target/TargetOption/TargetArmAds/Aads/VariousControls/Define")
    text_node.text = defines_str

    project_dir_prefix = '../'
    #ScatterFile
    text_node=tree.find("Targets/Target/TargetOption/TargetArmAds/LDads/ScatterFile")
    for group in Projects:
        if 'LINKER_SCRIPT' in group:
            text_node.text = os.path.join(project_dir_prefix, group['LINKER_SCRIPT'])
    #INCS
    includes_str = ''
    for group in Projects:
        if 'incs' in group:
            for inc in group['incs']:
                includes_str += os.path.join(project_dir_prefix, os.path.relpath(inc)) + ';'
    #c/c++
    text_node = tree.find("Targets/Target/TargetOption/TargetArmAds/Cads/VariousControls/IncludePath")
    text_node.text = includes_str
    #asm
    text_node = tree.find("Targets/Target/TargetOption/TargetArmAds/Aads/VariousControls/IncludePath")
    text_node.text = includes_str
    #srcs
    for group in Projects:
        if 'srcs' in group:
            text_node = tree.find("Targets/Target/Groups")
            Group = etree.SubElement(text_node,"Group")
            text_node.append(Group)
            GroupName = etree.SubElement(Group,"GroupName")
            GroupName.text = group['name']
            Group.append(GroupName)
            Files = etree.SubElement(Group,"Files")
            for src in group['srcs']:
                File = etree.SubElement(Files,"File")
                FileName = etree.SubElement(File,"FileName")
                FileName.text = os.path.basename(src)
                File.append(FileName)

                FileType = etree.SubElement(File,"FileType")
                if (src.split('.')[-1] == 'c'):
                    FileType.text = '1'
                elif (src.split('.')[-1] == 'S' or 's'):
                    FileType.text = '2'
                elif (src.split('.')[-1] == 'lib'):
                    FileType.text = '4'
                File.append(FileType)

                FilePath = etree.SubElement(File,"FilePath")
                FilePath.text = project_dir_prefix + os.path.relpath(src)
                File.append(FilePath)

    #LIBS
    libs_str = ''
    for group in Projects:
        if 'libs' in group:
            for lib in group['libs']:
                libs_str += '\r\n' + project_dir_prefix + os.path.relpath(lib)
    # #InitializationFile
    # text_node = tree.find("Targets/Target/TargetOption/DebugOption/TargetDlls/InitializationFile")
    # text_node.text = ini_file
    # text_node = tree.find("Targets/Target/TargetOption/Utilities/Flash4")
    # text_node.text = ini_file
    # Mics controls
    text_node = tree.find("Targets/Target/TargetOption/TargetArmAds/LDads/Misc")
    # L6314: No section matches pattern; L6349; L6439W: Multiply defined Global Symbol
    text_node.text = '--diag_suppress=L6314 --diag_suppress=L6439W' + libs_str

    if gen_lib == True:
        # generate library
        text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/CreateExecutable")
        text_node.text = '0'
        text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/CreateLib")
        text_node.text = '1'
    else:
        # before make
        # UserProg1Name
        text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/BeforeMake/UserProg1Name")
        text_node.text = ''
        # after make
        # UserProg1Name
        text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/AfterMake/UserProg1Name")
        text_node.text = 'fromelf.exe --bin --output "$L@L.bin" "#L"'
        text_node = tree.find("Targets/Target/TargetOption/TargetCommonOption/AfterMake/UserProg2Name")
        text_node.text = 'fromelf.exe -c --output "$L@L.dis" "#L"'

    mdk_output_dir = os.path.join(os.getcwd(), 'keil5')
    if not os.path.exists(mdk_output_dir):
        os.mkdir(mdk_output_dir)
    project_path = '{}/{}.uvprojx'.format(mdk_output_dir, project)

    tree.write(project + '.uvprojx',pretty_print=True, xml_declaration=True, encoding='utf-8')
    shutil.move(project + '.uvprojx', project_path)
    shutil.copy(os.path.join(os.path.dirname(__file__), 'keil_load_debug.ini'), os.path.join(mdk_output_dir,'load_debug.ini'))

    # uvoptx file
    project_path_ex = '{}/{}.uvoptx'.format(mdk_output_dir, project)
    keil_uvoptx_template = os.path.join(os.path.dirname(__file__), 'keil_template.uvoptx')
    tree_ex = etree.parse(keil_uvoptx_template, parser=parser)
    #TargetName
    text_node = tree_ex.find("Target/TargetName")
    text_node.text = project
    #UL2CM3
    path = 'Target/TargetOption/TargetDriverDllRegistry/SetRegEntry/Key'
    ulcm3 = 'UL2CM3(-S0 -C0 -P0 )  -FN1 -FC20000 -FD20000000 -FF0{} -FL00200000 -FS00 -FP0($$Device:{}$Device\Onmicro\{}\{})'.format(device,device,device,flash_algo)
    keil_value_set(tree_ex, path, 'UL2CM3', 'Name', 'NULL', 'NULL', ulcm3)
    #JL2CM3
    jl2cm3 = '-U59406895 -O78 -S4 -ZTIFSpeedSel2000 -A0 -C0 -JU1 -JI127.0.0.1 -JP0 -RST0 -N00("ARM CoreSight SW-DP") -D00(0BB11477) -L00(0) -TO18 -TC10000000 -TP21 -TDS8007 -TDT0 -TDC1F -TIEFFFFFFFF -TIP8 -TB1 -TFE0 -FO15 -FD20000000 -FCF000 -FN1 -FF0{} -FS00 -FL010000000 -FP0($$Device:{}$Flash\{})'.format(flash_algo,device,flash_algo)
    keil_value_set(tree_ex, path, 'JL2CM3', 'Name', 'NULL', 'NULL', jl2cm3)

    tree_ex.write(project + '.uvoptx',pretty_print=True, xml_declaration=True, encoding='utf-8')
    shutil.move(project + '.uvoptx', project_path_ex)
    print("generate %s for %s keil5 MDK succeed!" %(project_path, device))

    return os.path.join(mdk_output_dir, project + '.uvprojx')
