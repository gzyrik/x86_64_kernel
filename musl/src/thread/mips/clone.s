.set noreorder
.global __clone
.type   __clone,@function
__clone:
	# Save function pointer and argument pointer on new thread stack
	subu $5, $5, 16
	sw $4, 0($5)
	sw $7, 4($5)
	# Shuffle (fn,sp,fl,arg,ptid,tls,ctid) to (fl,sp,ptid,tls,ctid)
	move $4, $6
	lw $6, 16($sp)
	lw $7, 20($sp)
	lw $9, 24($sp)
	sw $9, 16($sp)
	li $2, 4120
	syscall
	beq $7, $0, 1f
	nop
	jr $ra
	subu $2, $0, $2
1:	beq $2, $0, 1f
	nop
	jr $ra
	nop
1:	lw $25, 0($sp)
	lw $4, 4($sp)
	jr $25
	nop
