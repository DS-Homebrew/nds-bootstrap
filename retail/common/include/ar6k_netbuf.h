// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "calico_types.h"
//#include "../arm/common.h"

/*! @addtogroup netbuf
	@{
*/

/*! @brief Atomically swaps a 32-bit @p value with that at a memory location @p addr
	@warning This is not a compare-and-swap primitive - it *always* swaps.
	As such, it cannot be used to implement arbitrary atomic operations.
	@note For inter-processor synchronization consider using @ref SMutex.
	@returns The previous value of `*addr`
*/
MK_EXTINLINE u32 armSwapWord(u32 value, vu32* addr)
{
	u32 ret;
	__asm__ __volatile__ ("swp %[Rd], %[Rm], [%[Rn]]" : [Rd]"=r"(ret) : [Rm]"[Rd]"(value), [Rn]"r"(addr) : "memory");
	return ret;
}

MK_EXTERN_C_START

typedef struct NetBuf NetBuf;

//! Standard frame EtherType values
typedef enum NetEtherType {
	NetEtherType_First = 0x0600, //!< First valid EtherType (lower values are interpreted as packet size)
	NetEtherType_IPv4  = 0x0800, //!< Internet Protocol, version 4
	NetEtherType_ARP   = 0x0806, //!< Address Resolution Protocol
	NetEtherType_IPv6  = 0x86dd, //!< Internet Protocol, version 6
	NetEtherType_EAPOL = 0x888e, //!< Extensible Authentication Protocol over LAN (used by WPA)
} NetEtherType;

//! Standard link-layer frame header (Ethernet frame)
typedef struct NetMacHdr {
	u8 dst_mac[6];           //!< Destination MAC address
	u8 src_mac[6];           //!< Source MAC address
	u16 len_or_ethertype_be; //!< See @ref NetEtherType_First
} NetMacHdr;

//! Standard IEEE 802.2 LLC + SNAP header
typedef struct NetLlcSnapHdr {
	u8 dsap;
	u8 ssap;
	u8 control;
	u8 oui[3];
	u16 ethertype_be;
} NetLlcSnapHdr;

//! @private
typedef struct NetBufListNode {
	NetBuf* next;
	NetBuf* prev;
} NetBufListNode;

//! Network packet buffer object
struct NetBuf {
	NetBufListNode link; //!< @private
	u16 flags;           //!< @private
	u16 capacity;        //!< Total capacity of the buffer
	u16 pos;             //!< Start position of the packet data within the buffer
	u16 len;             //!< Current length of the packet data
	u32 reserved[4];     //!< @private
};

//! Network packet heap IDs
typedef enum NetBufPool {
	NetBufPool_Tx = 0, //!< Outgoing packet (TX) pool
	NetBufPool_Rx = 1, //!< Incoming packet (RX) pool

	NetBufPool_Count, //!< @private
} NetBufPool;

/*! @brief Allocates a new network buffer
	@param[in] hdr_headroom_sz Size in bytes of the header headroom to reserve
	@param[in] data_sz Size in bytes of the packet payload
	@param[in] pool ID of the heap from which to allocate the buffer (see @ref NetBufPool)
	@return Network buffer object on success, NULL on failure (out of memory)
*/
NetBuf* netbufAlloc(unsigned hdr_headroom_sz, unsigned data_sz, NetBufPool pool);

//! Ensures the other CPU can observe the current contents of network buffer @p nb
void netbufFlush(NetBuf* nb);

//! Frees network buffer @p nb
void netbufFree(NetBuf* nb);

//! Returns the data pointer of network buffer @p nb
MK_INLINE void* netbufGet(NetBuf* nb) {
	return (u8*)(nb+1) + nb->pos;
}

//! Prepends a header with the given @p _type to network buffer @p _nb
#define netbufPushHeaderType(_nb, _type)  ((_type*)netbufPushHeader ((_nb), sizeof(_type)))
//! Extracts a header with the given @p _type from the network buffer @p _nb
#define netbufPopHeaderType(_nb, _type)   ((_type*)netbufPopHeader  ((_nb), sizeof(_type)))
//! Appends trailing data with the given @p _type to network buffer @p _nb
#define netbufPushTrailerType(_nb, _type) ((_type*)netbufPushTrailer((_nb), sizeof(_type)))
//! Extracts trailing data with the given @p _type from the network buffer @p _nb
#define netbufPopTrailerType(_nb, _type)  ((_type*)netbufPopTrailer ((_nb), sizeof(_type)))

//! Prepends a header with the given @p size to network buffer @p nb
MK_INLINE void* netbufPushHeader(NetBuf* nb, unsigned size) {
	if (nb->pos < size) {
		return NULL;
	} else {
		nb->pos -= size;
		nb->len += size;
		return netbufGet(nb);
	}
}

//! Extracts a header with the given @p size from the network buffer @p nb
MK_INLINE void* netbufPopHeader(NetBuf* nb, unsigned size) {
	void* hdr = NULL;
	if (nb->len >= size) {
		hdr = netbufGet(nb);
		nb->pos += size;
		nb->len -= size;
	}
	return hdr;
}

//! Appends trailing data with the given @p size to network buffer @p nb
MK_INLINE void* netbufPushTrailer(NetBuf* nb, unsigned size) {
	void* trailer = NULL;
	if (nb->pos + nb->len + size <= nb->capacity) {
		trailer = (u8*)netbufGet(nb) + nb->len;
		nb->len += size;
	}
	return trailer;
}

//! Extracts trailing data with the given @p size from the network buffer @p nb
MK_INLINE void* netbufPopTrailer(NetBuf* nb, unsigned size) {
	void* trailer = NULL;
	if (nb->len >= size) {
		trailer = (u8*)netbufGet(nb) + nb->len - size;
		nb->len -= size;
	}
	return trailer;
}

//! @private
MK_INLINE void netbufQueueAppend(NetBufListNode* q, NetBuf* nb) {
	nb->link.next = NULL;
	if (q->next) {
		q->prev->link.next = nb;
	} else {
		q->next = nb;
	}
	q->prev = nb;
}

//! @private
MK_INLINE NetBuf* netbufQueueRemoveOne(NetBufListNode* q) {
	NetBuf* ret = q->next;
	if (ret) {
		q->next = ret->link.next;
	}
	return ret;
}

//! @private
MK_INLINE NetBuf* netbufQueueRemoveAll(NetBufListNode* q) {
	return (NetBuf*)armSwapWord(0, (u32*)&q->next);
}

MK_EXTERN_C_END

//! @}
