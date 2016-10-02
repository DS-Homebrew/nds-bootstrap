#define PAGE_4K		(0b01011 << 1)
#define PAGE_8K		(0b01100 << 1)
#define PAGE_16K	(0b01101 << 1)
#define PAGE_32K	(0b01110 << 1)
#define PAGE_64K	(0b01111 << 1)
#define PAGE_128K	(0b10000 << 1)
#define PAGE_256K	(0b10001 << 1)
#define PAGE_512K	(0b10010 << 1)
#define PAGE_1M		(0b10011 << 1)
#define PAGE_2M		(0b10100 << 1)
#define PAGE_4M		(0b10101 << 1)
#define PAGE_8M		(0b10110 << 1)
#define PAGE_16M	(0b10111 << 1)
#define PAGE_32M	(0b11000 << 1)
#define PAGE_64M	(0b11001 << 1)
#define PAGE_128M	(0b11010 << 1)
#define PAGE_256M	(0b11011 << 1)
#define PAGE_512M	(0b11100 << 1)
#define PAGE_1G		(0b11101 << 1)
#define PAGE_2G		(0b11110 << 1)
#define PAGE_4G		(0b11111 << 1)

#define ITCM_LOAD	(1<<19)
#define ITCM_ENABLE	(1<<18)
#define DTCM_LOAD	(1<<17)
#define DTCM_ENABLE	(1<<16)
#define DISABLE_TBIT	(1<<15)
#define ROUND_ROBIN	(1<<14)
#define ALT_VECTORS	(1<<13)
#define ICACHE_ENABLE	(1<<12)
#define BIG_ENDIAN	(1<<7)
#define DCACHE_ENABLE	(1<<2)
#define PROTECT_ENABLE	(1<<0)

@---------------------------------------------------------------------------------
@ DS processor selection
@---------------------------------------------------------------------------------
	.arch	armv5te
	.cpu	arm946e-s

	.text
	.arm

	.global	__custom_mpu_setup
	.type	__custom_mpu_setup STT_FUNC
	
@---------------------------------------------------------------------------------
__custom_mpu_setup:

	ldr	r1, =0x00002078			@ disable TCM and protection unit
	mcr	p15, 0, r1, c1, c0

@---------------------------------------------------------------------------------
@ Protection Unit Setup added by Sasq
@---------------------------------------------------------------------------------
	@ Flush cache
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ Instruction cache
	mcr	p15, 0, r0, c7, c6, 0		@ Data cache

	@ Wait for write buffer to empty 
	mcr	p15, 0, r0, c7, c10, 4

@---------------------------------------------------------------------------------
@ Modify memory regions
@---------------------------------------------------------------------------------

dsi_mode: @access to New WRAM with region 3
	ldr	r1,=( PAGE_16M | 0x03000000 | 1)
	
	mrc	p15, 0, r9, c6, c3, 0
	adr	r0,region3
	str	r9,[r0]
	
	mcr	p15, 0, r1, c6, c3, 0
	

	
	ldr	r2,=( PAGE_16M | 0x02000000 | 1)
	
	mrc	p15, 0, r9, c6, c6, 0
	adr	r0,region6
	str	r9,[r0]
	
	@-------------------------------------------------------------------------
	@ Region 6 - non cacheable main ram
	@-------------------------------------------------------------------------
	mcr	p15, 0, r2, c6, c6, 0
	
	ldr	r3,=( PAGE_4M | 0x02000000 | 1)
	
	mrc	p15, 0, r9, c6, c7, 0
	adr	r0,region7
	str	r9,[r0]	
	
	@-------------------------------------------------------------------------
	@ Region 7 - cacheable main ram
	@-------------------------------------------------------------------------
	mcr	p15, 0, r3, c6, c7, 0

@setregions:

	@-------------------------------------------------------------------------
	@ Enable ICache, DCache, ITCM & DTCM
	@-------------------------------------------------------------------------
	mrc	p15, 0, r0, c1, c0, 0
	ldr	r1,= ITCM_ENABLE | DTCM_ENABLE | ICACHE_ENABLE | DCACHE_ENABLE | PROTECT_ENABLE
	orr	r0,r0,r1
	mcr	p15, 0, r0, c1, c0, 0

	bx	lr
	
region3:	.word	0
region6:	.word	0
region7:	.word	0

	.global	__custom_mpu_restore
	.type	__custom_mpu_restore STT_FUNC
	
@---------------------------------------------------------------------------------
__custom_mpu_restore:
	ldr	r1,=region3
	ldr	r2,[r1]
	mcr	p15, 0, r2, c6, c3, 0
	ldr	r1,=region6
	ldr	r2,[r1]
	mcr	p15, 0, r2, c6, c6, 0
	ldr	r1,=region7
	ldr	r2,[r1]
	mcr	p15, 0, r2, c6, c7, 0
	
	bx	lr
	
dsmasks:
	.word	0x003fffff, 0x02000000, 0x02c00000

masks:	.word	dsmasks
	
	.global myMemCached
	.type	myMemCached STT_FUNC
@---------------------------------------------------------------------------------
myMemCached:
@---------------------------------------------------------------------------------
	ldr	r1,masks
	ldr	r2,[r1],#4
	and	r0,r0,r2
	ldr	r2,[r1]
	orr	r0,r0,r2
	bx	lr

	.global	myMemUncached
	.type	myMemUncached STT_FUNC
@---------------------------------------------------------------------------------
myMemUncached:
@---------------------------------------------------------------------------------
	ldr	r1,masks
	ldr	r2,[r1],#8
	and	r0,r0,r2
	ldr	r2,[r1]
	orr	r0,r0,r2
	bx	lr