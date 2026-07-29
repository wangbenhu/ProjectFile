..\..\tools\imgutils\win\gen_mbr.exe

:: Create update firmware without signature
..\..\tools\imgutils\win\CreateFwTool.exe pack ^
    .\ble_app_simple.bin APP 0  .\mbr.bin MBR_USR1 @0x108

:: Create update firmware with signature
:: ..\..\tools\imgutils\win\CreateFwTool.exe pack ^
::     .\ble_app_simple.bin APP 0  .\mbr.bin MBR_USR1 @0x108 -s tool\private_key.hex

pause

