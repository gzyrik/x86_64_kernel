#ifndef _NET_ETHER_H
#define _NET_ETHER_H
#define ETH_MIN_PACK_SIZE                 60
#define ETH_MAX_PACK_SIZE               1514
#define ETH_MAX_PACK_SIZE_TAGGED        1518
#define ETH_HDR_SIZE                      14
#define ETH_CRC_SIZE                       4
#include <net/if_ether.h>
#include <api/endian.h>
typedef u16_t ether_type_t;

#define ETH_ARP_PROTO    0x806
#define ETH_IP_PROTO     0x800
#define ETH_VLAN_PROTO  0x8100
#define ETH_IPV6_PROTO   0x86dd
/* Tag Control Information field for VLAN and Priority tagging */
#define ETH_TCI_PRIO_MASK       0xe000
#define ETH_TCI_CFI             0x1000  /* Canonical Formal Indicator */
#define ETH_TCI_VLAN_MASK       0x0fff  /* 12-bit vlan number */

typedef struct nwio_ethopt
{
        u32_t nweo_flags;
        ether_addr_t nweo_multi, nweo_rem;
        ether_type_t nweo_type;
} nwio_ethopt_t;

#define NWEO_NOFLAGS    0x0000L
#define NWEO_ACC_MASK   0x0003L
#       define NWEO_EXCL        0x00000001L
#       define NWEO_SHARED      0x00000002L
#       define NWEO_COPY        0x00000003L
#define NWEO_LOC_MASK   0x0010L
#       define NWEO_EN_LOC      0x00000010L
#       define NWEO_DI_LOC      0x00100000L
#define NWEO_BROAD_MASK 0x0020L
#       define NWEO_EN_BROAD    0x00000020L
#       define NWEO_DI_BROAD    0x00200000L
#define NWEO_MULTI_MASK 0x0040L
#       define NWEO_EN_MULTI    0x00000040L
#       define NWEO_DI_MULTI    0x00400000L
#define NWEO_PROMISC_MASK 0x0080L
#       define NWEO_EN_PROMISC  0x00000080L
#       define NWEO_DI_PROMISC  0x00800000L
#define NWEO_REM_MASK   0x0100L
#       define NWEO_REMSPEC     0x00000100L
#       define NWEO_REMANY      0x01000000L
#define NWEO_TYPE_MASK  0x0200L
#       define NWEO_TYPESPEC    0x00000200L
#       define NWEO_TYPEANY     0x02000000L
#define NWEO_RW_MASK    0x1000L
#       define NWEO_RWDATONLY   0x00001000L
#       define NWEO_RWDATALL    0x10000000L

typedef struct eth_stat
{
        unsigned long ets_recvErr,      /* # receive errors */
                ets_sendErr,            /* # send error */
                ets_OVW,                /* # buffer overwrite warnings,
                                           (packets arrive faster than
                                           can be processed) */
                ets_CRCerr,             /* # crc errors of read */
                ets_frameAll,           /* # frames not alligned (# bits
                                           not a mutiple of 8) */
                ets_missedP,            /* # packets missed due to too
                                           slow packet processing */
                ets_packetR,            /* # packets received */
                ets_packetT,            /* # packets transmitted */
                ets_transDef,           /* # transmission defered (there
                                           was a transmission of an
                                           other station in progress */
                ets_collision,          /* # collissions */
                ets_transAb,            /* # transmissions aborted due
                                           to accesive collisions */
                ets_carrSense,          /* # carrier sense lost */
                ets_fifoUnder,          /* # fifo underruns (processor
                                           is too busy) */
                ets_fifoOver,           /* # fifo overruns (processor is
                                           too busy) */
                ets_CDheartbeat,        /* # times unable to transmit
                                           collision signal */
                ets_OWC;                /* # times out of window
                                           collision */
} eth_stat_t;

typedef struct nwio_ethstat
{
        ether_addr_t nwes_addr;
        eth_stat_t nwes_stat;
} nwio_ethstat_t;

typedef struct eth_hdr
{
        ether_addr_t eh_dst;
        ether_addr_t eh_src;
        ether_type_t eh_proto;
} eth_hdr_t;

static inline void eth_set_hdr(eth_hdr_t *p,void * d,void * s, ether_type_t proto)
{
    __builtin_memcpy(&p->eh_dst,d,6);
    __builtin_memcpy(&p->eh_src,s,6);
    p->eh_proto = htons(proto);

}
#endif
