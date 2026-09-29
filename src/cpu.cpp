#include "cpu.h"
#include "cpu_instructions.h"
#include "cpu_routines.h"
#include "memory_bus.h"
#include "emulator_core.h"
#include "timer.h"
#include "interrupts.h"
#include "debug_log.h"
#include <cart.h>
#include <cstdio>

gb_cpu_registers cpu_registers;
uint8_t cpu_current_op_code = 0;
uint32_t cpu_instruction_counter = 0;
cpu_execute_op cpu_current_instruction_execute = nullptr;
uint8_t cpu_halt_count = 0; // 0 == not halted, 1 == halt instruction, 2 == stop instruction
bool cpu_halt_bug = false;
bool cpu_debug_instructions = false;

void cpu_reset()
{
    printf("ROM direct 0x0100: %02X\n", cartridge_data[0x0100]);
	printf("ROM bus   0x0100: %02X\n", memory_bus_read(0x0100));

	// After executing boot rom registers should have these values
	cpu_registers.af = 0x01B0;
	cpu_registers.bc = 0x0013;
	cpu_registers.de = 0x00D8;
	cpu_registers.hl = 0x014D;
	cpu_registers.sp = 0xFFFE;
	cpu_registers.pc = 0x0100;
}

void cpu_tick()
{
	/*
	 * EI enables interrupts after the instruction following EI.
	 * Update IME at the start of the tick so the instruction following EI executes with IME = true.
	 */
	if (interrupt_enable_ime_delay > 0)
	{
		interrupt_enable_ime_delay--;

		if (interrupt_enable_ime_delay == 0)
		{
			interrupt_master_enable = true;
		}
	}

	if (cpu_halt_count == 0)
	{
		cpu_fetch();
		cpu_execute();
		cpu_instruction_counter++;
	}
	else
	{
		core_advance_cpu_clocks(4); // Halted, waiting for an interrupt to trigger
	}

	if (cpu_instruction_counter > 10000 && cpu_debug_instructions)
	{
		cpu_debug_instructions = false;
		debug_log_close_file();
	}

	interrupt_service_routine();
}

void cpu_fetch()
{
	if (cpu_debug_instructions)
	{
		cpu_dump_registers(cpu_registers);
	}

	cpu_current_op_code = memory_bus_read(cpu_registers.pc++);
	const bool is_extended_cb_instruction = cpu_current_op_code == 0xCB;

	if (cpu_halt_bug)
	{
		cpu_registers.pc--; // Repeat one byte during halt bug
		cpu_halt_bug = false;
	}

	if (is_extended_cb_instruction)
	{
		core_advance_cpu_clocks(4);
		const uint8_t cpu_current_op_code_cb = memory_bus_read(cpu_registers.pc++);
		const gb_cpu_pre_cb_instruction& cb_instruction = cb_instructions[cpu_current_op_code_cb];
		cpu_current_instruction_execute = cb_instruction.execute;
	}
	else
	{
		const gb_cpu_instruction& instruction = instructions[cpu_current_op_code];
		cpu_current_instruction_execute = instruction.execute;
	}
}

bool cpu_execute()
{
	if (!cpu_current_instruction_execute)
	{
		const gb_cpu_instruction& instruction = instructions[cpu_current_op_code];
		const uint8_t pchi = ((cpu_registers.pc - 1) & 0xFF00) >> 8;
		const uint8_t pclo = ((cpu_registers.pc - 1) & 0xFF);
		debug_log("Unknown instruction %.2X at: %.2X%.2X (%s), count %i\n", cpu_current_op_code, pchi, pclo, instruction.disassembly, cpu_instruction_counter);
		return false;
	}
	// This actually executes the instruction
	cpu_current_instruction_execute();
	return true;
}

void cpu_dump_registers(const gb_cpu_registers& registers)
{
	const uint8_t op_code = memory_bus_read(registers.pc);
	const gb_cpu_instruction& instruction = instructions[op_code];
	const uint8_t pchi = (registers.pc & 0xFF00) >> 8;
	const uint8_t pclo = (registers.pc & 0xFF);
	const uint8_t sphi = (registers.sp & 0xFF00) >> 8;
	const uint8_t splo = (registers.sp & 0xFF);

	if (instruction.operand_length == 0)
	{
		debug_log("AF: %.2X%.2X  BC: %.2X%.2X  DE: %.2X%.2X  HL: %.2X%.2X  SP: %.2X%.2X  PC: %.2X%.2X %s\n",
			registers.a, registers.f, registers.b, registers.c, registers.d, registers.e, registers.h, registers.l, sphi, splo, pchi, pclo, instruction.disassembly);
	}
	else if (instruction.operand_length == 1)
	{
		const uint8_t operand = memory_bus_read(registers.pc + 1);
		debug_log("AF: %.2X%.2X  BC: %.2X%.2X  DE: %.2X%.2X  HL: %.2X%.2X  SP: %.2X%.2X  PC: %.2X%.2X %s (%.2X)\n",
			registers.a, registers.f, registers.b, registers.c, registers.d, registers.e, registers.h, registers.l, sphi, splo, pchi, pclo, instruction.disassembly, operand);
	}
	else if (instruction.operand_length == 2)
	{
		const uint8_t oplo = memory_bus_read(registers.pc + 1);
		const uint8_t ophi = memory_bus_read(registers.pc + 2);
		debug_log("AF: %.2X%.2X  BC: %.2X%.2X  DE: %.2X%.2X  HL: %.2X%.2X  SP: %.2X%.2X  PC: %.2X%.2X %s (%.2X%.2X)\n",
			registers.a, registers.f, registers.b, registers.c, registers.d, registers.e, registers.h, registers.l, sphi, splo, pchi, pclo, instruction.disassembly, ophi, oplo);
	}
}

void cpu_noop() // 0x00
{
	core_advance_cpu_clocks(4);
}

void cpu_ld_bc_nn() // 0x01
{
	cpu_routine_ld_16(cpu_registers.b, cpu_registers.c);
}

void cpu_ld_bc_a() // 0x02
{
	cpu_routine_ld_ptr8(cpu_registers.bc, cpu_registers.a);
}

void cpu_inc_bc() // 0x03
{
	cpu_routine_inc_16(cpu_registers.bc);
}

void cpu_inc_b() // 0x04
{
	cpu_routine_inc_8(cpu_registers.b);
}

void cpu_dec_b() // 0x05
{
	cpu_routine_dec_8(cpu_registers.b);
}

void cpu_ld_b_n() // 0x06
{
	cpu_routine_ld_8(cpu_registers.b);
}

void cpu_rlca()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_ZERO(0);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY((cpu_registers.a & 0x80) != 0);
	cpu_registers.a = (cpu_registers.a << 1) | GET_FLAG_CARRY;
}

void cpu_ld_nn_sp()
{
	core_advance_cpu_clocks(4);
	uint16_t temp = memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	temp |= ((uint16_t)memory_bus_read(cpu_registers.pc++)) << 8;
	core_advance_cpu_clocks(4);
	memory_bus_write(temp++, (cpu_registers.sp & 0xFF));
	core_advance_cpu_clocks(4);
	memory_bus_write(temp, ((cpu_registers.sp & 0xFF00) >> 8));
	core_advance_cpu_clocks(4);
}

void cpu_add_hl_bc() // 0x09
{
	cpu_routine_add_hl_16(cpu_registers.bc);
}

void cpu_ld_a_bc() // 0x0A
{
	cpu_routine_ld_ptr_16(cpu_registers.a, cpu_registers.bc);
}

void cpu_dec_bc() // 0x0B
{
	cpu_routine_dec_16(cpu_registers.bc);
}

void cpu_inc_c() // 0x0C
{
	cpu_routine_inc_8(cpu_registers.c);
}

void cpu_dec_c() // 0x0D
{
	cpu_routine_dec_8(cpu_registers.c);
}

void cpu_ld_c_n() // 0x0E
{
	cpu_routine_ld_8(cpu_registers.c);
}

void cpu_rrca()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_ZERO(0);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY((cpu_registers.a & 0x01) != 0);
	cpu_registers.a = (cpu_registers.a >> 1) | (GET_FLAG_CARRY << 7);
}

void cpu_stop()
{
	core_advance_cpu_clocks(4);
	if (memory_bus_read(cpu_registers.pc++) != 0)
	{
		debug_log("CPU - Corrupted STOP at PC: %04X, should have operand 0x00\n", cpu_registers.pc);
	}
	core_advance_cpu_clocks(4);
	timer_on_div_write(0);
	cpu_halt_count = 2;
}

void cpu_ld_de_nn() // 0x11
{
	cpu_routine_ld_16(cpu_registers.d, cpu_registers.e);
}

void cpu_ld_de_a() // 0x12
{
	cpu_routine_ld_ptr8(cpu_registers.de, cpu_registers.a);
}

void cpu_inc_de() // 0x13
{
	cpu_routine_inc_16(cpu_registers.de);
}

void cpu_inc_d() // 0x14
{
	cpu_routine_inc_8(cpu_registers.d);
}

void cpu_dec_d() // 0x15
{
	cpu_routine_dec_8(cpu_registers.d);
}

void cpu_ld_d_n() // 0x16
{
	cpu_routine_ld_8(cpu_registers.d);
}

void cpu_rla()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_ZERO(0);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	uint8_t temp = GET_FLAG_CARRY;
	SET_FLAG_CARRY((cpu_registers.a & 0x80) != 0);
	cpu_registers.a = (cpu_registers.a << 1) | temp;
}

void cpu_jr_n()
{
	core_advance_cpu_clocks(4);
	uint8_t temp = memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	cpu_registers.pc = (cpu_registers.pc + (int8_t)temp) & 0xFFFF;
	core_advance_cpu_clocks(4);
}

void cpu_add_hl_de() // 0x19
{
	cpu_routine_add_hl_16(cpu_registers.de);
}

void cpu_ld_a_de() // 0x1A
{
	cpu_routine_ld_ptr_16(cpu_registers.a, cpu_registers.de);
}

void cpu_dec_de() // 0x1B
{
	cpu_routine_dec_16(cpu_registers.de);
}

void cpu_inc_e() // 0x1C
{
	cpu_routine_inc_8(cpu_registers.e);
}

void cpu_dec_e() // 0x1D
{
	cpu_routine_dec_8(cpu_registers.e);
}

void cpu_ld_e_n() // 0x1E
{
	cpu_routine_ld_8(cpu_registers.e);
}

void cpu_rra()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_ZERO(0);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	uint8_t temp = GET_FLAG_CARRY;
	SET_FLAG_CARRY((cpu_registers.a & 0x01));
	cpu_registers.a = (cpu_registers.a >> 1) | (temp << 7);
}

void cpu_jr_nz_n() // 0x20
{
	cpu_routine_jr_conditional_n(GET_FLAG_ZERO == 0);
}

void cpu_ld_hl_nn() // 0x21
{
	cpu_routine_ld_16(cpu_registers.h, cpu_registers.l);
}

void cpu_ldi_hl_a()
{
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, cpu_registers.a);
	core_advance_cpu_clocks(4);
	cpu_registers.hl = (cpu_registers.hl + 1) & 0xFFFF;
}

void cpu_inc_hl() // 0x23
{
	cpu_routine_inc_16(cpu_registers.hl);
}

void cpu_inc_h() // 0x24
{
	cpu_routine_inc_8(cpu_registers.h);
}

void cpu_dec_h() // 0x25
{
	cpu_routine_dec_8(cpu_registers.h);
}

void cpu_ld_h_n() // 0x26
{
	cpu_routine_ld_8(cpu_registers.h);
}

void cpu_daa()
{
	core_advance_cpu_clocks(4);
	if (!GET_FLAG_SUBTRACT)
	{
		// after an addition, adjust if (half-)carry occurred of if result is out of bounds
		if (GET_FLAG_CARRY || cpu_registers.a > 0x99)
		{
			cpu_registers.a += 0x60;
			SET_FLAG_CARRY(1);
		}
		if (GET_FLAG_HALF_CARRY || (cpu_registers.a & 0x0F) > 0x09)
		{
			cpu_registers.a += 0x6;
		}
	}
	else
	{
		// after a subtraction, only adjust if (half-)carry occurred
		if (GET_FLAG_CARRY)
		{
			cpu_registers.a -= 0x60;
		}
		if (GET_FLAG_HALF_CARRY)
		{
			cpu_registers.a -= 0x6;
		}
	}
	SET_FLAG_ZERO(cpu_registers.a == 0);
	SET_FLAG_HALF_CARRY(0);
}

void cpu_jr_z_n() // 0x28
{
	cpu_routine_jr_conditional_n(GET_FLAG_ZERO);
}

void cpu_add_hl_hl()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY((cpu_registers.hl & 0x8000) != 0);
	SET_FLAG_HALF_CARRY((cpu_registers.hl & 0x0800) != 0);
	core_advance_cpu_clocks(4);
	cpu_registers.hl = (cpu_registers.hl << 1) & 0xFFFF;
}

void cpu_ldi_a_hl()
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	cpu_registers.hl = (cpu_registers.hl + 1) & 0xFFFF;
}

void cpu_dec_hl() // 0x2B
{
	cpu_routine_dec_16(cpu_registers.hl);
}

void cpu_inc_l() // 0x2C
{
	cpu_routine_inc_8(cpu_registers.l);
}

void cpu_dec_l() // 0x2D
{
	cpu_routine_dec_8(cpu_registers.l);
}

void cpu_ld_l_n() // 0x2E
{
	cpu_routine_ld_8(cpu_registers.l);
}

void cpu_cpl()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_HALF_CARRY(1);
	cpu_registers.a = ~cpu_registers.a;
}

void cpu_jr_nc_n() // 0x30
{
	cpu_routine_jr_conditional_n(GET_FLAG_CARRY == 0);
}

void cpu_ld_sp_nn() // 0x31
{
	cpu_routine_ld_16(cpu_registers.s, cpu_registers.p);
}

void cpu_ldd_hl_a() // 0x32
{
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, cpu_registers.a);
	cpu_registers.hl = (cpu_registers.hl - 1) & 0xFFFF;
	core_advance_cpu_clocks(4);
}

void cpu_inc_sp() // 0x33
{
	cpu_routine_inc_16(cpu_registers.sp);
}

void cpu_inc__hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY((temp & 0xF) == 0xF);
	temp = (temp + 1) & 0xFF;
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_dec__hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_HALF_CARRY((temp & 0xF) == 0);
	temp = (temp - 1) & 0xFF;
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_ld_hl_n()
{
	core_advance_cpu_clocks(4);
	uint8_t temp = memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
	core_advance_cpu_clocks(4);
}

void cpu_scf()
{
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY(1);
	core_advance_cpu_clocks(4);
}

void cpu_jr_c_n() // 0x38
{
	cpu_routine_jr_conditional_n(GET_FLAG_CARRY);
}

void cpu_add_hl_sp() // 0x39
{
	cpu_routine_add_hl_16(cpu_registers.sp);
}

void cpu_ldd_a_hl()
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	cpu_registers.hl = (cpu_registers.hl - 1) & 0xFFFF;
}

void cpu_dec_sp() // 0x3B
{
	cpu_routine_dec_16(cpu_registers.sp);
}

void cpu_inc_a() // 0x3C
{
	cpu_routine_inc_8(cpu_registers.a);
}

void cpu_dec_a() // 0x3D
{
	cpu_routine_dec_8(cpu_registers.a);
}

void cpu_ld_a_n() // 0x3E
{
	cpu_routine_ld_8(cpu_registers.a);
}

void cpu_ccf()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_CARRY(!GET_FLAG_CARRY);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
}

void cpu_ld_b_b() // 0x40
{
	core_advance_cpu_clocks(4);
}

void cpu_ld_b_c() // 0x41
{
	core_advance_cpu_clocks(4);
	cpu_registers.b = cpu_registers.c;
}

void cpu_ld_b_d() // 0x42
{
	core_advance_cpu_clocks(4);
	cpu_registers.b = cpu_registers.d;
}

void cpu_ld_b_e() // 0x43
{
	core_advance_cpu_clocks(4);
	cpu_registers.b = cpu_registers.e;
}

void cpu_ld_b_h() // 0x44
{
	core_advance_cpu_clocks(4);
	cpu_registers.b = cpu_registers.h;
}

void cpu_ld_b_l() // 0x45
{
	core_advance_cpu_clocks(4);
	cpu_registers.b = cpu_registers.l;
}

void cpu_ld_b_hl() // 0x46
{
	cpu_routine_ld_ptr_16(cpu_registers.b, cpu_registers.hl);
}

void cpu_ld_b_a() // 0x47
{
	core_advance_cpu_clocks(4);
	cpu_registers.b = cpu_registers.a;
}

void cpu_ld_c_b() // 0x48
{
	core_advance_cpu_clocks(4);
	cpu_registers.c = cpu_registers.b;
}

void cpu_ld_c_c() // 0x49
{
	core_advance_cpu_clocks(4);
}

void cpu_ld_c_d() // 0x4A
{
	core_advance_cpu_clocks(4);
	cpu_registers.c = cpu_registers.d;
}

void cpu_ld_c_e() // 0x4B
{
	core_advance_cpu_clocks(4);
	cpu_registers.c = cpu_registers.e;
}

void cpu_ld_c_h() // 0x4C
{
	core_advance_cpu_clocks(4);
	cpu_registers.c = cpu_registers.h;
}

void cpu_ld_c_l() // 0x4D
{
	core_advance_cpu_clocks(4);
	cpu_registers.c = cpu_registers.l;
}

void cpu_ld_c_hl() // 0x4E
{
	cpu_routine_ld_ptr_16(cpu_registers.c, cpu_registers.hl);
}

void cpu_ld_c_a() // 0x4F
{
	core_advance_cpu_clocks(4);
	cpu_registers.c = cpu_registers.a;
}

void cpu_ld_d_b() // 0x50
{
	core_advance_cpu_clocks(4);
	cpu_registers.d = cpu_registers.b;
}

void cpu_ld_d_c() // 0x51
{
	core_advance_cpu_clocks(4);
	cpu_registers.d = cpu_registers.c;
}

void cpu_ld_d_d() // 0x52
{
	core_advance_cpu_clocks(4);
}

void cpu_ld_d_e() // 0x53
{
	core_advance_cpu_clocks(4);
	cpu_registers.d = cpu_registers.e;
}

void cpu_ld_d_h() // 0x54
{
	core_advance_cpu_clocks(4);
	cpu_registers.d = cpu_registers.h;
}

void cpu_ld_d_l() // 0x55
{
	core_advance_cpu_clocks(4);
	cpu_registers.d = cpu_registers.l;
}

void cpu_ld_d_hl() // 0x56
{
	cpu_routine_ld_ptr_16(cpu_registers.d, cpu_registers.hl);
}

void cpu_ld_d_a() // 0x57
{
	core_advance_cpu_clocks(4);
	cpu_registers.d = cpu_registers.a;
}

void cpu_ld_e_b() // 0x58
{
	core_advance_cpu_clocks(4);
	cpu_registers.e = cpu_registers.b;
}

void cpu_ld_e_c() // 0x59
{
	core_advance_cpu_clocks(4);
	cpu_registers.e = cpu_registers.c;
}

void cpu_ld_e_d() // 0x5A
{
	core_advance_cpu_clocks(4);
	cpu_registers.e = cpu_registers.d;
}

void cpu_ld_e_e() // 0x5B
{
	core_advance_cpu_clocks(4);
}

void cpu_ld_e_h() // 0x5C
{
	core_advance_cpu_clocks(4);
	cpu_registers.e = cpu_registers.h;
}

void cpu_ld_e_l() // 0x5D
{
	core_advance_cpu_clocks(4);
	cpu_registers.e = cpu_registers.l;
}

void cpu_ld_e_hl() // 0x5E
{
	cpu_routine_ld_ptr_16(cpu_registers.e, cpu_registers.hl);
}

void cpu_ld_e_a() // 0x5F
{
	core_advance_cpu_clocks(4);
	cpu_registers.e = cpu_registers.a;
}

void cpu_ld_h_b() // 0x60
{
	core_advance_cpu_clocks(4);
	cpu_registers.h = cpu_registers.b;
}

void cpu_ld_h_c() // 0x61
{
	core_advance_cpu_clocks(4);
	cpu_registers.h = cpu_registers.c;
}

void cpu_ld_h_d() // 0x62
{
	core_advance_cpu_clocks(4);
	cpu_registers.h = cpu_registers.d;
}

void cpu_ld_h_e() // 0x63
{
	core_advance_cpu_clocks(4);
	cpu_registers.h = cpu_registers.e;
}

void cpu_ld_h_h() // 0x64
{
	core_advance_cpu_clocks(4);
}

void cpu_ld_h_l() // 0x65
{
	core_advance_cpu_clocks(4);
	cpu_registers.h = cpu_registers.l;
}

void cpu_ld_h_hl() // 0x66
{
	cpu_routine_ld_ptr_16(cpu_registers.h, cpu_registers.hl);
}

void cpu_ld_h_a() // 0x67
{
	core_advance_cpu_clocks(4);
	cpu_registers.h = cpu_registers.a;
}

void cpu_ld_l_b() // 0x68
{
	core_advance_cpu_clocks(4);
	cpu_registers.l = cpu_registers.b;
}

void cpu_ld_l_c() // 0x69
{
	core_advance_cpu_clocks(4);
	cpu_registers.l = cpu_registers.c;
}

void cpu_ld_l_d() // 0x6A
{
	core_advance_cpu_clocks(4);
	cpu_registers.l = cpu_registers.d;
}

void cpu_ld_l_e() // 0x6B
{
	core_advance_cpu_clocks(4);
	cpu_registers.l = cpu_registers.e;
}

void cpu_ld_l_h() // 0x6C
{
	core_advance_cpu_clocks(4);
	cpu_registers.l = cpu_registers.h;
}

void cpu_ld_l_l() // 0x6D
{
	core_advance_cpu_clocks(4);
}

void cpu_ld_l_hl() // 0x6E
{
	cpu_routine_ld_ptr_16(cpu_registers.l, cpu_registers.hl);
}

void cpu_ld_l_a() // 0x6F
{
	core_advance_cpu_clocks(4);
	cpu_registers.l = cpu_registers.a;
}

void cpu_ld_hl_b() // 0x70
{
	cpu_routine_ld_ptr8(cpu_registers.hl, cpu_registers.b);
}

void cpu_ld_hl_c() // 0x71
{
	cpu_routine_ld_ptr8(cpu_registers.hl, cpu_registers.c);
}

void cpu_ld_hl_d() // 0x72
{
	cpu_routine_ld_ptr8(cpu_registers.hl, cpu_registers.d);
}

void cpu_ld_hl_e() // 0x73
{
	cpu_routine_ld_ptr8(cpu_registers.hl, cpu_registers.e);
}

void cpu_ld_hl_h() // 0x74
{
	cpu_routine_ld_ptr8(cpu_registers.hl, cpu_registers.h);
}

void cpu_ld_hl_l() // 0x75
{
	cpu_routine_ld_ptr8(cpu_registers.hl, cpu_registers.l);
}

void cpu_halt()
{
	core_advance_cpu_clocks(4);
	const uint8_t interrupt_enable = memory_bus_read(ADDR_IO_IE);
	const uint8_t interrupt_flag = memory_bus_read(ADDR_IO_IF);
	const bool interrupt_pending = ((interrupt_enable & interrupt_flag) & 0x1F) != 0;
	if (!interrupt_master_enable && interrupt_pending != 0)
	{
		cpu_halt_bug = true;
	}
	else
	{
		cpu_halt_count = 1;
	}
}

void cpu_ld_hl_a() // 0x77
{
	cpu_routine_ld_ptr8(cpu_registers.hl, cpu_registers.a);
}

void cpu_ld_a_b() // 0x78
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = cpu_registers.b;
}

void cpu_ld_a_c() // 0x79
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = cpu_registers.c;
}

void cpu_ld_a_d() // 0x7A
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = cpu_registers.d;
}

void cpu_ld_a_e() // 0x7B
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = cpu_registers.e;
}

void cpu_ld_a_h() // 0x7C
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = cpu_registers.h;
}

void cpu_ld_a_l() // 0x7D
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = cpu_registers.l;
}

void cpu_ld_a_hl() // 0x7E
{
	cpu_routine_ld_ptr_16(cpu_registers.a, cpu_registers.hl);
}

void cpu_ld_a_a() // 0x7F
{
	core_advance_cpu_clocks(4);
}

void cpu_add_a_b() // 0x80
{
	cpu_routine_add_a_8(cpu_registers.b);
}

void cpu_add_a_c() // 0x81
{
	cpu_routine_add_a_8(cpu_registers.c);
}

void cpu_add_a_d() // 0x82
{
	cpu_routine_add_a_8(cpu_registers.d);
}

void cpu_add_a_e() // 0x83
{
	cpu_routine_add_a_8(cpu_registers.e);
}

void cpu_add_a_h() // 0x84
{
	cpu_routine_add_a_8(cpu_registers.h);
}

void cpu_add_a_l() // 0x85
{
	cpu_routine_add_a_8(cpu_registers.l);
}

void cpu_add_a_hl()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	uint32_t temp_a = cpu_registers.a;
	uint32_t temp_hl = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_HALF_CARRY(((temp_a & 0xF) + (temp_hl & 0xF)) > 0xF);
	cpu_registers.a += temp_hl;
	SET_FLAG_ZERO(cpu_registers.a == 0);
	SET_FLAG_CARRY(temp_a > cpu_registers.a);
}

void cpu_add_a_a()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY((cpu_registers.a & 0x08) != 0);
    SET_FLAG_CARRY((cpu_registers.a & 0x80) != 0);
	cpu_registers.a += cpu_registers.a;
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_adc_a_b() // 0x88
{
	cpu_routine_adc_a_8(cpu_registers.b);
}

void cpu_adc_a_c() // 0x89
{
	cpu_routine_adc_a_8(cpu_registers.c);
}

void cpu_adc_a_d() // 0x8A
{
	cpu_routine_adc_a_8(cpu_registers.d);
}

void cpu_adc_a_e() // 0x8B
{
	cpu_routine_adc_a_8(cpu_registers.e);
}

void cpu_adc_a_h() // 0x8C
{
	cpu_routine_adc_a_8(cpu_registers.h);
}

void cpu_adc_a_l() // 0x8D
{
	cpu_routine_adc_a_8(cpu_registers.l);
}

void cpu_adc_a_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp_hl = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	uint32_t temp_a = cpu_registers.a + temp_hl + GET_FLAG_CARRY;
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY((((cpu_registers.a & 0xF) + (temp_hl & 0xF)) + GET_FLAG_CARRY) > 0xF);
	SET_FLAG_CARRY(temp_a > 0xFF);
	temp_a &= 0xFF;
	cpu_registers.a = temp_a;
	SET_FLAG_ZERO(temp_a == 0);
}

void cpu_adc_a_a()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	uint32_t temp = (((uint32_t)cpu_registers.a) << 1) + GET_FLAG_CARRY;
	SET_FLAG_HALF_CARRY((cpu_registers.a & 0x08) != 0);
	SET_FLAG_CARRY(temp > 0xFF);
	temp &= 0xFF;
	cpu_registers.a = temp;
	SET_FLAG_ZERO(temp == 0);
}

void cpu_sub_a_b() // 0x90
{
	cpu_routine_sub_a_8(cpu_registers.b);
}

void cpu_sub_a_c() // 0x91
{
	cpu_routine_sub_a_8(cpu_registers.c);
}

void cpu_sub_a_d() // 0x92
{
	cpu_routine_sub_a_8(cpu_registers.d);
}

void cpu_sub_a_e() // 0x93
{
	cpu_routine_sub_a_8(cpu_registers.e);
}

void cpu_sub_a_h() // 0x94
{
	cpu_routine_sub_a_8(cpu_registers.h);
}

void cpu_sub_a_l() // 0x95
{
	cpu_routine_sub_a_8(cpu_registers.l);
}

void cpu_sub_a_hl()
{
	core_advance_cpu_clocks(4);
	uint8_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_HALF_CARRY((cpu_registers.a & 0xF) < (temp & 0xF));
	SET_FLAG_CARRY(cpu_registers.a < temp);
	cpu_registers.a -= temp;
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_sub_a_a()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_ZERO(1);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY(0);
	cpu_registers.a = 0;
}

void cpu_sbc_a_b() // 0x98
{
	cpu_routine_sbc_a_8(cpu_registers.b);
}

void cpu_sbc_a_c() // 0x99
{
	cpu_routine_sbc_a_8(cpu_registers.c);
}

void cpu_sbc_a_d() // 0x9A
{
	cpu_routine_sbc_a_8(cpu_registers.d);
}

void cpu_sbc_a_e() // 0x9B
{
	cpu_routine_sbc_a_8(cpu_registers.e);
}

void cpu_sbc_a_h() // 0x9C
{
	cpu_routine_sbc_a_8(cpu_registers.h);
}

void cpu_sbc_a_l() // 0x9D
{
	cpu_routine_sbc_a_8(cpu_registers.l);
}

void cpu_sbc_a_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp_hl = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	uint32_t temp_a = cpu_registers.a - (temp_hl + GET_FLAG_CARRY);
	SET_FLAG_CARRY((temp_a & ~0xFF) != 0);
	SET_FLAG_ZERO((temp_a & 0xFF) == 0);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_HALF_CARRY(((cpu_registers.a ^ temp_hl ^ temp_a) & 0x10) != 0);
	cpu_registers.a = temp_a;
}

void cpu_sbc_a_a()
{
	core_advance_cpu_clocks(4);
	if (GET_FLAG_CARRY)
	{
		cpu_registers.a = 0xFF;
		SET_FLAG_CARRY(1);
		SET_FLAG_HALF_CARRY(1);
		SET_FLAG_ZERO(0);
	}
	else
	{
		cpu_registers.a = 0;
		SET_FLAG_CARRY(0);
		SET_FLAG_HALF_CARRY(0);
		SET_FLAG_ZERO(1);
	}
	SET_FLAG_SUBTRACT(1);
}

void cpu_and_b() // 0xA0
{
	cpu_routine_and_a_8(cpu_registers.b);
}

void cpu_and_c() // 0xA1
{
	cpu_routine_and_a_8(cpu_registers.c);
}

void cpu_and_d() // 0xA2
{
	cpu_routine_and_a_8(cpu_registers.d);
}

void cpu_and_e() // 0xA3
{
	cpu_routine_and_a_8(cpu_registers.e);
}

void cpu_and_h() // 0xA4
{
	cpu_routine_and_a_8(cpu_registers.h);
}

void cpu_and_l() // 0xA5
{
	cpu_routine_and_a_8(cpu_registers.l);
}

void cpu_and_hl()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_HALF_CARRY(1);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	core_advance_cpu_clocks(4);
	cpu_registers.a &= memory_bus_read(cpu_registers.hl);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_and_a()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_HALF_CARRY(1);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_xor_b() // 0xA8
{
	cpu_routine_xor_a_8(cpu_registers.b);
}

void cpu_xor_c() // 0xA9
{
	cpu_routine_xor_a_8(cpu_registers.c);
}

void cpu_xor_d() // 0xAA
{
	cpu_routine_xor_a_8(cpu_registers.d);
}

void cpu_xor_e() // 0xAB
{
	cpu_routine_xor_a_8(cpu_registers.e);
}

void cpu_xor_h() // 0xAC
{
	cpu_routine_xor_a_8(cpu_registers.h);
}

void cpu_xor_l() // 0xAD
{
	cpu_routine_xor_a_8(cpu_registers.l);
}

void cpu_xor_hl()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	SET_FLAG_HALF_CARRY(0);
	core_advance_cpu_clocks(4);
	cpu_registers.a ^= memory_bus_read(cpu_registers.hl);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_xor_a() // 0xAF
{
	cpu_registers.a = 0;
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_ZERO(1);
	core_advance_cpu_clocks(4);
}

void cpu_or_b() // 0xB0
{
	cpu_routine_or_a_8(cpu_registers.b);
}

void cpu_or_c() // 0xB1
{
	cpu_routine_or_a_8(cpu_registers.c);
}

void cpu_or_d() // 0xB2
{
	cpu_routine_or_a_8(cpu_registers.d);
}

void cpu_or_e() // 0xB3
{
	cpu_routine_or_a_8(cpu_registers.e);
}

void cpu_or_h() // 0xB4
{
	cpu_routine_or_a_8(cpu_registers.h);
}

void cpu_or_l() // 0xB5
{
	cpu_routine_or_a_8(cpu_registers.l);
}

void cpu_or_hl()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_CARRY(0);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	core_advance_cpu_clocks(4);
	cpu_registers.a |= memory_bus_read(cpu_registers.hl);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_or_a()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_cp_b() // 0xB8
{
	cpu_routine_cp_a_8(cpu_registers.b);
}

void cpu_cp_c() // 0xB9
{
	cpu_routine_cp_a_8(cpu_registers.c);
}

void cpu_cp_d() // 0xBA
{
	cpu_routine_cp_a_8(cpu_registers.d);
}

void cpu_cp_e() // 0xBB
{
	cpu_routine_cp_a_8(cpu_registers.e);
}

void cpu_cp_h() // 0xBC
{
	cpu_routine_cp_a_8(cpu_registers.h);
}

void cpu_cp_l() // 0xBD
{
	cpu_routine_cp_a_8(cpu_registers.l);
}

void cpu_cp_hl()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(1);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_HALF_CARRY((cpu_registers.a & 0xF) < (temp & 0xF));
	SET_FLAG_CARRY((uint32_t)cpu_registers.a < temp);
	SET_FLAG_ZERO(cpu_registers.a == temp);
}

void cpu_cp_a()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_ZERO(1);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY(0);
}

void cpu_ret_nz() // 0xC0
{
	cpu_routine_ret_conditional(GET_FLAG_ZERO == 0);
}

void cpu_pop_bc() // 0xC1
{
	cpu_routine_pop_16(cpu_registers.b, cpu_registers.c);
}

void cpu_jp_nz_nn()	// 0xC2
{
	cpu_routine_jp_conditional_nnnn(GET_FLAG_ZERO == 0);
}

void cpu_jp_nn() // 0xC3
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.pc++);
	cpu_registers.pc &= 0xFFFF;
	core_advance_cpu_clocks(4);
	temp |= ((uint32_t)memory_bus_read(cpu_registers.pc++)) << 8;
	cpu_registers.pc &= 0xFFFF;
	core_advance_cpu_clocks(4);
	cpu_registers.pc = temp;
	core_advance_cpu_clocks(4);
}

void cpu_call_nz_nn() // 0xC4
{
	cpu_routine_call_conditional_nnnn(GET_FLAG_ZERO == 0);
}

void cpu_push_bc() // 0xC5
{
	cpu_routine_push_16(cpu_registers.b, cpu_registers.c);
}

void cpu_add_a_n()
{
	core_advance_cpu_clocks(4);
	uint32_t temp_a = cpu_registers.a;
	uint32_t temp_op = memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(((temp_a & 0xF) + (temp_op & 0xF)) > 0xF);
	cpu_registers.a += temp_op;
	SET_FLAG_ZERO(cpu_registers.a == 0);
	SET_FLAG_CARRY(temp_a > cpu_registers.a);
}

void cpu_rst_00() // 0xC7
{
	cpu_routine_rst_nnnn(0x0000);
}

void cpu_ret_z() // 0xC8
{
	cpu_routine_ret_conditional(GET_FLAG_ZERO);
}

void cpu_ret()
{
	core_advance_cpu_clocks(4);
	uint16_t temp = memory_bus_read(cpu_registers.sp++);
	core_advance_cpu_clocks(4);
	temp |= ((uint16_t)memory_bus_read(cpu_registers.sp++)) << 8;
	core_advance_cpu_clocks(4);
	cpu_registers.pc = temp;
	core_advance_cpu_clocks(4);
}

void cpu_jp_z_nn()	// 0xCA
{
	cpu_routine_jp_conditional_nnnn(GET_FLAG_ZERO);
}

void cpu_call_z_nn() // 0xCC
{
	cpu_routine_call_conditional_nnnn(GET_FLAG_ZERO);
}

void cpu_call_nn()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	temp |= ((uint32_t)memory_bus_read(cpu_registers.pc++)) << 8;
	core_advance_cpu_clocks(4);
	cpu_registers.sp--;
	core_advance_cpu_clocks(4);
	const uint8_t pchi = (cpu_registers.pc & 0xFF00) >> 8;
	memory_bus_write(cpu_registers.sp, pchi);
	core_advance_cpu_clocks(4);
	cpu_registers.sp--;
	core_advance_cpu_clocks(4);
	const uint8_t pclo = (cpu_registers.pc & 0xFF);
	memory_bus_write(cpu_registers.sp, pclo);
	cpu_registers.pc = temp;
}

void cpu_adc_a_n()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	uint32_t temp_op = memory_bus_read(cpu_registers.pc++);
	uint32_t temp_a = cpu_registers.a + temp_op + GET_FLAG_CARRY;
	SET_FLAG_HALF_CARRY((((cpu_registers.a & 0xF) + (temp_op & 0xF)) + GET_FLAG_CARRY) > 0xF);
	SET_FLAG_CARRY(temp_a > 0xFF);
	core_advance_cpu_clocks(4);
	cpu_registers.a = (temp_a & 0xFF);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_rst_08() // 0xCF
{
	cpu_routine_rst_nnnn(0x0008);
}

void cpu_ret_nc() // 0xD0
{
	cpu_routine_ret_conditional(GET_FLAG_CARRY == 0);
}

void cpu_pop_de() // 0xD1
{
	cpu_routine_pop_16(cpu_registers.d, cpu_registers.e);
}

void cpu_jp_nc_nn()	// 0xD2
{
	cpu_routine_jp_conditional_nnnn(GET_FLAG_CARRY == 0);
}

void cpu_call_nc_nn() // 0xD4
{
	cpu_routine_call_conditional_nnnn(GET_FLAG_CARRY == 0);
}

void cpu_push_de() // 0xD5
{
	cpu_routine_push_16(cpu_registers.d, cpu_registers.e);
}

void cpu_sub_a_n()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.pc++);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_HALF_CARRY((cpu_registers.a & 0xF) < (temp & 0xF));
	SET_FLAG_CARRY(cpu_registers.a < temp);
	core_advance_cpu_clocks(4);
	cpu_registers.a -= temp;
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_rst_10() // 0xD7
{
	cpu_routine_rst_nnnn(0x0010);
}

void cpu_ret_c() // 0xD8
{
	cpu_routine_ret_conditional(GET_FLAG_CARRY);
}

void cpu_reti()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.sp++);
	cpu_registers.sp &= 0xFFFF;
	core_advance_cpu_clocks(4);
	temp |= ((uint32_t)memory_bus_read(cpu_registers.sp++)) << 8;
	cpu_registers.sp &= 0xFFFF;
	core_advance_cpu_clocks(4);
	cpu_registers.pc = temp;
	core_advance_cpu_clocks(4);
	interrupt_master_enable = true;
}

void cpu_jp_c_nn() // 0xDA
{
	cpu_routine_jp_conditional_nnnn(GET_FLAG_CARRY);
}

void cpu_call_c_nn() // 0xDC
{
	cpu_routine_call_conditional_nnnn(GET_FLAG_CARRY);
}

void cpu_sbc_a_n()
{
	core_advance_cpu_clocks(4);
	uint16_t temp_op = memory_bus_read(cpu_registers.pc++);
	uint16_t temp_a = cpu_registers.a - (temp_op + GET_FLAG_CARRY);
	SET_FLAG_CARRY((temp_a & ~0xFF) != 0);
	SET_FLAG_ZERO((temp_a & 0xFF) == 0);
	SET_FLAG_SUBTRACT(1);
	SET_FLAG_HALF_CARRY(((cpu_registers.a ^ temp_op ^ temp_a) & 0x10) != 0);
	core_advance_cpu_clocks(4);
	cpu_registers.a = temp_a;
}

void cpu_rst_18() // 0xDF
{
	cpu_routine_rst_nnnn(0x0018);
}

void cpu_ldh_n_a()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = 0xFF00 + (uint32_t)memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	memory_bus_write(temp, cpu_registers.a);
	core_advance_cpu_clocks(4);
}

void cpu_pop_hl()
{
	cpu_routine_pop_16(cpu_registers.h, cpu_registers.l);
}

void cpu_ldh_c_a()
{
	core_advance_cpu_clocks(4);
	memory_bus_write(0xFF00 + (uint32_t)cpu_registers.c, cpu_registers.a);
	core_advance_cpu_clocks(4);
}

void cpu_push_hl()
{
	cpu_routine_push_16(cpu_registers.h, cpu_registers.l);
}

void cpu_and_n()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	SET_FLAG_HALF_CARRY(1);
	core_advance_cpu_clocks(4);
	cpu_registers.a &= memory_bus_read(cpu_registers.pc++);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_rst_20() // 0xE7
{
	cpu_routine_rst_nnnn(0x0020);
}

void cpu_add_sp_d()
{
	core_advance_cpu_clocks(4);
	int8_t temp = (int8_t)memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	SET_FLAG_ZERO(0);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(((cpu_registers.sp & 0x00FF) + (temp & 0x00FF)) > 0x00FF);
	SET_FLAG_HALF_CARRY(((cpu_registers.sp & 0x000F) + (temp & 0x000F)) > 0x000F);
	core_advance_cpu_clocks(4);
	cpu_registers.sp = (cpu_registers.sp + temp);
	core_advance_cpu_clocks(4);
}

void cpu_jp_hl()
{
	core_advance_cpu_clocks(4);
	cpu_registers.pc = cpu_registers.hl;
}

void cpu_ld_nn_a()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	cpu_registers.pc & 0xFFFF;
	temp |= ((uint32_t)memory_bus_read(cpu_registers.pc++)) << 8;
	core_advance_cpu_clocks(4);
	cpu_registers.pc &= 0xFFFF;
	core_advance_cpu_clocks(4);
	memory_bus_write(temp, cpu_registers.a);
}

void cpu_xor_n()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	SET_FLAG_HALF_CARRY(0);
	core_advance_cpu_clocks(4);
	cpu_registers.a ^= memory_bus_read(cpu_registers.pc++);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_rst_28() // 0xEF
{
	cpu_routine_rst_nnnn(0x0028);
}

void cpu_ldh_a_n()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = 0xFF00 + (uint32_t)memory_bus_read(cpu_registers.pc++);
	core_advance_cpu_clocks(4);
	cpu_registers.a = memory_bus_read(temp);
	core_advance_cpu_clocks(4);
}

void cpu_pop_af() // 0xF1
{
	cpu_routine_pop_16(cpu_registers.a, cpu_registers.f);
	cpu_registers.f &= 0xF0;	// All flags are reset to 0
}

void cpu_ldh_a_c()
{
	core_advance_cpu_clocks(4);
	cpu_registers.a = memory_bus_read(0xFF00 + cpu_registers.c);
	core_advance_cpu_clocks(4);
}

void cpu_di()
{
	core_advance_cpu_clocks(4);
	interrupt_master_enable = false;
	interrupt_enable_ime_delay = 0;
}

void cpu_push_af()	// 0xF5
{
	cpu_routine_push_16(cpu_registers.a, cpu_registers.f);
}

void cpu_or_n()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_CARRY(0);
	SET_FLAG_HALF_CARRY(0);
	core_advance_cpu_clocks(4);
	cpu_registers.a |= memory_bus_read(cpu_registers.pc++);
	SET_FLAG_ZERO(cpu_registers.a == 0);
}

void cpu_rst_30() // 0xF7
{
	cpu_routine_rst_nnnn(0x0030);
}

void cpu_ld_hl_sp_d()
{
	core_advance_cpu_clocks(4);
	int8_t temp = (int8_t)memory_bus_read(cpu_registers.pc++);
	cpu_registers.pc &= 0xFFFF;
	core_advance_cpu_clocks(4);
	int16_t res = (int16_t)cpu_registers.sp + temp;
	core_advance_cpu_clocks(4);
	cpu_registers.hl = res & 0xFFFF;
	SET_FLAG_ZERO(0);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(((cpu_registers.sp & 0x000F) + (temp & 0x000F)) > 0x000F);
	SET_FLAG_CARRY(((cpu_registers.sp & 0x00FF) + (temp & 0x00FF)) > 0x00FF);
}

void cpu_ld_sp_hl()
{
	core_advance_cpu_clocks(4);
	cpu_registers.sp = cpu_registers.hl;
	core_advance_cpu_clocks(4);
}

void cpu_ld_a_nn()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.pc++);
	cpu_registers.pc &= 0xFFFF;
	core_advance_cpu_clocks(4);
	temp |= ((uint32_t)memory_bus_read(cpu_registers.pc++)) << 8;
	cpu_registers.pc &= 0xFFFF;
	core_advance_cpu_clocks(4);
	cpu_registers.a = memory_bus_read(temp);
	core_advance_cpu_clocks(4);
}

void cpu_ei()
{
	core_advance_cpu_clocks(4);
	interrupt_enable_ime_delay = 1;

	printf(
		"[EI] PC=%04X IME=%d DELAY=%d\n",
		cpu_registers.pc,
		interrupt_master_enable,
		interrupt_enable_ime_delay
	);
}

void cpu_cp_n()
{
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(1);
	uint32_t temp_op = memory_bus_read(cpu_registers.pc++);
	uint32_t temp_a = cpu_registers.a;
	SET_FLAG_HALF_CARRY((temp_a & 0xF) < (temp_op & 0xF));
	SET_FLAG_CARRY(temp_a < temp_op);
	SET_FLAG_ZERO(temp_a == temp_op);
	core_advance_cpu_clocks(4);
}

void cpu_rst_38() // 0xFF
{
	cpu_routine_rst_nnnn(0x0038);
}

void cpu_cb_rlc_b()
{
	cpu_routine_rlc_8(cpu_registers.b);
}

void cpu_cb_rlc_c()
{
	cpu_routine_rlc_8(cpu_registers.c);
}

void cpu_cb_rlc_d()
{
	cpu_routine_rlc_8(cpu_registers.d);
}

void cpu_cb_rlc_e()
{
	cpu_routine_rlc_8(cpu_registers.e);
}

void cpu_cb_rlc_h()
{
	cpu_routine_rlc_8(cpu_registers.h);
}

void cpu_cb_rlc_l()
{
	cpu_routine_rlc_8(cpu_registers.l);
}

void cpu_cb_rlc_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY((temp & 0x80) != 0);
	temp = (temp << 1) | GET_FLAG_CARRY;
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_rlc_a()
{
	cpu_routine_rlc_8(cpu_registers.a);
}

void cpu_cb_rrc_b()
{
	cpu_routine_rrc_8(cpu_registers.b);
}

void cpu_cb_rrc_c()
{
	cpu_routine_rrc_8(cpu_registers.c);
}

void cpu_cb_rrc_d()
{
	cpu_routine_rrc_8(cpu_registers.d);
}

void cpu_cb_rrc_e()
{
	cpu_routine_rrc_8(cpu_registers.e);
}

void cpu_cb_rrc_h()
{
	cpu_routine_rrc_8(cpu_registers.h);
}

void cpu_cb_rrc_l()
{
	cpu_routine_rrc_8(cpu_registers.l);
}

void cpu_cb_rrc_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY((temp & 0x01) != 0);
	temp = (temp >> 1) | (GET_FLAG_CARRY << 7);
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_rrc_a()
{
	cpu_routine_rrc_8(cpu_registers.a);
}

void cpu_cb_rl_b()
{
	cpu_routine_rl_8(cpu_registers.b);
}

void cpu_cb_rl_c()
{
	cpu_routine_rl_8(cpu_registers.c);
}

void cpu_cb_rl_d()
{
	cpu_routine_rl_8(cpu_registers.d);
}

void cpu_cb_rl_e()
{
	cpu_routine_rl_8(cpu_registers.e);
}

void cpu_cb_rl_h()
{
	cpu_routine_rl_8(cpu_registers.h);
}

void cpu_cb_rl_l()
{
	cpu_routine_rl_8(cpu_registers.l);
}

void cpu_cb_rl_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	uint32_t temp_c = GET_FLAG_CARRY;
	SET_FLAG_CARRY((temp & 0x80) != 0);
	temp = ((temp << 1) | temp_c) & 0xFF;
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_rl_a()
{
	cpu_routine_rl_8(cpu_registers.a);
}

void cpu_cb_rr_b()
{
	cpu_routine_rr_8(cpu_registers.b);
}

void cpu_cb_rr_c()
{
	cpu_routine_rr_8(cpu_registers.c);
}

void cpu_cb_rr_d()
{
	cpu_routine_rr_8(cpu_registers.d);
}

void cpu_cb_rr_e()
{
	cpu_routine_rr_8(cpu_registers.e);
}

void cpu_cb_rr_h()
{
	cpu_routine_rr_8(cpu_registers.h);
}

void cpu_cb_rr_l()
{
	cpu_routine_rr_8(cpu_registers.l);
}

void cpu_cb_rr_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	uint32_t temp_c = GET_FLAG_CARRY;
	SET_FLAG_CARRY((temp & 0x01) != 0);
	temp = (temp >> 1) | (temp_c << 7);
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_rr_a()
{
	cpu_routine_rr_8(cpu_registers.a);
}

void cpu_cb_sla_b()
{
	cpu_routine_sla_8(cpu_registers.b);
}

void cpu_cb_sla_c()
{
	cpu_routine_sla_8(cpu_registers.c);
}

void cpu_cb_sla_d()
{
	cpu_routine_sla_8(cpu_registers.d);
}

void cpu_cb_sla_e()
{
	cpu_routine_sla_8(cpu_registers.e);
}

void cpu_cb_sla_h()
{
	cpu_routine_sla_8(cpu_registers.h);
}

void cpu_cb_sla_l()
{
	cpu_routine_sla_8(cpu_registers.l);
}

void cpu_cb_sla_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY((temp & 0x80) != 0);
	temp = (temp << 1) & 0xFF;
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_sla_a()
{
	cpu_routine_sla_8(cpu_registers.a);
}

void cpu_cb_sra_b()
{
	cpu_routine_sra_8(cpu_registers.b);
}

void cpu_cb_sra_c()
{
	cpu_routine_sra_8(cpu_registers.c);
}

void cpu_cb_sra_d()
{
	cpu_routine_sra_8(cpu_registers.d);
}

void cpu_cb_sra_e()
{
	cpu_routine_sra_8(cpu_registers.e);
}

void cpu_cb_sra_h()
{
	cpu_routine_sra_8(cpu_registers.h);
}

void cpu_cb_sra_l()
{
	cpu_routine_sra_8(cpu_registers.l);
}

void cpu_cb_sra_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY((temp & 0x01) != 0);
	temp = (temp & 0x80) | (temp >> 1);
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_sra_a()
{
	cpu_routine_sra_8(cpu_registers.a);
}

void cpu_cb_swap_b()
{
	cpu_routine_swap_8(cpu_registers.b);
}

void cpu_cb_swap_c()
{
	cpu_routine_swap_8(cpu_registers.c);
}

void cpu_cb_swap_d()
{
	cpu_routine_swap_8(cpu_registers.d);
}

void cpu_cb_swap_e()
{
	cpu_routine_swap_8(cpu_registers.e);
}

void cpu_cb_swap_h()
{
	cpu_routine_swap_8(cpu_registers.h);
}

void cpu_cb_swap_l()
{
	cpu_routine_swap_8(cpu_registers.l);
}

void cpu_cb_swap_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY(0);
	temp = ((temp >> 4) | (temp << 4)) & 0xFF;
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_swap_a()
{
	cpu_routine_swap_8(cpu_registers.a);
}

void cpu_cb_srl_b()
{
	cpu_routine_srl_8(cpu_registers.b);
}

void cpu_cb_srl_c()
{
	cpu_routine_srl_8(cpu_registers.c);
}

void cpu_cb_srl_d()
{
	cpu_routine_srl_8(cpu_registers.d);
}

void cpu_cb_srl_e()
{
	cpu_routine_srl_8(cpu_registers.e);
}

void cpu_cb_srl_h()
{
	cpu_routine_srl_8(cpu_registers.h);
}

void cpu_cb_srl_l()
{
	cpu_routine_srl_8(cpu_registers.l);
}

void cpu_cb_srl_hl()
{
	core_advance_cpu_clocks(4);
	uint32_t temp = memory_bus_read(cpu_registers.hl);
	core_advance_cpu_clocks(4);
	SET_FLAG_SUBTRACT(0);
	SET_FLAG_HALF_CARRY(0);
	SET_FLAG_CARRY((temp & 0x01) != 0);
	temp = temp >> 1;
	SET_FLAG_ZERO(temp == 0);
	core_advance_cpu_clocks(4);
	memory_bus_write(cpu_registers.hl, temp);
}

void cpu_cb_srl_a()
{
	cpu_routine_srl_8(cpu_registers.a);
}

void cpu_cb_bit_0_b()
{
	cpu_routine_bit_n_8(0, cpu_registers.b);
}

void cpu_cb_bit_0_c()
{
	cpu_routine_bit_n_8(0, cpu_registers.c);
}

void cpu_cb_bit_0_d()
{
	cpu_routine_bit_n_8(0, cpu_registers.d);
}

void cpu_cb_bit_0_e()
{
	cpu_routine_bit_n_8(0, cpu_registers.e);
}

void cpu_cb_bit_0_h()
{
	cpu_routine_bit_n_8(0, cpu_registers.h);
}

void cpu_cb_bit_0_l()
{
	cpu_routine_bit_n_8(0, cpu_registers.l);
}

void cpu_cb_bit_0_hl()
{
	cpu_routine_bit_n_ptr_hl(0);
}

void cpu_cb_bit_0_a()
{
	cpu_routine_bit_n_8(0, cpu_registers.a);
}

void cpu_cb_bit_1_b()
{
	cpu_routine_bit_n_8(1, cpu_registers.b);
}

void cpu_cb_bit_1_c()
{
	cpu_routine_bit_n_8(1, cpu_registers.c);
}

void cpu_cb_bit_1_d()
{
	cpu_routine_bit_n_8(1, cpu_registers.d);
}

void cpu_cb_bit_1_e()
{
	cpu_routine_bit_n_8(1, cpu_registers.e);
}

void cpu_cb_bit_1_h()
{
	cpu_routine_bit_n_8(1, cpu_registers.h);
}

void cpu_cb_bit_1_l()
{
	cpu_routine_bit_n_8(1, cpu_registers.l);
}

void cpu_cb_bit_1_hl()
{
	cpu_routine_bit_n_ptr_hl(1);
}

void cpu_cb_bit_1_a()
{
	cpu_routine_bit_n_8(1, cpu_registers.a);
}

void cpu_cb_bit_2_b()
{
	cpu_routine_bit_n_8(2, cpu_registers.b);
}

void cpu_cb_bit_2_c()
{
	cpu_routine_bit_n_8(2, cpu_registers.c);
}

void cpu_cb_bit_2_d()
{
	cpu_routine_bit_n_8(2, cpu_registers.d);
}

void cpu_cb_bit_2_e()
{
	cpu_routine_bit_n_8(2, cpu_registers.e);
}

void cpu_cb_bit_2_h()
{
	cpu_routine_bit_n_8(2, cpu_registers.h);
}

void cpu_cb_bit_2_l()
{
	cpu_routine_bit_n_8(2, cpu_registers.l);
}

void cpu_cb_bit_2_hl()
{
	cpu_routine_bit_n_ptr_hl(2);
}

void cpu_cb_bit_2_a()
{
	cpu_routine_bit_n_8(2, cpu_registers.a);
}

void cpu_cb_bit_3_b()
{
	cpu_routine_bit_n_8(3, cpu_registers.b);
}

void cpu_cb_bit_3_c()
{
	cpu_routine_bit_n_8(3, cpu_registers.c);
}

void cpu_cb_bit_3_d()
{
	cpu_routine_bit_n_8(3, cpu_registers.d);
}

void cpu_cb_bit_3_e()
{
	cpu_routine_bit_n_8(3, cpu_registers.e);
}

void cpu_cb_bit_3_h()
{
	cpu_routine_bit_n_8(3, cpu_registers.h);
}

void cpu_cb_bit_3_l()
{
	cpu_routine_bit_n_8(3, cpu_registers.l);
}

void cpu_cb_bit_3_hl()
{
	cpu_routine_bit_n_ptr_hl(3);
}

void cpu_cb_bit_3_a()
{
	cpu_routine_bit_n_8(3, cpu_registers.a);
}

void cpu_cb_bit_4_b()
{
	cpu_routine_bit_n_8(4, cpu_registers.b);
}

void cpu_cb_bit_4_c()
{
	cpu_routine_bit_n_8(4, cpu_registers.c);
}

void cpu_cb_bit_4_d()
{
	cpu_routine_bit_n_8(4, cpu_registers.d);
}

void cpu_cb_bit_4_e()
{
	cpu_routine_bit_n_8(4, cpu_registers.e);
}

void cpu_cb_bit_4_h()
{
	cpu_routine_bit_n_8(4, cpu_registers.h);
}

void cpu_cb_bit_4_l()
{
	cpu_routine_bit_n_8(4, cpu_registers.l);
}

void cpu_cb_bit_4_hl()
{
	cpu_routine_bit_n_ptr_hl(4);
}

void cpu_cb_bit_4_a()
{
	cpu_routine_bit_n_8(4, cpu_registers.a);
}

void cpu_cb_bit_5_b()
{
	cpu_routine_bit_n_8(5, cpu_registers.b);
}

void cpu_cb_bit_5_c()
{
	cpu_routine_bit_n_8(5, cpu_registers.c);
}

void cpu_cb_bit_5_d()
{
	cpu_routine_bit_n_8(5, cpu_registers.d);
}

void cpu_cb_bit_5_e()
{
	cpu_routine_bit_n_8(5, cpu_registers.e);
}

void cpu_cb_bit_5_h()
{
	cpu_routine_bit_n_8(5, cpu_registers.h);
}

void cpu_cb_bit_5_l()
{
	cpu_routine_bit_n_8(5, cpu_registers.l);
}

void cpu_cb_bit_5_hl()
{
	cpu_routine_bit_n_ptr_hl(5);
}

void cpu_cb_bit_5_a()
{
	cpu_routine_bit_n_8(5, cpu_registers.a);
}

void cpu_cb_bit_6_b()
{
	cpu_routine_bit_n_8(6, cpu_registers.b);
}

void cpu_cb_bit_6_c()
{
	cpu_routine_bit_n_8(6, cpu_registers.c);
}

void cpu_cb_bit_6_d()
{
	cpu_routine_bit_n_8(6, cpu_registers.d);
}

void cpu_cb_bit_6_e()
{
	cpu_routine_bit_n_8(6, cpu_registers.e);
}

void cpu_cb_bit_6_h()
{
	cpu_routine_bit_n_8(6, cpu_registers.h);
}

void cpu_cb_bit_6_l()
{
	cpu_routine_bit_n_8(6, cpu_registers.l);
}

void cpu_cb_bit_6_hl()
{
	cpu_routine_bit_n_ptr_hl(6);
}

void cpu_cb_bit_6_a()
{
	cpu_routine_bit_n_8(6, cpu_registers.a);
}

void cpu_cb_bit_7_b()
{
	cpu_routine_bit_n_8(7, cpu_registers.b);
}

void cpu_cb_bit_7_c()
{
	cpu_routine_bit_n_8(7, cpu_registers.c);
}

void cpu_cb_bit_7_d()
{
	cpu_routine_bit_n_8(7, cpu_registers.d);
}

void cpu_cb_bit_7_e()
{
	cpu_routine_bit_n_8(7, cpu_registers.e);
}

void cpu_cb_bit_7_h()
{
	cpu_routine_bit_n_8(7, cpu_registers.h);
}

void cpu_cb_bit_7_l()
{
	cpu_routine_bit_n_8(7, cpu_registers.l);
}

void cpu_cb_bit_7_hl()
{
	cpu_routine_bit_n_ptr_hl(7);
}

void cpu_cb_bit_7_a()
{
	cpu_routine_bit_n_8(7, cpu_registers.a);
}

void cpu_cb_res_0_b()
{
	cpu_routine_res_n_8(0, cpu_registers.b);
}

void cpu_cb_res_0_c()
{
	cpu_routine_res_n_8(0, cpu_registers.c);
}

void cpu_cb_res_0_d()
{
	cpu_routine_res_n_8(0, cpu_registers.d);
}

void cpu_cb_res_0_e()
{
	cpu_routine_res_n_8(0, cpu_registers.e);
}

void cpu_cb_res_0_h()
{
	cpu_routine_res_n_8(0, cpu_registers.h);
}

void cpu_cb_res_0_l()
{
	cpu_routine_res_n_8(0, cpu_registers.l);
}

void cpu_cb_res_0_hl()
{
	cpu_routine_res_n_ptr_hl(0);
}

void cpu_cb_res_0_a()
{
	cpu_routine_res_n_8(0, cpu_registers.a);
}

void cpu_cb_res_1_b()
{
	cpu_routine_res_n_8(1, cpu_registers.b);
}

void cpu_cb_res_1_c()
{
	cpu_routine_res_n_8(1, cpu_registers.c);
}

void cpu_cb_res_1_d()
{
	cpu_routine_res_n_8(1, cpu_registers.d);
}

void cpu_cb_res_1_e()
{
	cpu_routine_res_n_8(1, cpu_registers.e);
}

void cpu_cb_res_1_h()
{
	cpu_routine_res_n_8(1, cpu_registers.h);
}

void cpu_cb_res_1_l()
{
	cpu_routine_res_n_8(1, cpu_registers.l);
}

void cpu_cb_res_1_hl()
{
	cpu_routine_res_n_ptr_hl(1);
}

void cpu_cb_res_1_a()
{
	cpu_routine_res_n_8(1, cpu_registers.a);
}

void cpu_cb_res_2_b()
{
	cpu_routine_res_n_8(2, cpu_registers.b);
}

void cpu_cb_res_2_c()
{
	cpu_routine_res_n_8(2, cpu_registers.c);
}

void cpu_cb_res_2_d()
{
	cpu_routine_res_n_8(2, cpu_registers.d);
}

void cpu_cb_res_2_e()
{
	cpu_routine_res_n_8(2, cpu_registers.e);
}

void cpu_cb_res_2_h()
{
	cpu_routine_res_n_8(2, cpu_registers.h);
}

void cpu_cb_res_2_l()
{
	cpu_routine_res_n_8(2, cpu_registers.l);
}

void cpu_cb_res_2_hl()
{
	cpu_routine_res_n_ptr_hl(2);
}

void cpu_cb_res_2_a()
{
	cpu_routine_res_n_8(2, cpu_registers.a);
}

void cpu_cb_res_3_b()
{
	cpu_routine_res_n_8(3, cpu_registers.b);
}

void cpu_cb_res_3_c()
{
	cpu_routine_res_n_8(3, cpu_registers.c);
}

void cpu_cb_res_3_d()
{
	cpu_routine_res_n_8(3, cpu_registers.d);
}

void cpu_cb_res_3_e()
{
	cpu_routine_res_n_8(3, cpu_registers.e);
}

void cpu_cb_res_3_h()
{
	cpu_routine_res_n_8(3, cpu_registers.h);
}

void cpu_cb_res_3_l()
{
	cpu_routine_res_n_8(3, cpu_registers.l);
}

void cpu_cb_res_3_hl()
{
	cpu_routine_res_n_ptr_hl(3);
}

void cpu_cb_res_3_a()
{
	cpu_routine_res_n_8(3, cpu_registers.a);
}

void cpu_cb_res_4_b()
{
	cpu_routine_res_n_8(4, cpu_registers.b);
}

void cpu_cb_res_4_c()
{
	cpu_routine_res_n_8(4, cpu_registers.c);
}

void cpu_cb_res_4_d()
{
	cpu_routine_res_n_8(4, cpu_registers.d);
}

void cpu_cb_res_4_e()
{
	cpu_routine_res_n_8(4, cpu_registers.e);
}

void cpu_cb_res_4_h()
{
	cpu_routine_res_n_8(4, cpu_registers.h);
}

void cpu_cb_res_4_l()
{
	cpu_routine_res_n_8(4, cpu_registers.l);
}

void cpu_cb_res_4_hl()
{
	cpu_routine_res_n_ptr_hl(4);
}

void cpu_cb_res_4_a()
{
	cpu_routine_res_n_8(4, cpu_registers.a);
}

void cpu_cb_res_5_b()
{
	cpu_routine_res_n_8(5, cpu_registers.b);
}

void cpu_cb_res_5_c()
{
	cpu_routine_res_n_8(5, cpu_registers.c);
}

void cpu_cb_res_5_d()
{
	cpu_routine_res_n_8(5, cpu_registers.d);
}

void cpu_cb_res_5_e()
{
	cpu_routine_res_n_8(5, cpu_registers.e);
}

void cpu_cb_res_5_h()
{
	cpu_routine_res_n_8(5, cpu_registers.h);
}

void cpu_cb_res_5_l()
{
	cpu_routine_res_n_8(5, cpu_registers.l);
}

void cpu_cb_res_5_hl()
{
	cpu_routine_res_n_ptr_hl(5);
}

void cpu_cb_res_5_a()
{
	cpu_routine_res_n_8(5, cpu_registers.a);
}

void cpu_cb_res_6_b()
{
	cpu_routine_res_n_8(6, cpu_registers.b);
}

void cpu_cb_res_6_c()
{
	cpu_routine_res_n_8(6, cpu_registers.c);
}

void cpu_cb_res_6_d()
{
	cpu_routine_res_n_8(6, cpu_registers.d);
}

void cpu_cb_res_6_e()
{
	cpu_routine_res_n_8(6, cpu_registers.e);
}

void cpu_cb_res_6_h()
{
	cpu_routine_res_n_8(6, cpu_registers.h);
}

void cpu_cb_res_6_l()
{
	cpu_routine_res_n_8(6, cpu_registers.l);
}

void cpu_cb_res_6_hl()
{
	cpu_routine_res_n_ptr_hl(6);
}

void cpu_cb_res_6_a()
{
	cpu_routine_res_n_8(6, cpu_registers.a);
}

void cpu_cb_res_7_b()
{
	cpu_routine_res_n_8(7, cpu_registers.b);
}

void cpu_cb_res_7_c()
{
	cpu_routine_res_n_8(7, cpu_registers.c);
}

void cpu_cb_res_7_d()
{
	cpu_routine_res_n_8(7, cpu_registers.d);
}

void cpu_cb_res_7_e()
{
	cpu_routine_res_n_8(7, cpu_registers.e);
}

void cpu_cb_res_7_h()
{
	cpu_routine_res_n_8(7, cpu_registers.h);
}

void cpu_cb_res_7_l()
{
	cpu_routine_res_n_8(7, cpu_registers.l);
}

void cpu_cb_res_7_hl()
{
	cpu_routine_res_n_ptr_hl(7);
}

void cpu_cb_res_7_a()
{
	cpu_routine_res_n_8(7, cpu_registers.a);
}

void cpu_cb_set_0_b()
{
	cpu_routine_set_n_8(0, cpu_registers.b);
}

void cpu_cb_set_0_c()
{
	cpu_routine_set_n_8(0, cpu_registers.c);
}

void cpu_cb_set_0_d()
{
	cpu_routine_set_n_8(0, cpu_registers.d);
}

void cpu_cb_set_0_e()
{
	cpu_routine_set_n_8(0, cpu_registers.e);
}

void cpu_cb_set_0_h()
{
	cpu_routine_set_n_8(0, cpu_registers.h);
}

void cpu_cb_set_0_l()
{
	cpu_routine_set_n_8(0, cpu_registers.l);
}

void cpu_cb_set_0_hl()
{
	cpu_routine_set_n_ptr_hl(0);
}

void cpu_cb_set_0_a()
{
	cpu_routine_set_n_8(0, cpu_registers.a);
}

void cpu_cb_set_1_b()
{
	cpu_routine_set_n_8(1, cpu_registers.b);
}

void cpu_cb_set_1_c()
{
	cpu_routine_set_n_8(1, cpu_registers.c);
}

void cpu_cb_set_1_d()
{
	cpu_routine_set_n_8(1, cpu_registers.d);
}

void cpu_cb_set_1_e()
{
	cpu_routine_set_n_8(1, cpu_registers.e);
}

void cpu_cb_set_1_h()
{
	cpu_routine_set_n_8(1, cpu_registers.h);
}

void cpu_cb_set_1_l()
{
	cpu_routine_set_n_8(1, cpu_registers.l);
}

void cpu_cb_set_1_hl()
{
	cpu_routine_set_n_ptr_hl(1);
}

void cpu_cb_set_1_a()
{
	cpu_routine_set_n_8(1, cpu_registers.a);
}

void cpu_cb_set_2_b()
{
	cpu_routine_set_n_8(2, cpu_registers.b);
}

void cpu_cb_set_2_c()
{
	cpu_routine_set_n_8(2, cpu_registers.c);
}

void cpu_cb_set_2_d()
{
	cpu_routine_set_n_8(2, cpu_registers.d);
}

void cpu_cb_set_2_e()
{
	cpu_routine_set_n_8(2, cpu_registers.e);
}

void cpu_cb_set_2_h()
{
	cpu_routine_set_n_8(2, cpu_registers.h);
}

void cpu_cb_set_2_l()
{
	cpu_routine_set_n_8(2, cpu_registers.l);
}

void cpu_cb_set_2_hl()
{
	cpu_routine_set_n_ptr_hl(2);
}

void cpu_cb_set_2_a()
{
	cpu_routine_set_n_8(2, cpu_registers.a);
}

void cpu_cb_set_3_b()
{
	cpu_routine_set_n_8(3, cpu_registers.b);
}

void cpu_cb_set_3_c()
{
	cpu_routine_set_n_8(3, cpu_registers.c);
}

void cpu_cb_set_3_d()
{
	cpu_routine_set_n_8(3, cpu_registers.d);
}

void cpu_cb_set_3_e()
{
	cpu_routine_set_n_8(3, cpu_registers.e);
}

void cpu_cb_set_3_h()
{
	cpu_routine_set_n_8(3, cpu_registers.h);
}

void cpu_cb_set_3_l()
{
	cpu_routine_set_n_8(3, cpu_registers.l);
}

void cpu_cb_set_3_hl()
{
	cpu_routine_set_n_ptr_hl(3);
}

void cpu_cb_set_3_a()
{
	cpu_routine_set_n_8(3, cpu_registers.a);
}

void cpu_cb_set_4_b()
{
	cpu_routine_set_n_8(4, cpu_registers.b);
}

void cpu_cb_set_4_c()
{
	cpu_routine_set_n_8(4, cpu_registers.c);
}

void cpu_cb_set_4_d()
{
	cpu_routine_set_n_8(4, cpu_registers.d);
}

void cpu_cb_set_4_e()
{
	cpu_routine_set_n_8(4, cpu_registers.e);
}

void cpu_cb_set_4_h()
{
	cpu_routine_set_n_8(4, cpu_registers.h);
}

void cpu_cb_set_4_l()
{
	cpu_routine_set_n_8(4, cpu_registers.l);
}

void cpu_cb_set_4_hl()
{
	cpu_routine_set_n_ptr_hl(4);
}

void cpu_cb_set_4_a()
{
	cpu_routine_set_n_8(4, cpu_registers.a);
}

void cpu_cb_set_5_b()
{
	cpu_routine_set_n_8(5, cpu_registers.b);
}

void cpu_cb_set_5_c()
{
	cpu_routine_set_n_8(5, cpu_registers.c);
}

void cpu_cb_set_5_d()
{
	cpu_routine_set_n_8(5, cpu_registers.d);
}

void cpu_cb_set_5_e()
{
	cpu_routine_set_n_8(5, cpu_registers.e);
}

void cpu_cb_set_5_h()
{
	cpu_routine_set_n_8(5, cpu_registers.h);
}

void cpu_cb_set_5_l()
{
	cpu_routine_set_n_8(5, cpu_registers.l);
}

void cpu_cb_set_5_hl()
{
	cpu_routine_set_n_ptr_hl(5);
}

void cpu_cb_set_5_a()
{
	cpu_routine_set_n_8(5, cpu_registers.a);
}

void cpu_cb_set_6_b()
{
	cpu_routine_set_n_8(6, cpu_registers.b);
}

void cpu_cb_set_6_c()
{
	cpu_routine_set_n_8(6, cpu_registers.c);
}

void cpu_cb_set_6_d()
{
	cpu_routine_set_n_8(6, cpu_registers.d);
}

void cpu_cb_set_6_e()
{
	cpu_routine_set_n_8(6, cpu_registers.e);
}

void cpu_cb_set_6_h()
{
	cpu_routine_set_n_8(6, cpu_registers.h);
}

void cpu_cb_set_6_l()
{
	cpu_routine_set_n_8(6, cpu_registers.l);
}

void cpu_cb_set_6_hl()
{
	cpu_routine_set_n_ptr_hl(6);
}

void cpu_cb_set_6_a()
{
	cpu_routine_set_n_8(6, cpu_registers.a);
}

void cpu_cb_set_7_b()
{
	cpu_routine_set_n_8(7, cpu_registers.b);
}

void cpu_cb_set_7_c()
{
	cpu_routine_set_n_8(7, cpu_registers.c);
}

void cpu_cb_set_7_d()
{
	cpu_routine_set_n_8(7, cpu_registers.d);
}

void cpu_cb_set_7_e()
{
	cpu_routine_set_n_8(7, cpu_registers.e);
}

void cpu_cb_set_7_h()
{
	cpu_routine_set_n_8(7, cpu_registers.h);
}

void cpu_cb_set_7_l()
{
	cpu_routine_set_n_8(7, cpu_registers.l);
}

void cpu_cb_set_7_hl()
{
	cpu_routine_set_n_ptr_hl(7);
}

void cpu_cb_set_7_a()
{
	cpu_routine_set_n_8(7, cpu_registers.a);
}