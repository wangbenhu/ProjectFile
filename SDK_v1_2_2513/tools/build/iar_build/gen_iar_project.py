#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import os
import shutil

try:
    from lxml import etree
except ImportError:
    print("ERROR:Not installed lxml module")

def iar_value_set(tree, regName0, modifyStr0, regName1, modifyStr1, modifyVal):
    text_node = tree.findall("./configuration/settings/name")
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


def gen_iar_eww(project_name):
    parser = etree.XMLParser(remove_blank_text=True)
    eww_template = os.path.join(os.path.dirname(__file__), 'iar.eww')
    tree = etree.parse(eww_template, parser=parser)
    # project_path
    output_name = project_name + '.eww'
    text_node = tree.find("project/path")
    text_node.text = '$WS_DIR$/' + project_name + '.ewp'
    tree.write(output_name, pretty_print=True, xml_declaration=True, encoding='utf-8')

def gen_iar_ewp(project_name, device, projects, defs, proj_path_relative_gen, optim, gen_lib):
    parser = etree.XMLParser(remove_blank_text=True)
    ewp_template = os.path.join(os.path.dirname(__file__), 'iar.ewp')
    tree = etree.parse(ewp_template, parser=parser)
    # project_path
    output_name = project_name + '.ewp'
    text_node = tree.find("configuration/name")
    text_node.text = project_name
    # GFPUDeviceSlave
    device_name = device.upper() + '	Onmicro ' + device.upper()
    iar_value_set(tree, 'General', 'data/option/name', 'OGCoreOrChip', 'state', '1')
    iar_value_set(tree, 'General', 'data/option/name', 'GFPUDeviceSlave', 'state', device_name)
    iar_value_set(tree, 'General', 'data/option/name', 'OGChipSelectEditMenu', 'state', device_name)
    # outputfile path
    BrowseInfoPath = project_name + '/BrowseInfo'
    iar_value_set(tree, 'General', 'data/option/name', 'BrowseInfoPath', 'state', BrowseInfoPath)
    ExePath = project_name + '/Exe'
    iar_value_set(tree, 'General', 'data/option/name', 'ExePath', 'state', ExePath)
    ObjPath = project_name + '/Obj'
    iar_value_set(tree, 'General', 'data/option/name', 'ObjPath', 'state', ObjPath)
    ListPath = project_name + '/List'
    iar_value_set(tree, 'General', 'data/option/name', 'ListPath', 'state', ListPath)
    # Define
    iar_value_set(tree, 'ICCARM', 'data/option/name', 'CCDefines', 'state', defs)
    iar_value_set(tree, 'AARM', 'data/option/name', 'ADefines', 'state', defs)
    # .out file path
    outfilename = project_name + '.out'
    iar_value_set(tree, 'ILINK', 'data/option/name', 'IlinkOutputFile', 'state', outfilename)
    # mask warning
    ccdiagsuppress = 'Pe111,Pe177,Pe128,Ta022,Ta023'
    iar_value_set(tree, 'ICCARM', 'data/option/name', 'CCDiagSuppress', 'state', ccdiagsuppress)
    # opt level
    iar_opt = ['None', 'Low', 'Medium', 'High Balanced', 'High Size', 'High Speed']
    if optim not in iar_opt:
        print('Paramater "optim" is error, iar optimization level must be: {}'.format(iar_opt))
    for index in range(len(iar_opt)):
        if optim == iar_opt[index]:
            if index < 3:
                opt_level = index
            else:
                opt_level = 3
                opt_strategy = index - 3
    ccoptlevel = str(opt_level)
    iar_value_set(tree, 'ICCARM', 'data/option/name', 'CCOptLevel', 'state', ccoptlevel)
    ccoptstrategy = str(opt_strategy)
    iar_value_set(tree, 'ICCARM', 'data/option/name', 'CCOptStrategy', 'state', ccoptstrategy)
    proj_path_relative_gen = os.path.join('$PROJ_DIR$', proj_path_relative_gen)
    # outputformat(bin) and outputfilepath
    outputfilepath = '{}\{}.bin'.format(proj_path_relative_gen, project_name)
    iar_value_set(tree, 'OBJCOPY', 'data/option/name', 'OOCOutputFormat', 'state', '3')
    iar_value_set(tree, 'OBJCOPY', 'data/option/name', 'OOCOutputFormat', 'version', '3')
    iar_value_set(tree, 'OBJCOPY', 'data/option/name', 'OCOutputOverride', 'state', '1')
    iar_value_set(tree, 'OBJCOPY', 'data/option/name', 'OOCOutputFile', 'state', outputfilepath)
    #ScatterFile
    for group in projects:
        if 'LINKER_SCRIPT' in group:
            file_path = proj_path_relative_gen + os.path.relpath(group['LINKER_SCRIPT'])
            iar_value_set(tree, 'ILINK', 'data/option/name', 'IlinkIcfFile', 'state', file_path)
    #INCS
    incs_list = []
    for group in projects:
        if 'incs' in group:
            for inc in group['incs']:
                incs_list.append(proj_path_relative_gen + os.path.relpath(inc))
    iar_value_set(tree, 'ICCARM', 'data/option/name', 'CCIncludePath2', 'state', incs_list)
    iar_value_set(tree, 'AARM', 'data/option/name', 'AUserIncludes', 'state', incs_list)
    #SRCS
    root = tree.getroot()
    for group in projects:
        if 'srcs' in group:
            group_node = etree.SubElement(root,"group")
            root.append(group_node)
            group_name = etree.SubElement(group_node,"name")
            group_name.text = group['name']
            group_node.append(group_name)
            for src in group['srcs']:
                file = etree.SubElement(group_node,"file")
                group_node.append(file)
                file_path = etree.SubElement(file,"name")
                file_path.text = proj_path_relative_gen + os.path.relpath(src)
                file.append(file_path)
    #LIBS
    libs_list = []
    for group in projects:
        if 'libs' in group:
            for lib in group['libs']:
                libs_list.append(proj_path_relative_gen + os.path.relpath(lib))
    iar_value_set(tree, 'ILINK', 'data/option/name', 'IlinkAdditionalLibs', 'state', libs_list)
    # generate liberary
    if gen_lib == True:
        iar_value_set(tree, 'General', 'data/option/name', 'GOutputBinary', 'state', '1')
    tree.write(output_name, pretty_print=True, xml_declaration=True, encoding='utf-8')


def gen_iar_ewd(project_name, macfile_path):
    parser = etree.XMLParser(remove_blank_text=True)
    ewd_template = os.path.join(os.path.dirname(__file__), 'iar.ewd')
    tree = etree.parse(ewd_template, parser=parser)

    output_name = project_name + '.ewd'
    # project_path
    text_node = tree.find("configuration/name")
    text_node.text = project_name
    # add mac file
    macfile_path = macfile_path
    iar_value_set(tree, 'C-SPY', 'data/option/name', 'MacOverride', 'state', '1')
    iar_value_set(tree, 'C-SPY', 'data/option/name', 'MacFile', 'state', macfile_path)

    tree.write(output_name, pretty_print=True, xml_declaration=True, encoding='utf-8')


def gen_iar_project(device, Projects, project, defs, macfile_path, project_with_device=0, optim='Medium', gen_lib=False):
    if project_with_device :
        project = project + '_' + device.lower()
    gen_iar_eww(project)
    gen_iar_ewp(project, device, Projects, defs, '../', optim, gen_lib)
    gen_iar_ewd(project, macfile_path)
    output_dir = 'iar/'
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)
    shutil.move(project + '.eww', output_dir + project + '.eww')
    shutil.move(project + '.ewp', output_dir + project + '.ewp')
    shutil.move(project + '.ewd', output_dir + project + '.ewd')

    print("generate %s.eww IAR succeed!" %(output_dir + project))

    return output_dir + project + '.ewp'
