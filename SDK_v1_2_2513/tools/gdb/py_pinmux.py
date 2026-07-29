import gdb

t = gdb.inferiors()[0]
reg_value = t.read_memory(0xE000EE08, 4)

if (ord(reg_value[2]) & 1) == 1:
    print("Current domain is Secure")
    reg = 0x40000080
else:
    print("Current domain is non-Secure")
    reg = 0x50000080

i = 0
reg_value = t.read_memory(0x50000080, 32)
print("pin index:  pin mux(hex):  pin mux(dec):         func:")

for char in reg_value:
    num = ord(char)
    num &= 0x7F
    num_s = str(num)
    cmd_s = "printf \"%02d:         0x%02x(hex)      %04d(dec)             \"," + str(i) + "," + str(num_s) + "," + str(num_s)
    gdb.execute(cmd_s)
    cmd_s = "output (pin_" + str(i) + "_func_t)" + num_s
    gdb.execute(cmd_s)
    print("")
    i+=1