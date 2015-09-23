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
/* IA-32 Exception Bitmap */

#include <mon_defs.h>
#include <mon_arch_defs.h>
#include <xmon_loader.h>
#include <memory.h>
#include <screen.h>


typedef enum {
	IA32_EXCEPTION_VECTOR_DIVIDE_ERROR,
	IA32_EXCEPTION_VECTOR_DEBUG_BREAK_POINT,
	IA32_EXCEPTION_VECTOR_NMI,
	IA32_EXCEPTION_VECTOR_BREAK_POINT,
	IA32_EXCEPTION_VECTOR_OVERFLOW,
	IA32_EXCEPTION_VECTOR_BOUND_RANGE_EXCEEDED,
	IA32_EXCEPTION_VECTOR_UNDEFINED_OPCODE,
	IA32_EXCEPTION_VECTOR_NO_MATH_COPROCESSOR,
	IA32_EXCEPTION_VECTOR_DOUBLE_FAULT,
	IA32_EXCEPTION_VECTOR_RESERVED_0X09,
	IA32_EXCEPTION_VECTOR_INVALID_TASK_SEGMENT_SELECTOR,
	IA32_EXCEPTION_VECTOR_SEGMENT_NOT_PRESENT,
	IA32_EXCEPTION_VECTOR_STACK_SEGMENT_FAULT,
	IA32_EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT,
	IA32_EXCEPTION_VECTOR_PAGE_FAULT,
	IA32_EXCEPTION_VECTOR_RESERVED_0X0F,
	IA32_EXCEPTION_VECTOR_MATH_FAULT,
	IA32_EXCEPTION_VECTOR_ALIGNMENT_CHECK,
	IA32_EXCEPTION_VECTOR_MACHINE_CHECK,
	IA32_EXCEPTION_VECTOR_SIMD_FLOATING_POINT_NUMERIC_ERROR,
	IA32_EXCEPTION_VECTOR_RESERVED_SIMD_FLOATING_POINT_NUMEIC_ERROR,
	IA32_EXCEPTION_VECTOR_RESERVED_0X14,
	IA32_EXCEPTION_VECTOR_RESERVED_0X15,
	IA32_EXCEPTION_VECTOR_RESERVED_0X16,
	IA32_EXCEPTION_VECTOR_RESERVED_0X17,
	IA32_EXCEPTION_VECTOR_RESERVED_0X18,
	IA32_EXCEPTION_VECTOR_RESERVED_0X19,
	IA32_EXCEPTION_VECTOR_RESERVED_0X1A,
	IA32_EXCEPTION_VECTOR_RESERVED_0X1B,
	IA32_EXCEPTION_VECTOR_RESERVED_0X1C,
	IA32_EXCEPTION_VECTOR_RESERVED_0X1D,
	IA32_EXCEPTION_VECTOR_RESERVED_0X1E,
	IA32_EXCEPTION_VECTOR_RESERVED_0X1F
} ia32_exception_vectors_t;

/* IA-32 Interrupt Descriptor Table - Gate Descriptor */
typedef struct {
	uint32_t offset_low:16;  /* Offset bits 15..0 */
	uint32_t selector:16;    /* Selector */
	uint32_t reserved_0:8;   /* reserved */
	uint32_t gate_type:8;    /* Gate Type.  See #defines above */
	uint32_t offset_high:16; /* Offset bits 31..16 */
} ia32_idt_gate_descriptor_t;

/* Descriptor for the Global Descriptor Table(GDT) and Interrupt Descriptor
 * Table(IDT) */
typedef struct {
	uint16_t limit;
	uint32_t base;
} ia32_descriptor_t;

/* Ring 0 Interrupt Descriptor Table - Gate Types */
#define IA32_IDT_GATE_TYPE_TASK          0x85
#define IA32_IDT_GATE_TYPE_INTERRUPT_16  0x86
#define IA32_IDT_GATE_TYPE_TRAP_16       0x87
#define IA32_IDT_GATE_TYPE_INTERRUPT_32  0x8E
#define IA32_IDT_GATE_TYPE_TRAP_32       0x8F

#define IDT_VECTOR_COUNT 256
#define XMON_CS_SELECTOR 0x10

#define ERROR_CODE_EXT_BIT 0x1
#define ERROR_CODE_IN_IDT  0x2
#define ERROR_CODE_TI      0x4

ia32_idt_gate_descriptor_t xmon_idt[IDT_VECTOR_COUNT];
ia32_descriptor_t idt_descriptor;

void print_exception_header(uint32_t cs, uint32_t eip)
{
	/* clear_screen(); */
	PRINT_STRING
		("****************************************************************\n");
	PRINT_STRING
		("*                                                              *\n");
	PRINT_STRING
		("*       Intel Lightweight Virtual Machine Monitor (TM)         *\n");
	PRINT_STRING
		("*                                                              *\n");
	PRINT_STRING
		("****************************************************************\n");
	PRINT_STRING("\nFatal error has occured at 0x");
	PRINT_VALUE(cs);
	PRINT_STRING(":0x");
	PRINT_VALUE(eip);
	PRINT_STRING("\n");

	PRINT_STRING("Error type: ");
}

void print_error_code_generic(uint32_t error_code)
{
	PRINT_STRING("Error code: 0x");
	PRINT_VALUE(error_code);
	PRINT_STRING(", index 0x");
	PRINT_VALUE(error_code >> 3);
	PRINT_STRING("\n");

	if ((error_code & ERROR_CODE_EXT_BIT) != 0) {
		PRINT_STRING("External event\n");
	} else {
		PRINT_STRING("Internal event\n");
	}

	if ((error_code & ERROR_CODE_IN_IDT) != 0) {
		PRINT_STRING("index is in IDT\n");
	} else if ((error_code & ERROR_CODE_TI) != 0) {
		PRINT_STRING("index is in LDT\n");
	} else {
		PRINT_STRING("index is in GDT\n");
	}
}

void exception_handler_reserved(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("reserved exception\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_divide_error(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Divide error\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_debug_break_point(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Debug breakpoint\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_nmi(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("nmi\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_break_point(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Breakpoint\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_overflow(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Overflow\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_bound_range_exceeded(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Bound range exceeded\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_undefined_opcode(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Undefined opcode\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_no_math_coprocessor(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("No math coprocessor\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_double_fault(uint32_t cs, uint32_t eip, uint32_t error_code)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Double fault\n");

	/* No need to print error code here because it is always zero */

	MON_UP_BREAKPOINT();
}

void exception_handler_invalid_task_segment_selector(uint32_t cs, uint32_t eip,
						     uint32_t error_code)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Invalid task segment selector\n");
	print_error_code_generic(error_code);
	MON_UP_BREAKPOINT();
}

void exception_handler_segment_not_present(uint32_t cs,
					   uint32_t eip,
					   uint32_t error_code)
{
	print_exception_header(cs, eip);
	PRINT_STRING("segment not present\n");
	print_error_code_generic(error_code);
	MON_UP_BREAKPOINT();
}

void exception_handler_stack_segment_fault(uint32_t cs,
					   uint32_t eip,
					   uint32_t error_code)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Stack segment fault\n");
	print_error_code_generic(error_code);
	MON_UP_BREAKPOINT();
}

void exception_handler_general_protection_fault(uint32_t cs, uint32_t eip,
						uint32_t error_code)
{
	print_exception_header(cs, eip);
	PRINT_STRING("General protection fault\n");
	print_error_code_generic(error_code);
	MON_UP_BREAKPOINT();
}

void exception_handler_page_fault(uint32_t cs, uint32_t eip, uint32_t error_code)
{
	uint32_t cr2_val;

	__asm__ __volatile__ (
		"mov %%cr2, %%rax\n"
		"mov %%eax, %0\n"
		: "=m" (cr2_val)
		:
		: "%eax", "memory"
		);

	print_exception_header(cs, eip);
	PRINT_STRING("Page fault\n");
	PRINT_STRING("Faulting address 0x");
	PRINT_VALUE(cr2_val);
	PRINT_STRING("\n");

	/* TODO: need a specific error code print function here */
	MON_UP_BREAKPOINT();
}

void exception_handler_math_fault(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Math fault\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_alignment_check(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Alignment check\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_machine_check(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("Machine check\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_simd_floating_point_numeric_error(uint32_t cs, uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("SIMD floating point numeric error\n");
	MON_UP_BREAKPOINT();
}

void exception_handler_reserved_simd_floating_point_numeric_error(uint32_t cs,
								  uint32_t eip)
{
	print_exception_header(cs, eip);
	PRINT_STRING("reserved SIMD floating point numeric error\n");
	MON_UP_BREAKPOINT();
}

void install_exception_handler(uint32_t exception_index, uint64_t handler_addr)
{
	xmon_idt[exception_index].offset_low = handler_addr & 0xFFFF;
	xmon_idt[exception_index].offset_high = handler_addr >> 16;
}

void setup_idt()
{
	int i;

	uint32_t p_idt_descriptor;

	PRINT_STRING("SetupIdt called\n");

	zero_mem(&xmon_idt, sizeof(xmon_idt));

	for (i = 0; i < 32; i++) {
		xmon_idt[i].gate_type = IA32_IDT_GATE_TYPE_INTERRUPT_32;
		xmon_idt[i].selector = XMON_CS_SELECTOR;
		xmon_idt[i].reserved_0 = 0;
		install_exception_handler(i, (uint64_t)(exception_handler_reserved));
	}

	install_exception_handler(IA32_EXCEPTION_VECTOR_DIVIDE_ERROR,
		(uint64_t)exception_handler_divide_error);
	install_exception_handler(IA32_EXCEPTION_VECTOR_DEBUG_BREAK_POINT,
		(uint64_t)exception_handler_debug_break_point);
	install_exception_handler(IA32_EXCEPTION_VECTOR_NMI,
		(uint64_t)exception_handler_nmi);
	install_exception_handler(IA32_EXCEPTION_VECTOR_BREAK_POINT,
		(uint64_t)exception_handler_break_point);
	install_exception_handler(IA32_EXCEPTION_VECTOR_OVERFLOW,
		(uint64_t)exception_handler_overflow);
	install_exception_handler(IA32_EXCEPTION_VECTOR_BOUND_RANGE_EXCEEDED,
		(uint64_t)exception_handler_bound_range_exceeded);
	install_exception_handler(IA32_EXCEPTION_VECTOR_UNDEFINED_OPCODE,
		(uint64_t)exception_handler_undefined_opcode);
	install_exception_handler(IA32_EXCEPTION_VECTOR_NO_MATH_COPROCESSOR,
		(uint64_t)exception_handler_no_math_coprocessor);
	install_exception_handler(IA32_EXCEPTION_VECTOR_DOUBLE_FAULT,
		(uint64_t)exception_handler_double_fault);
	install_exception_handler(
		IA32_EXCEPTION_VECTOR_INVALID_TASK_SEGMENT_SELECTOR,
		(uint64_t)
		exception_handler_invalid_task_segment_selector);
	install_exception_handler(IA32_EXCEPTION_VECTOR_SEGMENT_NOT_PRESENT,
		(uint64_t)exception_handler_segment_not_present);
	install_exception_handler(IA32_EXCEPTION_VECTOR_STACK_SEGMENT_FAULT,
		(uint64_t)exception_handler_stack_segment_fault);
	install_exception_handler(IA32_EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT,
		(uint64_t)exception_handler_general_protection_fault);
	install_exception_handler(IA32_EXCEPTION_VECTOR_PAGE_FAULT,
		(uint64_t)exception_handler_page_fault);
	install_exception_handler(IA32_EXCEPTION_VECTOR_MATH_FAULT,
		(uint64_t)exception_handler_math_fault);
	install_exception_handler(IA32_EXCEPTION_VECTOR_ALIGNMENT_CHECK,
		(uint64_t)exception_handler_alignment_check);
	install_exception_handler(IA32_EXCEPTION_VECTOR_MACHINE_CHECK,
		(uint64_t)exception_handler_machine_check);
	install_exception_handler(
		IA32_EXCEPTION_VECTOR_SIMD_FLOATING_POINT_NUMERIC_ERROR,
		(uint64_t)
		exception_handler_simd_floating_point_numeric_error);
	install_exception_handler
		(IA32_EXCEPTION_VECTOR_RESERVED_SIMD_FLOATING_POINT_NUMEIC_ERROR,
		(uint64_t)exception_handler_reserved_simd_floating_point_numeric_error);

	idt_descriptor.base = (uint32_t)(uint64_t)(xmon_idt);
	idt_descriptor.limit = sizeof(ia32_idt_gate_descriptor_t) * 32 - 1;

	p_idt_descriptor = (uint32_t)(uint64_t)&idt_descriptor;
	__asm__ __volatile__ (
		"mov   %0, %%edx\n"
		"lidt  (%%edx)\n"
		:
		: "g" (p_idt_descriptor)
		: "edx"
		);
}
