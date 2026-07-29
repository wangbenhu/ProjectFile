
printf "pin index:  pin mux(hex):  pin mux(dec):         func:\n"

set $i=0
while ($i < 39)
    set $group = $i / 4
    set $index = $i % 4

    if $index == 0
        set $ctrl = *(volatile unsigned *)(0x40000080 + $group*4)
    end

    set $ctrlp = $ctrl >> (8 * $index)
    set $pinmux = (unsigned int)$ctrlp & 0x7f
    printf "%02d:         0x%02x(hex)      %04d(dec)", $i, $pinmux, $pinmux
    printf "          "
    eval "output (pin_%d_func_t) $pinmux", $i
    printf "\n"
    set $i++
end
