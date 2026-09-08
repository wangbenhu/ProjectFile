#!/usr/bin/python
# -*- coding: UTF-8 -*-

import struct, crcmod, hashlib, yaml, sys
from typing import Union
from enum import IntEnum
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric import utils


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

def extract_file_path():
    # 检查是否有-f参数
    if '-p' in sys.argv:
        # 获取-p参数的索引
        index = sys.argv.index('-p')
        # 确保有参数值（即-f后面有值）
        if index + 1 < len(sys.argv):
            file_path = sys.argv[index + 1]
            return file_path

    return './tool/mbr_cfg.yaml'

class GenMbr:
    def __init__(self, private_key):
        if private_key:
            self.sign_algo   = ec.ECDSA(hashes.SHA256())
            self.private_key = ec.derive_private_key(private_key, ec.SECP256R1())
            self.public_key  = self.private_key.public_key()
        else:
            self.private_key = None

    def crc32_of(self, data: Union[str, list, bytes, bytearray]) -> hex:
        if isinstance(data, str):
            data = data.encode(encoding='utf-8')

        crc32_func = crcmod.mkCrcFun(0x104c11db7, rev=True, initCrc=0xFFFFFFFF, xorOut=0x0)
        crc32_output = crc32_func(bytes(data))

        return crc32_output

    def hash_of(self, data: Union[str, list, bytes, bytearray]) -> tuple:
        if isinstance(data, str):
            data = data.encode(encoding='utf-8')

        sha256_hash = hashlib.sha256()
        sha256_hash.update(bytes(data))
        hashed_output = sha256_hash.digest()

        return tuple(hashed_output)

    def get_key(self) -> tuple:
        if self.private_key:
            # 私钥处理
            private_key_bytes = self.private_key.private_numbers().private_value.to_bytes(32, 'little')

            # 公钥处理
            publick_key_x_bytes = self.public_key.public_numbers().x.to_bytes(32, 'little')
            publick_key_y_bytes = self.public_key.public_numbers().y.to_bytes(32, 'little')
            public_key_list     = list(publick_key_x_bytes + publick_key_y_bytes)

            return (tuple(private_key_bytes), tuple(public_key_list), tuple(self.hash_of(public_key_list)))
        else:
            return (tuple([0]*32), tuple([0]*64), tuple([0]*32))

    def get_sign_of(self, data: Union[str, list, bytes, bytearray]) -> tuple:
        if self.private_key:
            if isinstance(data, str):
                data = data.encode(encoding='utf-8')

            sign = self.private_key.sign(bytes(data), self.sign_algo)
            try:
                self.public_key.verify(sign, bytes(data), self.sign_algo)
            except Exception:
                raise ValueError('Generate ECDSA Signature Failed')

            [r, s] = utils.decode_dss_signature(sign)
            sign_r = r.to_bytes(32, 'little')
            sign_s = s.to_bytes(32, 'little')
            sign_bytes = sign_r + sign_s

            return tuple(sign_bytes)
        else:
            return tuple([0]*64)

if __name__ == '__main__':
    cfg_path = extract_file_path()
    with open(cfg_path, 'r', encoding='utf-8') as f:
        mbr_cfg = yaml.safe_load(f)

    gen_mbr = GenMbr(mbr_cfg['private_key'])

    # mbr data由flash configuration和image mainfest组成，各自占一个页
    flash_config_data = [0xFF] * 256
    manifest_config_data = [0xFF] * 256

    # flash configuration
    if (mbr_cfg['iflash_config']['clk_div']   != None and
        mbr_cfg['iflash_config']['delay']     != None and
        mbr_cfg['iflash_config']['read_cmd']  != None and
        mbr_cfg['iflash_config']['write_cmd'] != None and
        mbr_cfg['iflash_config']['spi_mode']  != None and
        mbr_cfg['iflash_config']['encrypt_en'] != None):

        if (mbr_cfg['iflash_config']['clk_div'] < 4 or
           mbr_cfg['iflash_config']['read_cmd'] == 'FLASH_FAST_READ_QIO' or
           mbr_cfg['iflash_config']['read_cmd'] == 'FLASH_FAST_READ_DIO'):
            print('ERROR: Flash configuration parameters')
            import sys
            sys.exit()

        flash_cfg_top = (bytes((ord('C'), ord('F'), ord('G'), 0)) +
                         struct.pack('<6B', mbr_cfg['iflash_config']['clk_div'],
                                            mbr_cfg['iflash_config']['delay'],
                                            FlashRdCmd[mbr_cfg['iflash_config']['read_cmd']].value,
                                            FlashWrCmd[mbr_cfg['iflash_config']['write_cmd']].value,
                                            FlashSpiMode[mbr_cfg['iflash_config']['spi_mode']].value,
                                            mbr_cfg['iflash_config']['encrypt_en']))
        flash_cfg_crc32 = gen_mbr.crc32_of(flash_cfg_top)
        flash_cfg = flash_cfg_top + struct.pack('<I', flash_cfg_crc32)
        flash_config_data[0:len(flash_cfg)] = list(flash_cfg)

    # manifest configuration
    if mbr_cfg['manifest_config']['img_path'] == None:
        raise ValueError('Image Path Not Found')
    with open(mbr_cfg['manifest_config']['img_path'], 'rb') as f:
        img_data = f.read()
    magic_num = (ord('M'), ord('B'), ord('R'), 0)
    img_hash = gen_mbr.hash_of(img_data)
    public_key = gen_mbr.get_key()[1]

    manifest_config_top = (bytes(magic_num) +
                        struct.pack('<5I',
                                    0xB4,
                                    mbr_cfg['manifest_config']['img_ver'],
                                    mbr_cfg['manifest_config']['img_vma'],
                                    mbr_cfg['manifest_config']['img_lma'],
                                    len(img_data)) +
                        bytes(img_hash) +
                        bytes(public_key))
    manifest_sign = gen_mbr.get_sign_of(manifest_config_top)
    manifest_crc32 = gen_mbr.crc32_of(manifest_config_top + bytes(manifest_sign))
    manifest_config = manifest_config_top + bytes(manifest_sign) + struct.pack('<I', manifest_crc32)
    manifest_config_data[0:len(manifest_config)] = list(manifest_config)

    # write to file
    mbr_data = flash_config_data + manifest_config_data
    with open(mbr_cfg['output_path'], 'wb') as f:
        f.write(bytes(mbr_data))
