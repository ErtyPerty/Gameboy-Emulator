#pragma once

#include "cpu.h"
#include "memory_bus.h"
#include "emulator_core.h"

static inline void cpu_routine_ld_8(uint8_t &reg){
    core_advance_cpu_clocks(4);
    reg = memory_bus_read(cpu_registers.pc++);
    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_ld_16(uint8_t &reg_hi, uint8_t &reg_low){
    core_advance_cpu_clocks(4);
    reg_low = memory_bus_read(cpu_registers.pc++);
    core_advance_cpu_clocks(4);
    reg_hi = memory_bus_read(cpu_registers.pc++);
    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_inc_8(uint8_t &reg){
    uint8_t old = reg;
    reg++;

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY((old & 0x0F) == 0x0F);

    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_dec_8(uint8_t &reg){
    uint8_t old = reg;
    reg--;

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(1);
    SET_FLAG_HALF_CARRY((old & 0x0F) == 0x00);

    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_inc_16(uint16_t &reg){
    reg++;
    core_advance_cpu_clocks(8);
}

static inline void cpu_routine_dec_16(uint16_t &reg){
    reg--;
    core_advance_cpu_clocks(8);
}

static inline void cpu_routine_add_hl(uint16_t value){
    uint16_t old = cpu_registers.hl;
    uint32_t result = (uint32_t)old + value;

    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(((old & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF);
    SET_FLAG_CARRY(result > 0xFFFF);

    cpu_registers.hl = (uint16_t)result;

    core_advance_cpu_clocks(8);
}

static inline void cpu_routine_add_a(uint8_t value){
    uint8_t old = cpu_registers.a;
    uint16_t result = (uint16_t)old + value;

    cpu_registers.a = (uint8_t)result;

    SET_FLAG_ZERO(cpu_registers.a == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(((old & 0x0F) + (value & 0x0F)) > 0x0F);
    SET_FLAG_CARRY(result > 0xFF);
}

static inline void cpu_routine_adc_a(uint8_t value){
    uint8_t old = cpu_registers.a;
    uint8_t carry = GET_FLAG_CARRY();
    uint16_t result = (uint16_t)old + value + carry;

    cpu_registers.a = (uint8_t)result;

    SET_FLAG_ZERO(cpu_registers.a == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(((old & 0x0F) + (value & 0x0F) + carry) > 0x0F);
    SET_FLAG_CARRY(result > 0xFF);
}

static inline void cpu_routine_sub_a(uint8_t value){
    uint8_t old = cpu_registers.a;

    cpu_registers.a = old - value;

    SET_FLAG_ZERO(cpu_registers.a == 0);
    SET_FLAG_SUBTRACT(1);
    SET_FLAG_HALF_CARRY((old & 0x0F) < (value & 0x0F));
    SET_FLAG_CARRY(old < value);
}

static inline void cpu_routine_sbc_a(uint8_t value){
    uint8_t old = cpu_registers.a;
    uint8_t carry = GET_FLAG_CARRY();
    uint16_t amount = (uint16_t)value + carry;

    cpu_registers.a = old - amount;

    SET_FLAG_ZERO(cpu_registers.a == 0);
    SET_FLAG_SUBTRACT(1);
    SET_FLAG_HALF_CARRY((old & 0x0F) < ((value & 0x0F) + carry));
    SET_FLAG_CARRY(old < amount);
}

static inline void cpu_routine_and_a(uint8_t value){
    cpu_registers.a &= value;

    SET_FLAG_ZERO(cpu_registers.a == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(1);
    SET_FLAG_CARRY(0);
}

static inline void cpu_routine_xor_a(uint8_t value){
    cpu_registers.a ^= value;

    SET_FLAG_ZERO(cpu_registers.a == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(0);
}

static inline void cpu_routine_or_a(uint8_t value){
    cpu_registers.a |= value;

    SET_FLAG_ZERO(cpu_registers.a == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(0);
}

static inline void cpu_routine_cp_a(uint8_t value){
    uint8_t old = cpu_registers.a;

    SET_FLAG_ZERO(old == value);
    SET_FLAG_SUBTRACT(1);
    SET_FLAG_HALF_CARRY((old & 0x0F) < (value & 0x0F));
    SET_FLAG_CARRY(old < value);
}

static inline void cpu_routine_rlca(){
    uint8_t carry = (cpu_registers.a >> 7) & 1;

    cpu_registers.a = (uint8_t)((cpu_registers.a << 1) | carry);

    SET_FLAG_ZERO(0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);

    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_rrca(){
    uint8_t carry = cpu_registers.a & 1;

    cpu_registers.a = (uint8_t)((cpu_registers.a >> 1) | (carry << 7));

    SET_FLAG_ZERO(0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);

    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_rla(){
    uint8_t old_carry = GET_FLAG_CARRY();
    uint8_t carry = (cpu_registers.a >> 7) & 1;

    cpu_registers.a = (uint8_t)((cpu_registers.a << 1) | old_carry);

    SET_FLAG_ZERO(0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);

    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_rra(){
    uint8_t old_carry = GET_FLAG_CARRY();
    uint8_t carry = cpu_registers.a & 1;

    cpu_registers.a = (uint8_t)((cpu_registers.a >> 1) | (old_carry << 7));

    SET_FLAG_ZERO(0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);

    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_daa(){
    uint8_t a = cpu_registers.a;
    uint8_t correction = 0;
    uint8_t carry = GET_FLAG_CARRY();

    if (!GET_FLAG_SUBTRACT()){
        if (carry || a > 0x99){
            correction |= 0x60;
            carry = 1;
        }

        if (GET_FLAG_HALF_CARRY() || (a & 0x0F) > 0x09){
            correction |= 0x06;
        }

        a += correction;
    }
    else{
        if (carry){
            a -= 0x60;
        }

        if (GET_FLAG_HALF_CARRY()){
            a -= 0x06;
        }
    }

    cpu_registers.a = a;

    SET_FLAG_ZERO(a == 0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);

    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_add_sp_r8(){
    uint16_t old = cpu_registers.sp;
    int8_t value = (int8_t)memory_bus_read(cpu_registers.pc++);

    uint16_t result = (uint16_t)(old + value);

    SET_FLAG_ZERO(0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(((old & 0x0F) + ((uint16_t)value & 0x0F)) > 0x0F);
    SET_FLAG_CARRY(((old & 0xFF) + ((uint16_t)value & 0xFF)) > 0xFF);

    cpu_registers.sp = result;
}

static inline void cpu_routine_push_16(uint16_t value){
    core_advance_cpu_clocks(4);
    core_advance_cpu_clocks(4);

    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, (uint8_t)(value >> 8));

    core_advance_cpu_clocks(4);

    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, (uint8_t)(value & 0xFF));

    core_advance_cpu_clocks(4);
}

static inline uint16_t cpu_routine_pop_16(){
    core_advance_cpu_clocks(4);
    uint8_t low = memory_bus_read(cpu_registers.sp++);

    core_advance_cpu_clocks(4);
    uint8_t high = memory_bus_read(cpu_registers.sp++);

    core_advance_cpu_clocks(4);

    return (uint16_t)(low | ((uint16_t)high << 8));
}

static inline uint16_t cpu_routine_read_nn(){
    core_advance_cpu_clocks(4);
    uint8_t low = memory_bus_read(cpu_registers.pc++);

    core_advance_cpu_clocks(4);
    uint8_t high = memory_bus_read(cpu_registers.pc++);

    core_advance_cpu_clocks(4);

    return (uint16_t)(low | ((uint16_t)high << 8));
}

static inline void cpu_routine_ld_mem_nn_a(){
    uint16_t address = cpu_routine_read_nn();

    memory_bus_write(address, cpu_registers.a);
    core_advance_cpu_clocks(4);
}

static inline void cpu_routine_ld_nn_sp(){
    uint16_t address = cpu_routine_read_nn();

    core_advance_cpu_clocks(4);
    memory_bus_write(address, cpu_registers.p);

    core_advance_cpu_clocks(4);
    memory_bus_write(address + 1, cpu_registers.s);
}

static inline void cpu_routine_cb_rlc(uint8_t &reg){
    uint8_t carry = (reg >> 7) & 1;
    reg = (uint8_t)((reg << 1) | carry);

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);
}

static inline void cpu_routine_cb_rrc(uint8_t &reg){
    uint8_t carry = reg & 1;
    reg = (uint8_t)((reg >> 1) | (carry << 7));

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);
}

static inline void cpu_routine_cb_rl(uint8_t &reg){
    uint8_t old_carry = GET_FLAG_CARRY();
    uint8_t carry = (reg >> 7) & 1;

    reg = (uint8_t)((reg << 1) | old_carry);

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);
}

static inline void cpu_routine_cb_rr(uint8_t &reg){
    uint8_t old_carry = GET_FLAG_CARRY();
    uint8_t carry = reg & 1;

    reg = (uint8_t)((reg >> 1) | (old_carry << 7));

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);
}

static inline void cpu_routine_cb_sla(uint8_t &reg){
    uint8_t carry = (reg >> 7) & 1;

    reg = (uint8_t)(reg << 1);

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);
}

static inline void cpu_routine_cb_sra(uint8_t &reg){
    uint8_t carry = reg & 1;
    uint8_t bit7 = reg & 0x80;

    reg = (uint8_t)((reg >> 1) | bit7);

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);
}

static inline void cpu_routine_cb_swap(uint8_t &reg){
    reg = (uint8_t)((reg << 4) | (reg >> 4));

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(0);
}

static inline void cpu_routine_cb_srl(uint8_t &reg){
    uint8_t carry = reg & 1;

    reg >>= 1;

    SET_FLAG_ZERO(reg == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(0);
    SET_FLAG_CARRY(carry);
}

static inline void cpu_routine_cb_bit(uint8_t value, uint8_t bit){
    SET_FLAG_ZERO((value & (1u << bit)) == 0);
    SET_FLAG_SUBTRACT(0);
    SET_FLAG_HALF_CARRY(1);
}

static inline void cpu_routine_cb_res(uint8_t &reg, uint8_t bit){
    reg &= (uint8_t)~(1u << bit);
}

static inline void cpu_routine_cb_set(uint8_t &reg, uint8_t bit){
    reg |= (uint8_t)(1u << bit);
}
