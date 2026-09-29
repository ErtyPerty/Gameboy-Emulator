#include "cpu.h"
#include "memory_bus.h"
#include "cpu_instructions.h"
#include "emulator_core.h"
#include "cpu_routines.h"

#include <cstdio>

gb_cpu_registers cpu_registers;

bool cpu_halted = false;
bool cpu_stopped = false;
bool cpu_ime = false;

uint8_t cpu_current_op_code = 0;
uint8_t cpu_current_cb_op_code = 0;
uint32_t cpu_instruction_counter = 0;

cpu_execute_op cpu_current_instruction_execute = nullptr;


static uint8_t &cpu_get_register_8(uint8_t index){
    switch (index & 7){
        case 0: return cpu_registers.b;
        case 1: return cpu_registers.c;
        case 2: return cpu_registers.d;
        case 3: return cpu_registers.e;
        case 4: return cpu_registers.h;
        case 5: return cpu_registers.l;
        default: return cpu_registers.a;
    }
}


static uint8_t cpu_read_register_or_hl(uint8_t index){
    if ((index & 7) == 6){
        return memory_bus_read(cpu_registers.hl);
    }

    return cpu_get_register_8(index);
}


static void cpu_write_register_or_hl(uint8_t index, uint8_t value){
    if ((index & 7) == 6){
        memory_bus_write(cpu_registers.hl, value);
        return;
    }

    cpu_get_register_8(index) = value;
}


static uint16_t cpu_condition_address(){
    return cpu_routine_read_nn();
}


static void cpu_call_to(uint16_t address){
    uint16_t return_address = cpu_registers.pc;

    core_advance_cpu_clocks(4);

    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, (uint8_t)(return_address >> 8));

    core_advance_cpu_clocks(4);

    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, (uint8_t)(return_address & 0xFF));

    core_advance_cpu_clocks(4);

    cpu_registers.pc = address;
}


static void cpu_rst_to(uint16_t address){
    uint16_t return_address = cpu_registers.pc;

    core_advance_cpu_clocks(4);
    core_advance_cpu_clocks(4);

    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, (uint8_t)(return_address >> 8));

    core_advance_cpu_clocks(4);

    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, (uint8_t)(return_address & 0xFF));

    core_advance_cpu_clocks(4);

    cpu_registers.pc = address;
}

void cpu_reset(){
    cpu_registers.af = 0x01B0;
    cpu_registers.bc = 0x0013;
    cpu_registers.de = 0x00D8;
    cpu_registers.hl = 0x014D;
    cpu_registers.sp = 0xFFFE;
    cpu_registers.pc = 0x0100;

    cpu_halted = false;
    cpu_stopped = false;
    cpu_ime = false;

    cpu_current_op_code = 0;
    cpu_current_cb_op_code = 0;
    cpu_current_instruction_execute = nullptr;
    cpu_instruction_counter = 0;
}


void cpu_fetch(){
    cpu_current_op_code = memory_bus_read(cpu_registers.pc++);

    const gb_cpu_instruction& instruction = instructions[cpu_current_op_code];
    cpu_current_instruction_execute = instruction.execute;
}


bool cpu_execute(){
    if (!cpu_current_instruction_execute){
        const gb_cpu_instruction& instruction = instructions[cpu_current_op_code];

        const uint8_t pchi = (uint8_t)((cpu_registers.pc - 1) >> 8);
        const uint8_t pclo = (uint8_t)((cpu_registers.pc - 1) & 0xFF);

        printf(
            "Unknown instruction %.2X at: %.2X%.2X (%s), count %u\n",
            cpu_current_op_code,
            pchi,
            pclo,
            instruction.disassembly,
            cpu_instruction_counter
        );

        return false;
    }

    cpu_current_instruction_execute();

    cpu_instruction_counter++;

    return true;
}


// 0x00
void cpu_noop(){
    core_advance_cpu_clocks(4);
}

// 0x01
void cpu_ld_bc_nn(){
    cpu_routine_ld_16(cpu_registers.b, cpu_registers.c);
}

// 0x02
void cpu_ld_bc_a(){
    core_advance_cpu_clocks(4);
    memory_bus_write(cpu_registers.bc, cpu_registers.a);
    core_advance_cpu_clocks(4);
}

// 0x03
void cpu_inc_bc(){
    cpu_routine_inc_16(cpu_registers.bc);
}

// 0x04
void cpu_inc_b(){
    cpu_routine_inc_8(cpu_registers.b);
}

// 0x05
void cpu_dec_b(){
    cpu_routine_dec_8(cpu_registers.b);
}

// 0x06
void cpu_ld_b_n(){
    cpu_routine_ld_8(cpu_registers.b);
}

// 0x07
void cpu_rlca(){
    cpu_routine_rlca();
}

// 0x08
void cpu_ld_nn_sp(){
    cpu_routine_ld_nn_sp();
}

// 0x09
void cpu_add_hl_bc(){
    cpu_routine_add_hl(cpu_registers.bc);
}

// 0x0A
void cpu_ld_a_bc(){
    core_advance_cpu_clocks(4);
    cpu_registers.a = memory_bus_read(cpu_registers.bc);
    core_advance_cpu_clocks(4);
}

// 0x0B
void cpu_dec_bc(){
    cpu_routine_dec_16(cpu_registers.bc);
}

// 0x0C
void cpu_inc_c(){
    cpu_routine_inc_8(cpu_registers.c);
}

// 0x0D
void cpu_dec_c(){
    cpu_routine_dec_8(cpu_registers.c);
}

// 0x0E
void cpu_ld_c_n(){
    cpu_routine_ld_8(cpu_registers.c);
}

// 0x0F
void cpu_rrca(){
    cpu_routine_rrca();
}

// 0x10
void cpu_stop(){
    memory_bus_read(cpu_registers.pc++);
    cpu_stopped = true;
    core_advance_cpu_clocks(4);
}

// 0x11
void cpu_ld_de_nn(){
    cpu_routine_ld_16(cpu_registers.d, cpu_registers.e);
}

// 0x12
void cpu_ld_de_a(){
    core_advance_cpu_clocks(4);
    memory_bus_write(cpu_registers.de, cpu_registers.a);
    core_advance_cpu_clocks(4);
}

// 0x13
void cpu_inc_de(){
    cpu_routine_inc_16(cpu_registers.de);
}

// 0x14
void cpu_inc_d(){
    cpu_routine_inc_8(cpu_registers.d);
}

// 0x15
void cpu_dec_d(){
    cpu_routine_dec_8(cpu_registers.d);
}

// 0x16
void cpu_ld_d_n(){
    cpu_routine_ld_8(cpu_registers.d);
}

// 0x17
void cpu_rla(){
    cpu_routine_rla();
}

// 0x18
void cpu_jr(){
    core_advance_cpu_clocks(4);
    int8_t offset = (int8_t)memory_bus_read(cpu_registers.pc++);

    core_advance_cpu_clocks(4);
    cpu_registers.pc = (uint16_t)(cpu_registers.pc + offset);

    core_advance_cpu_clocks(4);
}

// 0x19
void cpu_add_hl_de(){
    cpu_routine_add_hl(cpu_registers.de);
}

// 0x1A
void cpu_ld_a_de(){
    core_advance_cpu_clocks(4);
    cpu_registers.a = memory_bus_read(cpu_registers.de);
    core_advance_cpu_clocks(4);
}

// 0x1B
void cpu_dec_de(){
    cpu_routine_dec_16(cpu_registers.de);
}

// 0x1C
void cpu_inc_e(){
    cpu_routine_inc_8(cpu_registers.e);
}

// 0x1D
void cpu_dec_e(){
    cpu_routine_dec_8(cpu_registers.e);
}

// 0x1E
void cpu_ld_e_n(){
    cpu_routine_ld_8(cpu_registers.e);
}

// 0x1F
void cpu_rra(){
    cpu_routine_rra();
}

// 0x20
void cpu_jr_nz(){
    core_advance_cpu_clocks(4);
    int8_t offset = (int8_t)memory_bus_read(cpu_registers.pc++);

    if (GET_FLAG_ZERO()){
        core_advance_cpu_clocks(4);
        return;
    }

    cpu_registers.pc = (uint16_t)(cpu_registers.pc + offset);
    core_advance_cpu_clocks(8);
}

// 0x21
void cpu_ld_hl_nn(){
    cpu_routine_ld_16(cpu_registers.h, cpu_registers.l);
}

// 0x22
void cpu_ld_hli_a(){
    core_advance_cpu_clocks(4);
    memory_bus_write(cpu_registers.hl, cpu_registers.a);
    cpu_registers.hl++;
    core_advance_cpu_clocks(4);
}

// 0x23
void cpu_inc_hl(){
    cpu_routine_inc_16(cpu_registers.hl);
}

// 0x24
void cpu_inc_h(){
    cpu_routine_inc_8(cpu_registers.h);
}

// 0x25
void cpu_dec_h(){
    cpu_routine_dec_8(cpu_registers.h);
}

// 0x26
void cpu_ld_h_n(){
    cpu_routine_ld_8(cpu_registers.h);
}

// 0x27
void cpu_daa(){
    cpu_routine_daa();
}

// 0x28
void cpu_jr_z(){
    core_advance_cpu_clocks(4);
    int8_t offset = (int8_t)memory_bus_read(cpu_registers.pc++);

    if (!GET_FLAG_ZERO()){
        core_advance_cpu_clocks(4);
        return;
    }

    cpu_registers.pc = (uint16_t)(cpu_registers.pc + offset);
    core_advance_cpu_clocks(8);
}

// 0x29
void cpu_add_hl_hl(){
    cpu_routine_add_hl(cpu_registers.hl);
}

// 0x2A
void cpu_ld_a_hli(){
    core_advance_cpu_clocks(4);
    cpu_registers.a = memory_bus_read(cpu_registers.hl);
    cpu_registers.hl++;
    core_advance_cpu_clocks(4);
}

// 0x2B
void cpu_dec_hl(){
    cpu_routine_dec_16(cpu_registers.hl);
}

// 0x2C
void cpu_inc_l(){
    cpu_routine_inc_8(cpu_registers.l);
}

// 0x2D
void cpu_dec_l(){
    cpu_routine_dec_8(cpu_registers.l);
}

// 0x2E
void cpu_ld_l_n(){
    cpu_routine_ld_8(cpu_registers.l);
}

// 0x2F
void cpu_cpl(){
    cpu_registers.a = (uint8_t)~cpu_registers.a;

    SET_FLAG_SUBTRACT(1);
    SET_FLAG_HALF_CARRY(1);

    core_advance_cpu_clocks(4);
}

// 0x30
void cpu_jr_nc(){
    core_advance_cpu_clocks(4);
    int8_t offset = (int8_t)memory_bus_read(cpu_registers.pc++);

    if (GET_FLAG_CARRY()){
        core_advance_cpu_clocks(4);
        return;
    }

    cpu_registers.pc = (uint16_t)(cpu_registers.pc + offset);
    core_advance_cpu_clocks(8);
}

// 0x31
void cpu_ld_sp_nn(){
    cpu_routine_ld_16(cpu_registers.s, cpu_registers.p);
}

// 0x32
void cpu_ldd_hl_a(){
    core_advance_cpu_clocks(4);
    memory_bus_write(cpu_registers.hl, cpu_registers.a);
    cpu_registers.hl--;
    core_advance_cpu_clocks(4);
}

// 0x33
void cpu_inc_sp(){
    cpu_routine_inc_16(cpu_registers.sp);
}

// 0x34
void cpu_inc_hl_mem(){
    core_advance_cpu_clocks(4);

    uint8_t value = memory_bus_read(cpu_registers.hl);
    uint8_t old = value++;

    SET_FLAG_ZERO(value == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY((old & 0x0F) == 0x0F);

    core_advance_cpu_clocks(4);
    memory_bus_write(cpu_registers.hl, value);
    core_advance_cpu_clocks(4);
}

// 0x35
void cpu_dec_hl_mem(){
    core_advance_cpu_clocks(4);

    uint8_t value = memory_bus_read(cpu_registers.hl);
    uint8_t old = value--;

    SET_FLAG_ZERO(value == 0);
    SET_FLAG_SUBTRACT(1);
    SET_FLAG_HALF_CARRY((old & 0x0F) == 0x00);

    core_advance_cpu_clocks(4);
    memory_bus_write(cpu_registers.hl, value);
    core_advance_cpu_clocks(4);
}

// 0x36
void cpu_ld_hl_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    core_advance_cpu_clocks(4);
    memory_bus_write(cpu_registers.hl, value);

    core_advance_cpu_clocks(4);
}

// 0x37
void cpu_scf(){
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(1);

    core_advance_cpu_clocks(4);
}

// 0x38
void cpu_jr_c(){
    core_advance_cpu_clocks(4);
    int8_t offset = (int8_t)memory_bus_read(cpu_registers.pc++);

    if (!GET_FLAG_CARRY()){
        core_advance_cpu_clocks(4);
        return;
    }

    cpu_registers.pc = (uint16_t)(cpu_registers.pc + offset);
    core_advance_cpu_clocks(8);
}

// 0x39
void cpu_add_hl_sp(){
    cpu_routine_add_hl(cpu_registers.sp);
}

// 0x3A
void cpu_ld_a_hld(){
    core_advance_cpu_clocks(4);
    cpu_registers.a = memory_bus_read(cpu_registers.hl);
    cpu_registers.hl--;
    core_advance_cpu_clocks(4);
}

// 0x3B
void cpu_dec_sp(){
    cpu_routine_dec_16(cpu_registers.sp);
}

// 0x3C
void cpu_inc_a(){
    cpu_routine_inc_8(cpu_registers.a);
}

// 0x3D
void cpu_dec_a(){
    cpu_routine_dec_8(cpu_registers.a);
}

// 0x3E
void cpu_ld_a_n(){
    cpu_routine_ld_8(cpu_registers.a);
}

// 0x3F
void cpu_ccf(){
    uint8_t carry = GET_FLAG_CARRY();

    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(!carry);

    core_advance_cpu_clocks(4);
}

// 0x40-0x7F
void cpu_ld_r_r(){
    uint8_t dst = (cpu_current_op_code >> 3) & 7;
    uint8_t src = cpu_current_op_code & 7;
    uint8_t value = cpu_read_register_or_hl(src);

    cpu_write_register_or_hl(dst, value);

    core_advance_cpu_clocks((dst == 6 || src == 6) ? 8 : 4);
}

// 0x76
void cpu_halt(){
    cpu_halted = true;
    core_advance_cpu_clocks(4);
}

// 0x80-0xBF
void cpu_alu_r(){
    uint8_t group = (cpu_current_op_code >> 3) & 7;
    uint8_t src = cpu_current_op_code & 7;
    uint8_t value = cpu_read_register_or_hl(src);

    switch (group){
        case 0: cpu_routine_add_a(value); break;
        case 1: cpu_routine_adc_a(value); break;
        case 2: cpu_routine_sub_a(value); break;
        case 3: cpu_routine_sbc_a(value); break;
        case 4: cpu_routine_and_a(value); break;
        case 5: cpu_routine_xor_a(value); break;
        case 6: cpu_routine_or_a(value); break;
        case 7: cpu_routine_cp_a(value); break;
    }

    core_advance_cpu_clocks((src == 6) ? 8 : 4);
}

// 0xC0
void cpu_ret_nz(){
    core_advance_cpu_clocks(4);

    if (GET_FLAG_ZERO()){
        core_advance_cpu_clocks(4);
        return;
    }

    core_advance_cpu_clocks(4);
    cpu_registers.pc = cpu_routine_pop_16();
}

// 0xC1
void cpu_pop_bc(){
    cpu_registers.bc = cpu_routine_pop_16();
}

// 0xC2
void cpu_jp_nz(){
    uint16_t address = cpu_condition_address();

    if (GET_FLAG_ZERO()){
        return;
    }

    cpu_registers.pc = address;
    core_advance_cpu_clocks(4);
}

// 0xC3
void cpu_jp_nn(){
    uint16_t address = cpu_routine_read_nn();

    core_advance_cpu_clocks(4);
    cpu_registers.pc = address;
}

// 0xC4
void cpu_call_nz(){
    uint16_t address = cpu_condition_address();

    if (GET_FLAG_ZERO()){
        return;
    }

    cpu_call_to(address);
}

// 0xC5
void cpu_push_bc(){
    cpu_routine_push_16(cpu_registers.bc);
}

// 0xC6
void cpu_add_a_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_add_a(value);
    core_advance_cpu_clocks(4);
}

// 0xC7
void cpu_rst_00(){
    cpu_rst_to(0x0000);
}

// 0xC8
void cpu_ret_z(){
    core_advance_cpu_clocks(4);

    if (!GET_FLAG_ZERO()){
        core_advance_cpu_clocks(4);
        return;
    }

    core_advance_cpu_clocks(4);
    cpu_registers.pc = cpu_routine_pop_16();
}

// 0xC9
void cpu_ret(){
    core_advance_cpu_clocks(4);
    cpu_registers.pc = cpu_routine_pop_16();
}

// 0xCA
void cpu_jp_z(){
    uint16_t address = cpu_condition_address();

    if (!GET_FLAG_ZERO()){
        return;
    }

    cpu_registers.pc = address;
    core_advance_cpu_clocks(4);
}

// 0xCB
void cpu_prefix_cb(){
    core_advance_cpu_clocks(4);

    cpu_current_cb_op_code = memory_bus_read(cpu_registers.pc++);

    const gb_cpu_instruction& instruction = cb_instructions[cpu_current_cb_op_code];
    instruction.execute();
}

// 0xCC
void cpu_call_z(){
    uint16_t address = cpu_condition_address();

    if (!GET_FLAG_ZERO()){
        return;
    }

    cpu_call_to(address);
}

// 0xCD
void cpu_call_nn(){
    uint16_t address = cpu_routine_read_nn();
    cpu_call_to(address);
}

// 0xCE
void cpu_adc_a_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_adc_a(value);
    core_advance_cpu_clocks(4);
}

// 0xCF
void cpu_rst_08(){
    cpu_rst_to(0x0008);
}

// 0xD0
void cpu_ret_nc(){
    core_advance_cpu_clocks(4);

    if (GET_FLAG_CARRY()){
        core_advance_cpu_clocks(4);
        return;
    }

    core_advance_cpu_clocks(4);
    cpu_registers.pc = cpu_routine_pop_16();
}

// 0xD1
void cpu_pop_de(){
    cpu_registers.de = cpu_routine_pop_16();
}

// 0xD2
void cpu_jp_nc(){
    uint16_t address = cpu_condition_address();

    if (GET_FLAG_CARRY()){
        return;
    }

    cpu_registers.pc = address;
    core_advance_cpu_clocks(4);
}

// 0xD4
void cpu_call_nc(){
    uint16_t address = cpu_condition_address();

    if (GET_FLAG_CARRY()){
        return;
    }

    cpu_call_to(address);
}

// 0xD5
void cpu_push_de(){
    cpu_routine_push_16(cpu_registers.de);
}

// 0xD6
void cpu_sub_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_sub_a(value);
    core_advance_cpu_clocks(4);
}

// 0xD7
void cpu_rst_10(){
    cpu_rst_to(0x0010);
}

// 0xD8
void cpu_ret_c(){
    core_advance_cpu_clocks(4);

    if (!GET_FLAG_CARRY()){
        core_advance_cpu_clocks(4);
        return;
    }

    core_advance_cpu_clocks(4);
    cpu_registers.pc = cpu_routine_pop_16();
}

// 0xD9
void cpu_reti(){
    core_advance_cpu_clocks(4);
    cpu_registers.pc = cpu_routine_pop_16();
    cpu_ime = true;
}

// 0xDA
void cpu_jp_c(){
    uint16_t address = cpu_condition_address();

    if (!GET_FLAG_CARRY()){
        return;
    }

    cpu_registers.pc = address;
    core_advance_cpu_clocks(4);
}

// 0xDC
void cpu_call_c(){
    uint16_t address = cpu_condition_address();

    if (!GET_FLAG_CARRY()){
        return;
    }

    cpu_call_to(address);
}

// 0xDE
void cpu_sbc_a_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_sbc_a(value);
    core_advance_cpu_clocks(4);
}

// 0xDF
void cpu_rst_18(){
    cpu_rst_to(0x0018);
}

// 0xE0
void cpu_ldh_na(){
    core_advance_cpu_clocks(4);
    uint8_t offset = memory_bus_read(cpu_registers.pc++);

    core_advance_cpu_clocks(4);
    memory_bus_write((uint16_t)(0xFF00 + offset), cpu_registers.a);

    core_advance_cpu_clocks(4);
}

// 0xE1
void cpu_pop_hl(){
    cpu_registers.hl = cpu_routine_pop_16();
}

// 0xE2
void cpu_ld_c_io_a(){
    core_advance_cpu_clocks(4);
    memory_bus_write((uint16_t)(0xFF00 + cpu_registers.c), cpu_registers.a);
    core_advance_cpu_clocks(4);
}

// 0xE5
void cpu_push_hl(){
    cpu_routine_push_16(cpu_registers.hl);
}

// 0xE6
void cpu_and_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_and_a(value);
    core_advance_cpu_clocks(4);
}

// 0xE7
void cpu_rst_20(){
    cpu_rst_to(0x0020);
}

// 0xE8
void cpu_add_sp_r8(){
    core_advance_cpu_clocks(4);
    cpu_routine_add_sp_r8();
    core_advance_cpu_clocks(12);
}

// 0xE9
void cpu_jp_hl(){
    cpu_registers.pc = cpu_registers.hl;
    core_advance_cpu_clocks(4);
}

// 0xEA
void cpu_ld_nn_a(){
    cpu_routine_ld_mem_nn_a();
}

// 0xEE
void cpu_xor_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_xor_a(value);
    core_advance_cpu_clocks(4);
}

// 0xEF
void cpu_rst_28(){
    cpu_rst_to(0x0028);
}

// 0xF0
void cpu_ldh_a_n(){
    core_advance_cpu_clocks(4);
    uint8_t offset = memory_bus_read(cpu_registers.pc++);

    core_advance_cpu_clocks(4);
    cpu_registers.a = memory_bus_read((uint16_t)(0xFF00 + offset));

    core_advance_cpu_clocks(4);
}

// 0xF1
void cpu_pop_af(){
    cpu_registers.af = cpu_routine_pop_16();
    cpu_registers.f &= 0xF0;
}

// 0xF2
void cpu_ld_a_c_io(){
    core_advance_cpu_clocks(4);
    cpu_registers.a = memory_bus_read((uint16_t)(0xFF00 + cpu_registers.c));
    core_advance_cpu_clocks(4);
}

// 0xF3
void cpu_di(){
    cpu_ime = false;
    core_advance_cpu_clocks(4);
}

// 0xF5
void cpu_push_af(){
    cpu_routine_push_16((uint16_t)((cpu_registers.a << 8) | (cpu_registers.f & 0xF0)));
}

// 0xF6
void cpu_or_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_or_a(value);
    core_advance_cpu_clocks(4);
}

// 0xF7
void cpu_rst_30(){
    cpu_rst_to(0x0030);
}

// 0xF8
void cpu_ld_hl_sp_r8(){
    core_advance_cpu_clocks(4);

    int8_t value = (int8_t)memory_bus_read(cpu_registers.pc++);
    uint16_t old = cpu_registers.sp;

    uint16_t result = (uint16_t)(old + value);

    SET_FLAG_ZERO(0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(((old & 0x0F) + ((uint16_t)value & 0x0F)) > 0x0F);
    SET_FLAG_CARRY(((old & 0xFF) + ((uint16_t)value & 0xFF)) > 0xFF);

    cpu_registers.hl = result;

    core_advance_cpu_clocks(4);
    core_advance_cpu_clocks(4);
}

// 0xF9
void cpu_ld_sp_hl(){
    cpu_registers.sp = cpu_registers.hl;
    core_advance_cpu_clocks(8);
}

// 0xFA
void cpu_ld_a_nn(){
    uint16_t address = cpu_routine_read_nn();

    cpu_registers.a = memory_bus_read(address);
    core_advance_cpu_clocks(4);
}

// 0xFB
void cpu_ei(){
    cpu_ime = true;
    core_advance_cpu_clocks(4);
}

// 0xFE
void cpu_cp_n(){
    core_advance_cpu_clocks(4);
    uint8_t value = memory_bus_read(cpu_registers.pc++);

    cpu_routine_cp_a(value);
    core_advance_cpu_clocks(4);
}

// 0xFF
void cpu_rst_38(){
    cpu_rst_to(0x0038);
}


// CB-prefixed operations.
// cpu_prefix_cb accounts for the CB-prefix fetch. This function accounts
// for the second opcode fetch/operation and decodes all 256 CB opcodes.
void cpu_cb_execute(){
    uint8_t opcode = cpu_current_cb_op_code;
    uint8_t group = opcode >> 6;
    uint8_t bit_or_group = (opcode >> 3) & 7;
    uint8_t target = opcode & 7;

    if (target == 6){
        uint8_t value = memory_bus_read(cpu_registers.hl);

        switch (group){
            case 0:
                switch (bit_or_group){
                    case 0: cpu_routine_cb_rlc(value); break;
                    case 1: cpu_routine_cb_rrc(value); break;
                    case 2: cpu_routine_cb_rl(value); break;
                    case 3: cpu_routine_cb_rr(value); break;
                    case 4: cpu_routine_cb_sla(value); break;
                    case 5: cpu_routine_cb_sra(value); break;
                    case 6: cpu_routine_cb_swap(value); break;
                    case 7: cpu_routine_cb_srl(value); break;
                }

                memory_bus_write(cpu_registers.hl, value);
                break;

            case 1:
                cpu_routine_cb_bit(value, bit_or_group);
                break;

            case 2:
                cpu_routine_cb_res(value, bit_or_group);
                memory_bus_write(cpu_registers.hl, value);
                break;

            case 3:
                cpu_routine_cb_set(value, bit_or_group);
                memory_bus_write(cpu_registers.hl, value);
                break;
        }

        core_advance_cpu_clocks(8);
        return;
    }

    uint8_t &reg = cpu_get_register_8(target);

    switch (group){
        case 0:
            switch (bit_or_group){
                case 0: cpu_routine_cb_rlc(reg); break;
                case 1: cpu_routine_cb_rrc(reg); break;
                case 2: cpu_routine_cb_rl(reg); break;
                case 3: cpu_routine_cb_rr(reg); break;
                case 4: cpu_routine_cb_sla(reg); break;
                case 5: cpu_routine_cb_sra(reg); break;
                case 6: cpu_routine_cb_swap(reg); break;
                case 7: cpu_routine_cb_srl(reg); break;
            }
            break;

        case 1:
            cpu_routine_cb_bit(reg, bit_or_group);
            break;

        case 2:
            cpu_routine_cb_res(reg, bit_or_group);
            break;

        case 3:
            cpu_routine_cb_set(reg, bit_or_group);
            break;
    }

    core_advance_cpu_clocks(4);
}
