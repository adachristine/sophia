#pragma once

#define PAGE_PR 1
#define PAGE_WR 2

#define PAGE_MAP_LEVELS 4
#define PAGE_MAP_BITS 9
#define PAGE_OFFSET_BITS 12

#define PAGE_TABLE_INDEX_MASK ((1 << PAGE_MAP_BITS) - 1)
#define PAGE_TABLE_ADDRESS_MASK ~((1 << PAGE_OFFSET_BITS) - 1)

#define page_table_index_bits(l) (PAGE_OFFSET_BITS + PAGE_MAP_BITS * (l - 1))
#define page_table_index(v, l) ((v >> page_table_index_bits(l)) & PAGE_TABLE_INDEX_MASK)

