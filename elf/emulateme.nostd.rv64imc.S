	.file	"emulateme.nostd.c"
	.option nopic
	.attribute arch, "rv64i2p1_m2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.section	.sdata,"aw"
	.align	3
	.type	uart_address, @object
	.size	uart_address, 8
uart_address:
	.dword	140737793775224
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
	sd	s0,40(sp)
	.cfi_offset 8, -8
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	sd	a0,-40(s0)
	sd	a1,-48(s0)
	sd	zero,-24(s0)
	j	.L2
.L3:
	ld	a4,-40(s0)
	ld	a5,-24(s0)
	add	a4,a4,a5
	lui	a5,%hi(uart_address)
	ld	a3,%lo(uart_address)(a5)
	lui	a5,%hi(uart_position)
	ld	a5,%lo(uart_position)(a5)
	add	a5,a3,a5
	lbu	a4,0(a4)
	sb	a4,0(a5)
	lui	a5,%hi(uart_position)
	ld	a5,%lo(uart_position)(a5)
	addi	a4,a5,1
	lui	a5,%hi(uart_position)
	sd	a4,%lo(uart_position)(a5)
	ld	a5,-24(s0)
	addi	a5,a5,1
	sd	a5,-24(s0)
.L2:
	ld	a4,-24(s0)
	ld	a5,-48(s0)
	bltu	a4,a5,.L3
	nop
	nop
	ld	s0,40(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 48
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE0:
	.size	uart_write_text, .-uart_write_text
	.section	.rodata
	.align	3
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
	sd	s0,40(sp)
	.cfi_offset 8, -8
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	mv	a5,a0
	sb	a5,-33(s0)
	lui	a5,%hi(.LC0)
	addi	a5,a5,%lo(.LC0)
	sd	a5,-24(s0)
	lbu	a5,-33(s0)
	srliw	a5,a5,4
	sb	a5,-25(s0)
	lbu	a5,-33(s0)
	andi	a5,a5,15
	sb	a5,-26(s0)
	lbu	a5,-25(s0)
	ld	a4,-24(s0)
	add	a4,a4,a5
	lui	a5,%hi(uart_address)
	ld	a3,%lo(uart_address)(a5)
	lui	a5,%hi(uart_position)
	ld	a5,%lo(uart_position)(a5)
	add	a5,a3,a5
	lbu	a4,0(a4)
	sb	a4,0(a5)
	lbu	a5,-26(s0)
	ld	a4,-24(s0)
	add	a4,a4,a5
	lui	a5,%hi(uart_address)
	ld	a3,%lo(uart_address)(a5)
	lui	a5,%hi(uart_position)
	ld	a5,%lo(uart_position)(a5)
	addi	a5,a5,1
	add	a5,a3,a5
	lbu	a4,0(a4)
	sb	a4,0(a5)
	lui	a5,%hi(uart_position)
	ld	a5,%lo(uart_position)(a5)
	addi	a4,a5,2
	lui	a5,%hi(uart_position)
	sd	a4,%lo(uart_position)(a5)
	nop
	ld	s0,40(sp)
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
	sd	s0,40(sp)
	.cfi_offset 8, -8
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	sd	a0,-40(s0)
	sd	zero,-24(s0)
	j	.L6
.L7:
	ld	a5,-40(s0)
	addi	a5,a5,1
	sd	a5,-40(s0)
	ld	a5,-24(s0)
	addi	a5,a5,1
	sd	a5,-24(s0)
.L6:
	ld	a5,-40(s0)
	lbu	a5,0(a5)
	bne	a5,zero,.L7
	ld	a5,-24(s0)
	mv	a0,a5
	ld	s0,40(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 48
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE2:
	.size	c_strlen, .-c_strlen
	.data
	.align	3
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
	sd	s0,56(sp)
	.cfi_offset 8, -8
	addi	s0,sp,64
	.cfi_def_cfa 8, 0
	sd	a0,-56(s0)
	sb	zero,-17(s0)
	sd	zero,-32(s0)
	j	.L10
.L11:
	lui	a5,%hi(seckrit)
	addi	a4,a5,%lo(seckrit)
	ld	a5,-32(s0)
	add	a5,a4,a5
	lbu	a4,0(a5)
	ld	a3,-56(s0)
	ld	a5,-32(s0)
	add	a5,a3,a5
	lbu	a5,0(a5)
	xor	a5,a4,a5
	andi	a4,a5,0xff
	lui	a5,%hi(seckrit)
	addi	a3,a5,%lo(seckrit)
	ld	a5,-32(s0)
	add	a5,a3,a5
	sb	a4,0(a5)
	lui	a5,%hi(seckrit)
	addi	a4,a5,%lo(seckrit)
	ld	a5,-32(s0)
	add	a5,a4,a5
	lbu	a5,0(a5)
	lbu	a4,-17(s0)
	xor	a5,a5,a4
	sb	a5,-17(s0)
	ld	a5,-32(s0)
	addi	a5,a5,1
	sd	a5,-32(s0)
.L10:
	ld	a4,-32(s0)
	li	a5,15
	bleu	a4,a5,.L11
	lbu	a5,-17(s0)
	andi	a4,a5,0xff
	li	a5,88
	beq	a4,a5,.L12
	li	a5,0
	j	.L13
.L12:
	sd	zero,-40(s0)
	j	.L14
.L15:
	lui	a5,%hi(seckrit)
	addi	a4,a5,%lo(seckrit)
	ld	a5,-40(s0)
	add	a5,a4,a5
	lbu	a5,0(a5)
	lbu	a4,-17(s0)
	xor	a5,a5,a4
	andi	a4,a5,0xff
	lui	a5,%hi(seckrit)
	addi	a3,a5,%lo(seckrit)
	ld	a5,-40(s0)
	add	a5,a3,a5
	sb	a4,0(a5)
	ld	a5,-40(s0)
	addi	a5,a5,1
	sd	a5,-40(s0)
.L14:
	ld	a4,-40(s0)
	li	a5,15
	bleu	a4,a5,.L15
	li	a5,1
.L13:
	mv	a0,a5
	ld	s0,56(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 64
	addi	sp,sp,64
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE3:
	.size	decrypt, .-decrypt
	.section	.rodata
	.align	3
.LC1:
	.string	"usage: emulateme [key]\n"
	.align	3
.LC2:
	.string	"wrong length.\n"
	.align	3
.LC3:
	.string	"wrong key.\n"
	.align	3
.LC4:
	.string	"Decrypted: "
	.align	3
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
	sd	ra,40(sp)
	sd	s0,32(sp)
	.cfi_offset 1, -8
	.cfi_offset 8, -16
	addi	s0,sp,48
	.cfi_def_cfa 8, 0
	mv	a5,a0
	sd	a1,-48(s0)
	sw	a5,-36(s0)
	lw	a5,-36(s0)
	sext.w	a4,a5
	li	a5,2
	beq	a4,a5,.L17
	li	a1,23
	lui	a5,%hi(.LC1)
	addi	a0,a5,%lo(.LC1)
	call	uart_write_text
	li	a5,1
	j	.L18
.L17:
	ld	a5,-48(s0)
	ld	a5,8(a5)
	sd	a5,-24(s0)
	ld	a0,-24(s0)
	call	c_strlen
	mv	a4,a0
	li	a5,16
	beq	a4,a5,.L19
	li	a1,14
	lui	a5,%hi(.LC2)
	addi	a0,a5,%lo(.LC2)
	call	uart_write_text
	li	a5,1
	j	.L18
.L19:
	ld	a0,-24(s0)
	call	decrypt
	mv	a5,a0
	bne	a5,zero,.L20
	li	a1,11
	lui	a5,%hi(.LC3)
	addi	a0,a5,%lo(.LC3)
	call	uart_write_text
	li	a5,1
	j	.L18
.L20:
	li	a1,11
	lui	a5,%hi(.LC4)
	addi	a0,a5,%lo(.LC4)
	call	uart_write_text
	li	a1,16
	lui	a5,%hi(seckrit)
	addi	a0,a5,%lo(seckrit)
	call	uart_write_text
	li	a1,1
	lui	a5,%hi(.LC5)
	addi	a0,a5,%lo(.LC5)
	call	uart_write_text
	li	a5,0
.L18:
	mv	a0,a5
	ld	ra,40(sp)
	.cfi_restore 1
	ld	s0,32(sp)
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
