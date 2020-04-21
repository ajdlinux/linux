/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * PowerNV OPAL API wrappers
 *
 * Copyright 2011 IBM Corp.
 */

#include <linux/jump_label.h>
#include <asm/ppc_asm.h>
#include <asm/hvcall.h>
#include <asm/asm-offsets.h>
#include <asm/opal.h>
#include <asm/asm-compat.h>
#include <asm/feature-fixups.h>

	.section ".text"

/*
 * Current thought:
	stash the MSR in r1, rather than the stack pointer
	store the stack pointer in the PACA - we keep PACA pointer in r13????
	ah wait, the PACA is going to keep the base of the stack, not the current stack pointer??
	it looks like skiboot only restores LR (to jump to opal_return), R1, R13?????
	I don't think we can just enable IR/DR in the MSR all the time in the return path, because we might be calling this from real mode already?????
	stash the MSR in r13, rather than the PACA, keep a pointer to the PACA on the stack that we can recover??????????
 * r3-r10		- OPAL call arguments
 * STK_PARAM(R11)	- OPAL opcode
 * STK_PARAM(R12)	- MSR to restore
 */
_GLOBAL_TOC(__opal_call)
	mflr	r0			/* Store current LR on stack */
	std	r0,PPC_LR_STKOFF(r1)	/*                           */

	LOAD_REG_ADDR(r11, opal_return)
	mtlr	r11
	LOAD_REG_ADDR(r11, opal)
	ld	r2,0(r11)
	ld	r11,8(r11)
	mtspr	SPRN_HSRR0,r11

	/* set token to r0 */
	ld	r0,STK_PARAM(R11)(r1)

	/* Save PACA pointer from R13 on to the stack */
	std	r13,STK_PARAM(R11)(r1) /* lol we need a better place than this surely */

	/* MSR */
	ld	r13,STK_PARAM(R12)(r1)  /* Load MSR to restore off stack... */
	li	r11,MSR_IR|MSR_DR|MSR_LE /* Disable these bits */
	andc	r11,r13,r11              /* */

	mtspr	SPRN_HSRR1,r11
	hrfid
opal_return:
	/*
	 * Restore MSR on OPAL return. The MSR is set to big-endian.
	 */
#ifdef __BIG_ENDIAN__
//	ld	r11,STK_PARAM(R12)(r1)
	mtmsrd	r13
#else
	/* Endian can only be switched with rfi, must byte reverse MSR load */
//	.short 0x4039	 /* li r10,STK_PARAM(R12)		*/ // R10 = stack offset of new MSR
//	.byte (STK_PARAM(R12) >> 8) & 0xff
//	.byte STK_PARAM(R12) & 0xff

//	.long 0x280c6a7d /* ldbrx r11,r10,r1			*/ // R11 = new MSR
	.long 0x05009f42 /* bcl 20,31,$+4			*/ // LR = next insn addr
	.long 0xa602487d /* mflr r10				*/ // R10 = LR = addr of this insn
	.long 0x14004a39 /* addi r10,r10,20			*/ // R10 = addr of insn after endif
	.long 0xa64b5a7d /* mthsrr0 r10				*/ // New NIP
	//.long 0xa64b7b7d /* mthsrr1 r11				*/ // New MSR
	.long 0xa64bbb7d /* mthsrr1 r13 */
	.long 0x2402004c /* hrfid				*/
#endif
	/* Restore PACA */
	ld	r13,STK_PARAM(R11)(r1)
	ld	r2,PACATOC(r13)
	ld	r0,PPC_LR_STKOFF(r1)
	mtlr	r0
	blr
