#!/usr/bin/python
# -*- coding: utf-8 -*-

import click, sys, os, re, yaml, struct, crcmod, hashlib
from typing import Union
from enum import IntEnum
from intelhex import IntelHex
from cryptography.hazmat.primitives.serialization import Encoding, PrivateFormat, NoEncryption, PublicFormat
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils
from cryptography.hazmat.primitives import hashes

class FlashRdCmd(IntEnum):
    # Read
    FLASH_READ          = 0x0
    FLASH_FAST_READ     = 0x1
    FLASH_FAST_READ_DO  = 0x2
    FLASH_FAST_READ_DIO = 0x3
    FLASH_FAST_READ_QO  = 0x4
    FLASH_FAST_READ_QIO = 0x5

class FlashWrCmd(IntEnum):
    # Program
    FLASH_PAGE_PROGRAM    = 0x0
    # 6627 内部flash不支持四线写
    # FLASH_PAGE_PROGRAM_QI = 0x1

class FlashSpiMode(IntEnum):
    # SPI mode
    SPI_MODE_0 = 0x0
    SPI_MODE_1 = 0x1
    SPI_MODE_2 = 0x2
    SPI_MODE_3 = 0x3

class HexParamType(click.ParamType):
    name = 'hex'

    def convert(self, value, param, ctx):
        try:
            if value.startswith('0x'):
                return int(value, 16)
            else:
                self.fail(f'Invalid hexadecimal number: {value}', param, ctx)
        except ValueError:
            self.fail(f'Invalid hexadecimal number: {value}', param, ctx)

def crc32_of(data: Union[str, list, bytes, bytearray]) -> hex:
    if isinstance(data, str):
        data = data.encode(encoding='utf-8')

    crc32_func = crcmod.mkCrcFun(0x104c11db7, rev=True, initCrc=0xFFFFFFFF, xorOut=0x0)
    crc32_output = crc32_func(bytes(data))

    return crc32_output

def hash_of(data: Union[str, list, bytes, bytearray]) -> tuple:
    if isinstance(data, str):
        data = data.encode(encoding='utf-8')

    sha256_hash = hashlib.sha256()
    sha256_hash.update(bytes(data))
    hashed_output = sha256_hash.digest()

    return tuple(hashed_output)

def sign_of(private_key_obj, data: Union[str, list, bytes, bytearray]) -> tuple:
    if private_key_obj:
        if isinstance(data, str):
            data = data.encode(encoding='utf-8')

        sign = private_key_obj.sign(bytes(data), ec.ECDSA(hashes.SHA256()))

        [r, s] = utils.decode_dss_signature(sign)
        sign_r = r.to_bytes(32, 'little')
        sign_s = s.to_bytes(32, 'little')
        sign_bytes = sign_r + sign_s

        return tuple(sign_bytes)
    else:
        return tuple([0xFF] * 64)

def format_c_array(array_name, data_list):
    format_list = ''

    for i in range(0, len(data_list)):
        format_list += '0x{:02x}'.format(data_list[i])
        if i != len(data_list) - 1:
            format_list += ','
        if (i + 1) % 8 == 0:
            format_list += '\n'
            if i != len(data_list) - 1:
                format_list += '\t'
        else:
            format_list += ' '

    format_array = '#include <stdint.h>\n\nconst uint8_t {}[] = {{\n\t{}}};'.format(array_name, format_list)

    return format_array

def get_keypair_from_pem(pem_file):
    with open(pem_file, 'r') as pem_file:
        pem_data = pem_file.read()
        private_key_obj = serialization.load_pem_private_key(pem_data.encode(), password=None)
        public_key_obj = private_key_obj.public_key()
    return (private_key_obj, public_key_obj)

def get_keypair_from_array(c_file):
    with open(c_file, 'r') as f:
        private_key_val = int.from_bytes(extract_c_array(f.read()), 'little')
        private_key_obj = ec.derive_private_key(private_key_val, ec.SECP256R1())
        public_key_obj = private_key_obj.public_key()
    return (private_key_obj, public_key_obj)

def extract_c_array(content):
    c_array = []
    hex_numbers = re.findall(r'0x[0-9a-fA-F]+', content)

    for i in hex_numbers:
        c_array.append(int(i, 16))

    return c_array

@click.group()
@click.pass_context
def client(ctx):
    ctx.ensure_object(dict)

@client.command(name='merge', help = "Merge IntelHex files to one file(ELF/HEX/BIN)")
@click.argument('hex_files', nargs=-1, type=str)
@click.option('-o', '--output', type=str)
@click.pass_context
def merge(ctx, hex_files, output):
    if output is None:
        print('ERROR: Please specify output file name, need file name\'s postfix to decide output format')
        sys.exit()

    name, postfix = os.path.splitext(output)
    if postfix == '.hex':
        hex_file = IntelHex()
        for hex_file_name in hex_files:
            hex_file.merge(IntelHex(hex_file_name))
        hex_file.tofile(output, format='hex')
    elif postfix == '.bin':
        hex_file = IntelHex()
        for hex_file_name in hex_files:
            hex_file.merge(IntelHex(hex_file_name))
        for start, stop in hex_file.segments():
            filename = name + '_' + hex(start) + '_to_' + hex(stop - 1) + '.bin'
            with open(filename, 'wb') as f:
                f.write(hex_file.tobinarray(start = start, end = stop - 1))
    elif postfix == '.elf':
        if os.system('arm-none-eabi-objcopy --version') != 0:
            print('ERROR: arm-none-eabi-objcopy not found')
            sys.exit()
        hex_file = IntelHex()
        for hex_file_name in hex_files:
            hex_file.merge(IntelHex(hex_file_name))
        hex_file.tofile('tmp.hex', format='hex')
        os.system('arm-none-eabi-objcopy -I ihex -O elf32-little tmp.hex ' + output)
        os.remove('tmp.hex')

@client.command(name='gen_keypair', help='Generate ECDSA key pair(random)')
@click.option('-f', '--format', type=click.Choice(['PEM', 'C']), default='PEM')
@click.pass_context
def gen_keypair(ctx, format):
    private_key_obj = ec.generate_private_key(ec.SECP256R1())
    public_key_obj = private_key_obj.public_key()

    if format == 'PEM':
        private_key_pem = private_key_obj.private_bytes(
                                                    encoding=Encoding.PEM,
                                                    format=PrivateFormat.PKCS8,
                                                    encryption_algorithm=NoEncryption()
                                                    )
        public_key_pem = public_key_obj.public_bytes(
                                                    encoding=Encoding.PEM,
                                                    format=PublicFormat.SubjectPublicKeyInfo
                                                    )
        with open('private_key.pem', 'w') as f:
            f.write(private_key_pem.decode())
        with open('public_key.pem', 'w') as f:
            f.write(public_key_pem.decode())
    elif format == 'C':
        private_key_bytes = private_key_obj.private_numbers().private_value.to_bytes(32, 'little')
        with open('private_key.c', 'w') as f:
            f.write(format_c_array('private_key', private_key_bytes))

        publick_key_x_bytes = public_key_obj.public_numbers().x.to_bytes(32, 'little')
        publick_key_y_bytes = public_key_obj.public_numbers().y.to_bytes(32, 'little')
        public_key_bytes    = publick_key_x_bytes + publick_key_y_bytes
        with open('public_key.c', 'w') as f:
            f.write(format_c_array('public_key', public_key_bytes))

@client.command(name='gen_pubkey', help="Generate Public Key or Public Key's hash from ECDSA private key")
@click.option('-i', '--input', type=str)
@click.option('-o', '--output', default='puiblic_key.pem')
@click.pass_context
def gen_pubkey(ctx, input, output):
    if input == None:
        print('Please specify input private key file')
        sys.exit(1)
    _, postfix = os.path.splitext(input)
    if postfix == '.c':
        private_key_obj, public_key_obj = get_keypair_from_array(input)
    elif postfix == '.pem':
        private_key_obj, public_key_obj = get_keypair_from_pem(input)
    else:
        print('Unsupported input file format')
        sys.exit(1)

    _, postfix = os.path.splitext(output)
    if postfix == '.pem':
        public_key_obj = private_key_obj.public_key()
        public_key_pem = public_key_obj.public_bytes(
                                                    encoding=Encoding.PEM,
                                                    format=PublicFormat.SubjectPublicKeyInfo
                                                    )
        with open(output, 'w') as f:
            f.write(public_key_pem.decode())
    elif postfix == '.c':
        publick_key_x_bytes = public_key_obj.public_numbers().x.to_bytes(32, 'little')
        publick_key_y_bytes = public_key_obj.public_numbers().y.to_bytes(32, 'little')
        public_key_bytes    = publick_key_x_bytes + publick_key_y_bytes
        with open(output, 'w') as f:
            f.write(format_c_array('public_key', public_key_bytes))
    elif postfix == '.hash':
        publick_key_x_bytes = public_key_obj.public_numbers().x.to_bytes(32, 'little')
        publick_key_y_bytes = public_key_obj.public_numbers().y.to_bytes(32, 'little')
        public_key_bytes    = publick_key_x_bytes + publick_key_y_bytes
        with open(output, 'w') as f:
            f.write(format_c_array('public_key_hash', hash_of(public_key_bytes)))
    else:
        print('Unsupported output file format')
        sys.exit(1)

@client.command(name='gen_img', help='Generate image for downloading or upgrading')
@click.option('-c', '--config', type=str)
@click.option('-a', '--application_hex', type=str)
@click.option('-k', '--private_key', type=str)
@click.option('--load_addr', type=HexParamType())
@click.option('-o', '--output_hex', default='image.hex')
@click.pass_context
def gen_img(ctx, config, application_hex, private_key, load_addr, output_hex):
    application = application_hex
    output = output_hex

    if config == None or application == None:
        print('Please specify config and application file')
        sys.exit(1)

    app_hex = IntelHex(application)
    app_hex.padding = 0x00
    app_bin = app_hex.tobinarray()
    app_vma = app_hex.segments()[0][0]
    app_lma = load_addr if load_addr != None else 0xDEADBEEF

    private_key_obj = None
    if private_key:
        _, postfix = os.path.splitext(private_key)
        if postfix == '.pem':
            private_key_obj, public_key_obj = get_keypair_from_pem(private_key)
        elif postfix == '.c':
            private_key_obj, public_key_obj = get_keypair_from_array(private_key)
        else:
            print('Unsupported private key format')
            sys.exit(1)

    # Genrate mbr
    with open(config, 'r', encoding='utf-8') as f:
        mbr_cfg = yaml.safe_load(f)
    # mbr data由flash configuration和image mainfest组成，各自占一个页
    flash_config_data = [0xFF] * 256
    manifest_config_data = [0xFF] * 256
    # 1. flash configuration
    if (mbr_cfg['iflash_config']['clk_div']   != None and
        mbr_cfg['iflash_config']['delay']     != None and
        mbr_cfg['iflash_config']['read_cmd']  != None and
        mbr_cfg['iflash_config']['write_cmd'] != None and
        mbr_cfg['iflash_config']['spi_mode']  != None and
        mbr_cfg['iflash_config']['encrypt_en'] != None):
        flash_cfg_top = (bytes((ord('C'), ord('F'), ord('G'), 0)) +
                         struct.pack('<6B', mbr_cfg['iflash_config']['clk_div'],
                                            mbr_cfg['iflash_config']['delay'],
                                            FlashRdCmd[mbr_cfg['iflash_config']['read_cmd']].value,
                                            FlashWrCmd[mbr_cfg['iflash_config']['write_cmd']].value,
                                            FlashSpiMode[mbr_cfg['iflash_config']['spi_mode']].value,
                                            mbr_cfg['iflash_config']['encrypt_en']))
        flash_cfg_crc32 = crc32_of(flash_cfg_top)
        flash_cfg = flash_cfg_top + struct.pack('<I', flash_cfg_crc32)
        flash_config_data[0:len(flash_cfg)] = list(flash_cfg)
    # 2. manifest configuration
    magic_num = (ord('M'), ord('B'), ord('R'), 0)
    img_hash = hash_of(list(app_bin))
    if private_key_obj:
        publick_key_x_bytes = public_key_obj.public_numbers().x.to_bytes(32, 'little')
        publick_key_y_bytes = public_key_obj.public_numbers().y.to_bytes(32, 'little')
        public_key          = publick_key_x_bytes + publick_key_y_bytes
    else:
        public_key = [0xFF] * 64

    manifest_config_top = (bytes(magic_num) +
                        struct.pack('<5I',
                                    0xB4,
                                    mbr_cfg['manifest_config']['img_ver'],
                                    app_vma,
                                    app_lma,
                                    len(list(app_bin))) +
                        bytes(img_hash) +
                        bytes(public_key))
    manifest_sign = sign_of(private_key_obj, manifest_config_top)
    manifest_crc32 = crc32_of(manifest_config_top + bytes(manifest_sign))
    manifest_config = manifest_config_top + bytes(manifest_sign) + struct.pack('<I', manifest_crc32)
    manifest_config_data[0:len(manifest_config)] = list(manifest_config)

    # write to file
    mbr_data = flash_config_data + manifest_config_data

    # 3. 确定mbr和image的存放位置
    mbr_download_addr = 0x00402000 if load_addr == None else 0x00403000
    img_download_addr = app_vma if load_addr == None else app_lma

    mbr_hex = IntelHex()
    mbr_hex.frombytes(bytes(mbr_data), offset = mbr_download_addr)
    img_hex = IntelHex()
    img_hex.frombytes(bytes(app_bin), offset = img_download_addr)

    merge_hex = IntelHex()
    merge_hex.merge(mbr_hex)
    merge_hex.merge(img_hex)
    merge_hex.tofile(output, format='hex')


if __name__ == '__main__':
    client()
