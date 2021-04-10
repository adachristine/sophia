#pragma once


#define PAGE_MAP_LEVELS 4
#define PAGE_MAP_BITS 9
#define PAGE_OFFSET_BITS 12

#define PAGE_PR (1ULL << 0)
#define PAGE_WR (1ULL << 1)
#define PAGE_LG (1ULL << 7)
#define PAGE_NX (1ULL << 63)

#define PAGE_SIZE 0x1000
#define PAGE_OFFSET_MASK (PAGE_SIZE - 1)
#define PAGE_ADDRESS_MASK   0x0007fffffffff000
#define PAGE_ATTRIBUTE_MASK 0xf800000000000fff

#define PAGE_SIZE_LARGE 0x200000
#define PAGE_OFFSET_MASK_LARGE (PAGE_SIZE_LARGE - 1)
#define PAGE_ADDRESS_MASK_LARGE 0x0007ffffffe00000
#define PAGE_ATTRIBUTE_MASK_LARGE 0xf800000000001fff

#define PAGE_SIZE_HUGE 0x40000000

#define PAGE_TABLE_INDEX_MASK ((1 << PAGE_MAP_BITS) - 1)

#define page_table_index_bits(l) (PAGE_OFFSET_BITS + PAGE_MAP_BITS * (l - 1))
#define page_table_index(v, l) (((uint64_t)v >> page_table_index_bits(l)) & PAGE_TABLE_INDEX_MASK)
