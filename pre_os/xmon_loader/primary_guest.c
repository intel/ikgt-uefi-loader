/*******************************************************************************
* Copyright (c) 2015 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "mon_defs.h"
#include "msr_defs.h"
#include "ia32_defs.h"
#include "mon_arch_defs.h"
#include "mon_startup.h"
#include "xmon_desc.h"
#include "common.h"
#include "screen.h"
#include "memory.h"
#include "error_code.h"
#include "ikgtboot.h"
#include "em64t_defs.h"

uint32_t __readcs(void_t)
{
	__asm__ __volatile__ ("xor %rax, %rax; mov %cs, %ax\n");
}

uint32_t __readds(void_t)
{
	__asm__ __volatile__ ("xor %rax, %rax; mov %ds, %ax\n");
}

uint32_t __reades(void_t)
{
	__asm__ __volatile__ ("xor %rax, %rax; mov %es, %ax\n");
}

uint32_t __readfs(void_t)
{
	__asm__ __volatile__ ("xor %rax, %rax; mov %fs, %ax\n");
}

uint32_t __readgs(void_t)
{
	__asm__ __volatile__ ("xor %rax, %rax; mov %gs, %ax\n");
}

uint32_t __readss(void_t)
{
	__asm__ __volatile__ ("xor %rax, %rax; mov %ss, %ax\n");
}

uint16_t __readtr(void_t)
{
	__asm__ __volatile__ (
		"str %ax\n"
		);
}

void_t __readgdtr(void *p)
{
	__asm__ __volatile__ ("sgdt (%0)" : : "D" (p) : );
}

uint16_t __readldtr(void_t)
{
	__asm__ __volatile__ (
		"sldt %ax\n"
		);
}

uint16_t __sidt(void *p)
{
	__asm__ __volatile__ ("sidt (%0)" : : "D" (p) : );
}

uint32_t __readcr0(void_t)
{
	__asm__ __volatile__ ("mov %cr0, %rax\n");
}

uint32_t __readcr2(void_t)
{
	__asm__ __volatile__ ("mov %cr2, %rax\n");
}

uint32_t __readcr3(void_t)
{
	__asm__ __volatile__ ("mov %cr3, %rax\n");
}

uint32_t __readcr4(void_t)
{
	__asm__ __volatile__ ("mov %cr4, %rax\n");
}

uint64_t __readmsr(uint32_t msr_id)
{
	__asm__ __volatile__ (
		"movl %0, %%ecx\n"
		"xor %%rax, %%rax\n"
		"rdmsr\n"
		:
		: "g" (msr_id)
		: "%ecx", "%rax", "%rdx"
		);
}

void __cpuid(int cpu_info[4], int info_type);

static int validate_descriptor(ia32_segment_descriptor_t *d)
{
	if ((d->gen.hi.present == 0) || (d->gen.hi.mbz_21 == 1)) {
		return -1;
	}

	if (d->gen.hi.s == 0) {
		if ((d->tss.hi.mbo_8 == 0) ||
		    (d->tss.hi.mbz_10 == 1) ||
		    (d->tss.hi.mbo_11 == 0) ||
		    (d->tss.hi.mbz_12 == 1) ||
		    (d->tss.hi.mbz_21 == 1) || (d->tss.hi.mbz_22 == 1)) {
			return -1;
		}
	} else {
		if ((d->gen_attr.attributes & 0x0008) != 0) {
			if ((d->cs.hi.mbo_11 == 0) ||
			    (d->cs.hi.mbo_12 == 0) || (d->cs.hi.default_size == 0)) {
				return -1;
			}
		} else {
			if ((d->ds.hi.mbz_11 == 1) ||
			    (d->ds.hi.mbo_12 == 0) || (d->ds.hi.big == 0)) {
				return -1;
			}
		}
	}

	return 0;
}

static void save_segment_data(uint16_t sel16, mon_segment_struct_t *ss)
{
	em64t_gdtr_t gdtr;
	ia32_selector_t sel;
	ia32_segment_descriptor_t *desc;
	ia32_segment_descriptor_attr_t attr;
	unsigned int max;

	/* Check the limit, it will be 0 to
	 * (size_of_each_entry * num_entries) - 1. So max will have the
	 * last valid entry */
	__readgdtr(&gdtr);
	max = (uint32_t)gdtr.limit / sizeof(ia32_segment_descriptor_t);

	sel.sel16 = sel16;

	if ((sel.bits.index == 0) ||
	    ((sel.bits.index / sizeof(ia32_segment_descriptor_t)) > max) ||
	    (sel.bits.ti)) {
		return;
	}

	desc = (ia32_segment_descriptor_t *)
	       (gdtr.base + sizeof(ia32_segment_descriptor_t) * sel.bits.index);
#if 0 /*TODO: 64bit not have TSS, cleanup*/
	if (validate_descriptor(desc) != 0) {
		return;
	}
#endif
	ss->base = (uint64_t)((desc->gen.lo.base_address_15_00) |
			      (desc->gen.hi.base_address_23_16 << 16) |
			      (desc->gen.hi.base_address_31_24 << 24)
			      );

	ss->limit = (uint32_t)((desc->gen.lo.limit_15_00) |
			       (desc->gen.hi.limit_19_16 << 16)
			       );

	if (desc->gen.hi.granularity) {
		ss->limit = (ss->limit << 12) | 0x00000fff;
	}

	attr.attr16 = desc->gen_attr.attributes;
	attr.bits.limit_19_16 = 0;

	ss->attributes = (uint32_t)attr.attr16;
	ss->selector = sel.sel16;
	return;
}

void save_other_cpu_state(mon_guest_cpu_startup_state_t *s)
{
	em64t_gdtr_t gdtr;
	em64t_idt_descriptor_t idtr;
	ia32_selector_t sel;
	ia32_segment_descriptor_t *desc;

	__readgdtr(&gdtr);
	__sidt(&idtr);
	s->control.gdtr.base = (uint64_t)gdtr.base;
	s->control.gdtr.limit = (uint32_t)gdtr.limit;
	s->control.idtr.base = (uint64_t)idtr.base;
	s->control.idtr.limit = (uint32_t)idtr.limit;
	s->control.cr[IA32_CTRL_CR0] = __readcr0();
	s->control.cr[IA32_CTRL_CR2] = __readcr2();
	s->control.cr[IA32_CTRL_CR3] = __readcr3();
	s->control.cr[IA32_CTRL_CR4] = __readcr4();

	s->msr.msr_sysenter_cs = (uint32_t)__readmsr(IA32_MSR_SYSENTER_CS);
	s->msr.msr_sysenter_eip = __readmsr(IA32_MSR_SYSENTER_EIP);
	s->msr.msr_sysenter_esp = __readmsr(IA32_MSR_SYSENTER_ESP);
	s->msr.msr_efer = __readmsr(IA32_MSR_EFER);
	s->msr.msr_pat = __readmsr(IA32_MSR_PAT);
	s->msr.msr_debugctl = __readmsr(IA32_MSR_DEBUGCTL);
	s->msr.pending_exceptions = 0;
	s->msr.interruptibility_state = 0;
	s->msr.activity_state = 0;
	s->msr.smbase = 0;

	sel.sel16 = __readldtr();

	if (sel.bits.index != 0) {
		print_string("Invalid sel.bits.index\n");
		return;
	}

	s->seg.segment[IA32_SEG_LDTR].attributes = 0x00010000;
	s->seg.segment[IA32_SEG_TR].attributes = 0x0000808b;
	s->seg.segment[IA32_SEG_TR].limit = 0xffffffff;
	save_segment_data((uint16_t)__readcs(), &s->seg.segment[IA32_SEG_CS]);
	save_segment_data((uint16_t)__readds(), &s->seg.segment[IA32_SEG_DS]);
	save_segment_data((uint16_t)__reades(), &s->seg.segment[IA32_SEG_ES]);
	save_segment_data((uint16_t)__readfs(), &s->seg.segment[IA32_SEG_FS]);
	save_segment_data((uint16_t)__readgs(), &s->seg.segment[IA32_SEG_GS]);
	save_segment_data((uint16_t)__readss(), &s->seg.segment[IA32_SEG_SS]);
	return;
}


/*
 * setup primary guest initial environment,
 * here only assign return-RIP and a parameter in RCX, all the other
 * states are currently setup in starter_main() function in starter component.
 *
 */
mon_guest_startup_t *setup_primary_guest_env(xmon_desc_t *xd)
{
	mon_guest_cpu_startup_state_t *primary_guest_bsp_cpu;
	mon_guest_startup_t *primary_guest;
	uint32_t num_of_cpu = 1;                   /* initially, only one (BSP) for pre-os */

	/* save primary guest cpu state (BSP) */
	primary_guest_bsp_cpu = (mon_guest_cpu_startup_state_t *)
				allocate_memory(num_of_cpu *
		sizeof(mon_guest_cpu_startup_state_t));
	if (!primary_guest_bsp_cpu) {
		print_string("Heap Out of Memory (mon_guest_cpu_startup_state_t)\n");
		return NULL;
	}

	primary_guest_bsp_cpu->size_of_this_struct =
		sizeof(mon_guest_cpu_startup_state_t);
	primary_guest_bsp_cpu->version_of_this_struct =
		MON_GUEST_CPU_STARTUP_STATE_VERSION;

	/*
	 * here control the instruction returned after xmon starts
	 */

	primary_guest_bsp_cpu->gp.reg[IA32_REG_RIP] = xd->initial_state.rip;
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RSP] = xd->initial_state.rsp;         /* use starter stack */
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RBX] = xd->initial_state.rbx;         /* boot info */
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RAX] = xd->initial_state.rax;         /* boot magic */
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RFLAGS] = xd->initial_state.rflags;

	/* use ecx to pass on xmon_desc_t *xd */
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RCX] = (uint64_t)(xd);

	primary_guest_bsp_cpu->gp.reg[IA32_REG_RDX] = xd->initial_state.rdx;
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RBP] = xd->initial_state.rbp;
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RSI] = xd->initial_state.rsi;
	primary_guest_bsp_cpu->gp.reg[IA32_REG_RDI] = xd->initial_state.rdi;

	save_other_cpu_state(primary_guest_bsp_cpu);


	/* construct primary guest state */
	{
		primary_guest =
			(mon_guest_startup_t *)allocate_memory(sizeof(
					mon_guest_startup_t));
		if (!primary_guest) {
			return NULL;;
		}

		primary_guest->size_of_this_struct = sizeof(mon_guest_startup_t);
		primary_guest->version_of_this_struct = MON_GUEST_STARTUP_VERSION;
		primary_guest->flags = MON_GUEST_FLAG_LAUNCH_IMMEDIATELY |
				       MON_GUEST_FLAG_REAL_BIOS_ACCESS_ENABLE;
		primary_guest->guest_magic_number = 0;                  /* hard code magic number */
		primary_guest->cpu_affinity = (uint32_t)(-1);           /* Run on all available CPUs */
		primary_guest->cpu_states_count = num_of_cpu;           /* set it as 1 for pre-so luanch*/
		primary_guest->devices_count = 0;                       /* No virtualized devices */

		/* The following are set to 0 since this is a primary guest */
		primary_guest->image_size = 0;
		primary_guest->image_address = 0;
		primary_guest->physical_memory_size = 0;
		primary_guest->image_offset_in_guest_physical_memory = 0;

		primary_guest->cpu_states_array =
			(uint64_t)(primary_guest_bsp_cpu);
		primary_guest->devices_array = 0;                   /* No virtualized devices */
	}

	return primary_guest;
}
