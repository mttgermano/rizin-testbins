	.file	"emulateme.nostd.c"
	.option nopic
	.attribute arch, "rv32i2p1_m2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.section	.sdata,"aw"
	.align	2
	.type	uart_address, @object
	.size	uart_address, 4
uart_address:
	.word	305419896
	.local	uart_position
	.comm	uart_position,8,8
	.text
	.align	1
	.globl	uart_write_text
	.type	uart_write_text, @function
uart_write_text:
.LFB0:
	.cfi_startproc
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
	sw	s0,44(sp)
	.cfi_offset 8, -4
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	sw	a0,-36(s0)
	sw	a1,-48(s0)
	sw	a2,-44(s0)
	li	a5,0
	li	a6,0
	sw	a5,-24(s0)
	sw	a6,-20(s0)
	j	.L2
.L3:
	lw	a5,-24(s0)
	lw	a4,-36(s0)
	add	a3,a4,a5
	lui	a5,%hi(uart_address)
	lw	a2,%lo(uart_address)(a5)
	lui	a5,%hi(uart_position)
	lw	a4,%lo(uart_position)(a5)
	lw	a5,%lo(uart_position+4)(a5)
	mv	a5,a4
	add	a5,a2,a5
	lbu	a4,0(a3)
	sb	a4,0(a5)
	lui	a5,%hi(uart_position)
	lw	a4,%lo(uart_position)(a5)
	lw	a5,%lo(uart_position+4)(a5)
	li	a0,1
	li	a1,0
	add	a2,a4,a0
	mv	a6,a2
	sltu	a6,a6,a4
	add	a3,a5,a1
	add	a5,a6,a3
	mv	a3,a5
	mv	a4,a2
	mv	a5,a3
	lui	a3,%hi(uart_position)
	sw	a4,%lo(uart_position)(a3)
	sw	a5,%lo(uart_position+4)(a3)
	lw	a2,-24(s0)
	lw	a3,-20(s0)
	li	a0,1
	li	a1,0
	add	a4,a2,a0
	mv	a6,a4
	sltu	a6,a6,a2
	add	a5,a3,a1
	add	a3,a6,a5
	mv	a5,a3
	sw	a4,-24(s0)
	sw	a5,-20(s0)
.L2:
	lw	a4,-44(s0)
	lw	a5,-20(s0)
	bgtu	a4,a5,.L3
	lw	a4,-44(s0)
	lw	a5,-20(s0)
	bne	a4,a5,.L5
	lw	a4,-48(s0)
	lw	a5,-24(s0)
	bgtu	a4,a5,.L3
.L5:
	nop
	lw	s0,44(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 48
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE0:
	.size	uart_write_text, .-uart_write_text
	.section	.rodata
	.align	2
.LC0:
	.string	"0123456789abcdef"
	.text
	.align	1
	.globl	uart_write_hex
	.type	uart_write_hex, @function
uart_write_hex:
.LFB1:
	.cfi_startproc
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
	sw	s0,44(sp)
	.cfi_offset 8, -4
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	mv	a5,a0
	sb	a5,-33(s0)
	lui	a5,%hi(.LC0)
	addi	a5,a5,%lo(.LC0)
	sw	a5,-20(s0)
	lbu	a5,-33(s0)
	srli	a5,a5,4
	sb	a5,-21(s0)
	lbu	a5,-33(s0)
	andi	a5,a5,15
	sb	a5,-22(s0)
	lbu	a5,-21(s0)
	lw	a4,-20(s0)
	add	a3,a4,a5
	lui	a5,%hi(uart_address)
	lw	a2,%lo(uart_address)(a5)
	lui	a5,%hi(uart_position)
	lw	a4,%lo(uart_position)(a5)
	lw	a5,%lo(uart_position+4)(a5)
	mv	a5,a4
	add	a5,a2,a5
	lbu	a4,0(a3)
	sb	a4,0(a5)
	lbu	a5,-22(s0)
	lw	a4,-20(s0)
	add	a3,a4,a5
	lui	a5,%hi(uart_address)
	lw	a2,%lo(uart_address)(a5)
	lui	a5,%hi(uart_position)
	lw	a4,%lo(uart_position)(a5)
	lw	a5,%lo(uart_position+4)(a5)
	mv	a5,a4
	addi	a5,a5,1
	add	a5,a2,a5
	lbu	a4,0(a3)
	sb	a4,0(a5)
	lui	a5,%hi(uart_position)
	lw	a4,%lo(uart_position)(a5)
	lw	a5,%lo(uart_position+4)(a5)
	li	a0,2
	li	a1,0
	add	a2,a4,a0
	mv	a6,a2
	sltu	a6,a6,a4
	add	a3,a5,a1
	add	a5,a6,a3
	mv	a3,a5
	mv	a4,a2
	mv	a5,a3
	lui	a3,%hi(uart_position)
	sw	a4,%lo(uart_position)(a3)
	sw	a5,%lo(uart_position+4)(a3)
	nop
	lw	s0,44(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 48
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE1:
	.size	uart_write_hex, .-uart_write_hex
	.align	1
	.globl	c_strlen
	.type	c_strlen, @function
c_strlen:
.LFB2:
	.cfi_startproc
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
	sw	s0,44(sp)
	.cfi_offset 8, -4
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	sw	a0,-36(s0)
	li	a5,0
	li	a6,0
	sw	a5,-24(s0)
	sw	a6,-20(s0)
	j	.L8
.L9:
	lw	a5,-36(s0)
	addi	a5,a5,1
	sw	a5,-36(s0)
	lw	a2,-24(s0)
	lw	a3,-20(s0)
	li	a0,1
	li	a1,0
	add	a4,a2,a0
	mv	a6,a4
	sltu	a6,a6,a2
	add	a5,a3,a1
	add	a3,a6,a5
	mv	a5,a3
	sw	a4,-24(s0)
	sw	a5,-20(s0)
.L8:
	lw	a5,-36(s0)
	lbu	a5,0(a5)
	bne	a5,zero,.L9
	lw	a4,-24(s0)
	lw	a5,-20(s0)
	mv	a0,a4
	mv	a1,a5
	lw	s0,44(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 48
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE2:
	.size	c_strlen, .-c_strlen
	.data
	.align	2
	.type	seckrit, @object
	.size	seckrit, 17
seckrit:
	.string	"QSMwX\024Q_El\027\177nx\177\034"
	.text
	.align	1
	.globl	decrypt
	.type	decrypt, @function
decrypt:
.LFB3:
	.cfi_startproc
	addi	sp,sp,-64
	.cfi_def_cfa_offset 64
	sw	s0,60(sp)
	.cfi_offset 8, -4
	addi	s0,sp,64
	.cfi_def_cfa 8, 0
	sw	a0,-52(s0)
	sb	zero,-17(s0)
	li	a5,0
	li	a6,0
	sw	a5,-32(s0)
	sw	a6,-28(s0)
	j	.L12
.L13:
	lui	a5,%hi(seckrit)
	addi	a4,a5,%lo(seckrit)
	lw	a5,-32(s0)
	add	a5,a4,a5
	lbu	a4,0(a5)
	lw	a5,-32(s0)
	lw	a3,-52(s0)
	add	a5,a3,a5
	lbu	a5,0(a5)
	xor	a5,a4,a5
	andi	a4,a5,0xff
	lui	a5,%hi(seckrit)
	addi	a3,a5,%lo(seckrit)
	lw	a5,-32(s0)
	add	a5,a3,a5
	sb	a4,0(a5)
	lui	a5,%hi(seckrit)
	addi	a4,a5,%lo(seckrit)
	lw	a5,-32(s0)
	add	a5,a4,a5
	lbu	a5,0(a5)
	lbu	a4,-17(s0)
	xor	a5,a5,a4
	sb	a5,-17(s0)
	lw	a2,-32(s0)
	lw	a3,-28(s0)
	li	a0,1
	li	a1,0
	add	a4,a2,a0
	mv	a6,a4
	sltu	a6,a6,a2
	add	a5,a3,a1
	add	a3,a6,a5
	mv	a5,a3
	sw	a4,-32(s0)
	sw	a5,-28(s0)
.L12:
	lw	a5,-28(s0)
	bne	a5,zero,.L20
	lw	a5,-28(s0)
	bne	a5,zero,.L13
	lw	a4,-32(s0)
	li	a5,15
	bleu	a4,a5,.L13
.L20:
	lbu	a4,-17(s0)
	li	a5,88
	beq	a4,a5,.L15
	li	a5,0
	j	.L16
.L15:
	li	a5,0
	li	a6,0
	sw	a5,-40(s0)
	sw	a6,-36(s0)
	j	.L17
.L18:
	lui	a5,%hi(seckrit)
	addi	a4,a5,%lo(seckrit)
	lw	a5,-40(s0)
	add	a5,a4,a5
	lbu	a5,0(a5)
	lbu	a4,-17(s0)
	xor	a5,a5,a4
	andi	a4,a5,0xff
	lui	a5,%hi(seckrit)
	addi	a3,a5,%lo(seckrit)
	lw	a5,-40(s0)
	add	a5,a3,a5
	sb	a4,0(a5)
	lw	a2,-40(s0)
	lw	a3,-36(s0)
	li	a0,1
	li	a1,0
	add	a4,a2,a0
	mv	a6,a4
	sltu	a6,a6,a2
	add	a5,a3,a1
	add	a3,a6,a5
	mv	a5,a3
	sw	a4,-40(s0)
	sw	a5,-36(s0)
.L17:
	lw	a5,-36(s0)
	bne	a5,zero,.L21
	lw	a5,-36(s0)
	bne	a5,zero,.L18
	lw	a4,-40(s0)
	li	a5,15
	bleu	a4,a5,.L18
.L21:
	li	a5,1
.L16:
	mv	a0,a5
	lw	s0,60(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 64
	addi	sp,sp,64
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE3:
	.size	decrypt, .-decrypt
	.section	.rodata
	.align	2
.LC1:
	.string	"usage: emulateme [key]\n"
	.align	2
.LC2:
	.string	"wrong length.\n"
	.align	2
.LC3:
	.string	"wrong key.\n"
	.align	2
.LC4:
	.string	"Decrypted: "
	.align	2
.LC5:
	.string	"\n"
	.text
	.align	1
	.globl	main
	.type	main, @function
main:
.LFB4:
	.cfi_startproc
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
	sw	ra,44(sp)
	sw	s0,40(sp)
	.cfi_offset 1, -4
	.cfi_offset 8, -8
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	sw	a0,-36(s0)
	sw	a1,-40(s0)
	lw	a4,-36(s0)
	li	a5,2
	beq	a4,a5,.L23
	li	a1,23
	li	a2,0
	lui	a5,%hi(.LC1)
	addi	a0,a5,%lo(.LC1)
	call	uart_write_text
	li	a5,1
	j	.L24
.L23:
	lw	a5,-40(s0)
	lw	a5,4(a5)
	sw	a5,-20(s0)
	lw	a0,-20(s0)
	call	c_strlen
	mv	a4,a0
	mv	a5,a1
	mv	a2,a4
	li	a3,16
	bne	a2,a3,.L28
	beq	a5,zero,.L29
.L28:
	li	a1,14
	li	a2,0
	lui	a5,%hi(.LC2)
	addi	a0,a5,%lo(.LC2)
	call	uart_write_text
	li	a5,1
	j	.L24
.L29:
	lw	a0,-20(s0)
	call	decrypt
	mv	a5,a0
	bne	a5,zero,.L27
	li	a1,11
	li	a2,0
	lui	a5,%hi(.LC3)
	addi	a0,a5,%lo(.LC3)
	call	uart_write_text
	li	a5,1
	j	.L24
.L27:
	li	a1,11
	li	a2,0
	lui	a5,%hi(.LC4)
	addi	a0,a5,%lo(.LC4)
	call	uart_write_text
	li	a1,16
	li	a2,0
	lui	a5,%hi(seckrit)
	addi	a0,a5,%lo(seckrit)
	call	uart_write_text
	li	a1,1
	li	a2,0
	lui	a5,%hi(.LC5)
	addi	a0,a5,%lo(.LC5)
	call	uart_write_text
	li	a5,0
.L24:
	mv	a0,a5
	lw	ra,44(sp)
	.cfi_restore 1
	lw	s0,40(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 48
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE4:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
