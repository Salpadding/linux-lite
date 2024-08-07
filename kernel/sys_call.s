SIG_CHLD	= 17

EAX		= 0x00
EBX		= 0x04
ECX		= 0x08
EDX		= 0x0C
ORIG_EAX	= 0x10
FS		= 0x14
ES		= 0x18
DS		= 0x1C
EIP		= 0x20
CS		= 0x24
EFLAGS		= 0x28
OLDESP		= 0x2C
OLDSS		= 0x30

state	= 0		# these are offsets into the task-struct.
counter	= 4
priority = 8
signal	= 12
sigaction = 16		# MUST be 16 (=len of sigaction)
blocked = (33*16)

# offsets within sigaction
sa_handler = 0
sa_mask = 4
sa_flags = 8
sa_restorer = 12

nr_system_calls = 82

ENOSYS = 38

/*
 * Ok, I get parallel printer interrupts while using the floppy for some
 * strange reason. Urgel. Now I just ignore them.
 */
.globl system_call,timer_interrupt,sys_fork,coprocessor_error,device_not_available

.align 4
bad_sys_call:
	pushl $-ENOSYS
	jmp ret_from_sys_call
.align 4
reschedule:
	pushl $ret_from_sys_call
	jmp schedule
.align 4
system_call: /* 这里没有切 ldt */
	push %ds
	push %es
	push %fs
	pushl %eax		# save the orig_eax
	pushl %edx		
	pushl %ecx		# push %ebx,%ecx,%edx as parameters
	pushl %ebx		# to the system call
	movl $0x10,%edx		# set up ds,es to kernel space
	mov %dx,%ds
	mov %dx,%es
	movl $0x17,%edx		# fs points to local data space
	mov %dx,%fs /* 保留一个fs  指向用户态的地址空间 */
	cmpl NR_syscalls,%eax 
	jae bad_sys_call  /* 触发条件 %eax >= bad_sys_call */
	call sys_call_table(,%eax,4)
	pushl %eax /* 保存 sys_函数的返回值 */
2:
	movl current,%eax
	cmpl $0,state(%eax)		# state
	jne reschedule /* current.state != running */
	cmpl $0,counter(%eax) /* current.counter = 0 时间片用完了 */
	je reschedule
ret_from_sys_call:
	movl current,%eax
	cmpl task,%eax			# task[0] cannot have signals
	je 3f
	cmpw $0x0f,CS(%esp)		# was old code segment supervisor ?
	jne 3f
	cmpw $0x17,OLDSS(%esp)		# was stack segment = 0x17 ?
	jne 3f
	movl signal(%eax),%ebx  /* 信号位图 */
	movl blocked(%eax),%ecx #
	notl %ecx 
	andl %ebx,%ecx
	bsfl %ecx,%ecx /* bitscan */
	je 3f /* no signal found */
	btrl %ecx,%ebx /* btrl clear bit */
	movl %ebx,signal(%eax) /* clear the signal, since we will handle it */
	incl %ecx /* signal number is started at 1 */
	pushl %ecx
	call do_signal
	popl %ecx /* recover stack */
	testl %eax, %eax /* check if do_signal() */
	jne 2b		# do_signal() != 0, then we need to reschedule
3:	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	addl $4, %esp	# skip orig_eax
	pop %fs
	pop %es
	pop %ds
	iret

.align 4
timer_interrupt:
	push %ds		# save ds,es and put kernel data space
	push %es		# into them. %fs is used by _system_call
	push %fs
	pushl $-1		# fill in -1 for orig_eax
	pushl %edx		# we save %eax,%ecx,%edx as gcc doesn't
	pushl %ecx		# save those across function calls. %ebx
	pushl %ebx		# is saved as we use that in ret_sys_call
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	incl jiffies
	movb $0x20,%al		# EOI to interrupt controller #1
	outb %al,$0x20
	movl CS(%esp),%eax
	andl $3,%eax		# %eax is CPL (0 or 3, 0=supervisor)
	pushl %eax
	call do_timer		# 'do_timer(long CPL)' does everything from
	addl $4,%esp		# task switching to accounting ...
	jmp ret_from_sys_call

.align 4
sys_fork:
	call find_empty_process
	testl %eax,%eax
	js 1f
	push %gs
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call copy_process
	addl $20,%esp
1:	ret


.align 4
coprocessor_error:
	push %ds
	push %es
	push %fs
	pushl $-1		# fill in -1 for orig_eax
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	pushl $ret_from_sys_call
	jmp math_error

.align 4

device_not_available:
	push %ds
	push %es
	push %fs
	pushl $-1		# fill in -1 for orig_eax
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	pushl $ret_from_sys_call
	clts				# clear TS so that we can use math
	movl %cr0,%eax
	jmp math_state_restore
