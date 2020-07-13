#include <yaos/types.h>
#include <asm/bitops.h>
#include <yaos/printk.h>
#include <asm/pgtable.h>
#include <yaos/kheap.h>
#include <asm/pm64.h>
#include <yaos/assert.h>
#include <string.h>
#if 1
#define DEBUG_PRINTK printk
#else
#define DEBUG_PRINTK inline_printk
#endif

extern ulong __max_phy_addr;
static ulong *pgbits;
static ulong *pmax;
static ulong *pnow;
static ulong nowbit;
static ulong maxbits;
static ulong pages;

static ulong smallbits[PAGE_SIZE_LARGE/PAGE_4K_SIZE/sizeof(long)];
static ulong *smallpmax;
static ulong *smallpnow;
static ulong smallnowbit;
static ulong smallmaxbits;
static ulong smallpages;
static ulong smallpage_base;
extern ulong __max_phy_mem_addr;

#undef NULL_PTR
#define NULL_PTR ((ulong *)0)
static inline ulong *get_none_zero()
{
    ulong *pos;

    if (*pnow)
        return pnow;
    pos = pnow;
    while (++pnow < pmax) {
        if (*pnow)
            return pnow;
    }
    pnow = pgbits;
    if (*pnow)
        return pnow;
    while (++pnow < pos) {
        if (*pnow)
            return pnow;
    }
    return NULL_PTR;
}
static inline ulong *get_none_zero_small()
{
    ulong *pos;

    if (*smallpnow)
        return smallpnow;
    pos = smallpnow;
    while (++smallpnow < smallpmax) {
        if (*smallpnow)
            return smallpnow;
    }
    smallpnow = smallbits;
    if (*smallpnow)
        return smallpnow;
    while (++smallpnow < pos) {
        if (*smallpnow)
            return smallpnow;
    }
    return NULL_PTR;
}

ulong alloc_phy_page()
{
    ulong *p;
    int pos;

    while (pages) {
        p = get_none_zero();
        if (p == NULL_PTR)
            return 0;
        if (!*p)continue; 
        pos = __ffs(*p);
        if (test_and_clear_bit(pos, p)) {
            pages--;
            return PAGE_SIZE_LARGE * (pos + (p - pgbits) * 64);
        }

    }

    return 0;
}
//call from page fault
ulong alloc_small_phy_page_safe()
{
    ulong *p;
    int pos;
    if (!smallpages) {
        ulong addr = alloc_phy_page();
        if (!addr) return addr;
        smallpage_base = addr;
        memset( smallbits, 0xff, sizeof(smallbits));
        smallpnow = smallbits;
        smallpmax = smallbits + (PAGE_SIZE_LARGE / PAGE_4K_SIZE) /64;
        smallnowbit = 0;
        smallpages = PAGE_SIZE_LARGE / PAGE_4K_SIZE;
            
    }
    while (smallpages) {
        p = get_none_zero_small();
        if (p == NULL_PTR)
            return alloc_small_phy_page_safe();
        if (!*p) continue;
        pos = __ffs(*p);
        if (test_and_clear_bit(pos, p)) {
            smallpages--;
            return PAGE_4K_SIZE * (pos + (p - smallbits) * 64) + smallpage_base;
        }

    }
    return 0;

}
void free_phy_one_page(ulong addr)
{
    ulong pos;
    DEBUG_PRINTK("phy mem00: free one page at %lx,pages:%d\n", addr, pages);
    ASSERT(addr<__max_phy_mem_addr);
    if (addr) {                 /*can't free first page */
        /*page align */

        ASSERT((addr & (PAGE_SIZE_LARGE - 1)) == 0 && addr < __max_phy_addr);
        pos = addr / PAGE_SIZE_LARGE;
        if (test_and_set_bit(pos, pgbits)) {
            printk("addr:%lx already released\n", addr);
        }
        else
            pages++;
    }
}

void free_phy_pages(ulong addr, size_t size)
{
    ulong pos;
    ASSERT(addr<__max_phy_mem_addr);

    DEBUG_PRINTK("phy mem: free  pages at %lx,size:%lx\n", addr, size);

    if (addr) {
        ASSERT((addr & (PAGE_SIZE_LARGE - 1)) == 0 && addr < __max_phy_addr);
        ASSERT((size & (PAGE_SIZE_LARGE - 1)) == 0);
        pos = addr / PAGE_SIZE_LARGE;
        while (size > 0) {
            if (test_and_set_bit(pos, pgbits)) {
                printk("addr:%lx already released\n", addr);
            }
            else {
                pages++;
            }
            pos++;
            size -= PAGE_SIZE_LARGE;
        }
    }

}

void init_phy_mem()
{
    ulong bits = __max_phy_addr / PAGE_SIZE_LARGE;
    size_t size = bits / 8 + 8;
    char *p = kalloc(size);

    maxbits = bits;
    memset(p, 0, size);

    pgbits = (ulong *) p;
    pnow = pgbits;
    pmax = pgbits + bits / 64;
    nowbit = 0;
    pages = 0;
    

    bits = PAGE_SIZE_LARGE / PAGE_4K_SIZE;
    size  = bits/8;
    memset( smallbits, 0, sizeof(smallbits));
    smallmaxbits = bits;
    smallpnow = smallbits;
    smallpmax = smallbits + bits/64;
    smallnowbit = 0;
    smallpages = 0;    
}
