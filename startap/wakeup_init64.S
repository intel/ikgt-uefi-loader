################################################################################
# Copyright (c) 2015 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
.file   "wakeup_init64.s"

#include "msr_defs.h"

#define STARTAP_STACK_SIZE 0X400
#define MAX_CPUS       80

#define MON_MAX_CPU_SUPPORTED MAX_CPUS

.text

/*
stage_1:
1. fill the ap_presence_array to enumerate the AP#
2. wait the BSP to set mp_bootstrap_state=1, then jump to stage_2
%ecx -- saved the loacal_apic_id
*/
.globl ap_continue_wakeup_code
ap_continue_wakeup_code:
	leal ap_presence_array(%rip), %edx

	addl %ecx, %edx
	movb $1, (%edx)
wait_lock_1:
	xor %ecx, %ecx
	leal mp_bootstrap_state(%rip), %ecx
	cmpl $1, (%ecx)

	je stage_2
	pause
	jmp wait_lock_1

/*
stage_2:(after the bootstrap_state has been set 1 by BSP)
1. setup stacks for each APs
2. jump to the wakeup_code_C(), which will later call into the xmon_entry()
*/
stage_2:
	xor %rcx, %rcx
	xor %rax, %rax
	movb (%edx), %cl 	# now ecx contains AP ordered ID [1..Max]

#setup the stack for each AP, according the ordered ID
	xor %rdx, %rdx
	mov %ecx, %edx  #now the %edx saved the AP ordered ID
	lea apstack_start(%rip), %rax
	shll $10, %ecx #currently, the stack size used 0x400, it can be redueced!
	add %rcx, %rax
	sub $4, %rax
	mov %rax, %rsp

	push $0
	popf

	mov %rdx, %rdi # push  AP ordered ID
	call ap_continue_wakeup_code_C
# should never return
	jmp .

/*
 * convert the params as below:
 * call_xmon_entry (rdi,  rsi, rdx, rcx, r8)
 *            xmon_entry (rcx, rdx, r8,  r9)
*/
.globl call_xmon_entry
call_xmon_entry:
	cli
	xor %r9, %r9
	mov %r8, %r9

	xor %r8, %r8
	mov %rcx, %r8

	mov %rdx, %rdx #rdx save the xmon_struct

	xor %rcx, %rcx
	mov %rsi, %rcx #rcx save the cpuid

	xor %rbx, %rbx
	mov %rdi, %rbx #rbx will save the xmon_entry addr
	call *%rbx
    #should never return
	jmp .


.bss
/*
* stacks for each AP
*/
.align  4
apstack_start:
	.fill STARTAP_STACK_SIZE*(MON_MAX_CPU_SUPPORTED-1), 1, 0xCC
apstack:
