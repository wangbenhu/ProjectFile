set $mem_type=0
set $p_base=0
set $p_end=0
set $p_search=0

while $mem_type < CONFIG_MEM_NUM

    printf "Heap base: 0x%x, Heap end: 0x%x, Heap size: %d\n", om_mem_env.base[$mem_type], om_mem_env.end[$mem_type], om_mem_env.end[$mem_type] - om_mem_env.base[$mem_type]

    set $p_base   = (om_mem_heap_t *)om_mem_env.base[$mem_type]
    set $p_end    = (om_mem_heap_t *)(om_mem_env.end[$mem_type] - sizeof(om_mem_heap_t *))
    set $p_search = $p_base

    while $p_search != 0 && $p_search < $p_end
        if ($p_search == $p_base) && ($p_search->len == 0)
            printf "[freed] pos: 0x%x, len: %d\n", (unsigned)$p_base, (unsigned)(((om_mem_heap_t *)$p_base)->next) - (unsigned)$p_base
        else
            printf "[alloc] pos: 0x%x, len: %d\n", (uint32_t)$p_search, $p_search->len
            if ((unsigned)$p_search->next - (unsigned)$p_search) > $p_search->len
                printf "[freed] pos: 0x%x, len: %d\n", (unsigned)$p_search + $p_search->len, (unsigned)$p_search->next - ((unsigned)$p_search + $p_search->len)
            end
        end

        set $p_search = $p_search->next
    end

    set $mem_type += 1
end