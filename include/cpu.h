#pragma once

#include <stdint.h>

#define SET_FLAG_ZERO(value) \
    do { \
        cpu_registers.f = (cpu_registers.f & 0x7F) | (((value) & 1) << 7); \
    } while (0)

#define GET_FLAG_ZERO() \
    ((cpu_registers.f >> 7) & 1)

#define SET_FLAG_SUBTRACT(value) \
    do { \
        cpu_registers.f = (cpu_registers.f & 0xBF) | (((value) & 1) << 6); \
    } while (0)

#define GET_FLAG_SUBTRACT() \
    ((cpu_registers.f >> 6) & 1)

#define SET_FLAG_HALF_CARRY(value) \
    do { \
        cpu_registers.f = (cpu_registers.f & 0xDF) | (((value) & 1) << 5); \
    } while (0)

#define GET_FLAG_HALF_CARRY() \
    ((cpu_registers.f >> 5) & 1)

#define SET_FLAG_CARRY(value) \
    do { \
        cpu_registers.f = (cpu_registers.f & 0xEF) | (((value) & 1) << 4); \
    } while (0)

#define GET_FLAG_CARRY() \
    ((cpu_registers.f >> 4) & 1)


struct gb_cpu_registers{
    struct{
        union {
            struct {
                uint8_t f;
                uint8_t a;
            };

            uint16_t af;
        };
    };

    struct{
        union {
            struct {
                uint8_t c;
                uint8_t b;
            };

            uint16_t bc;
        };
    };

    struct{
        union {
            struct {
                uint8_t e;
                uint8_t d;
            };

            uint16_t de;
        };
    };

    struct{
        union {
            struct {
                uint8_t l;
                uint8_t h;
            };

            uint16_t hl;
        };
    };

    struct{
        union {
            struct {
                uint8_t p;
                uint8_t s;
            };

            uint16_t sp;
        };
    };

    uint16_t pc;
};


typedef void(*cpu_execute_op)();

extern gb_cpu_registers cpu_registers;

extern bool cpu_halted;
extern bool cpu_stopped;
extern bool cpu_ime;

void cpu_fetch();
bool cpu_execute();
void cpu_reset();

void cpu_noop();
void cpu_ld_bc_nn();
void cpu_ld_bc_a();
void cpu_inc_bc();
void cpu_inc_b();
void cpu_dec_b();
void cpu_ld_b_n();
void cpu_rlca();
void cpu_ld_nn_sp();
void cpu_add_hl_bc();
void cpu_ld_a_bc();
void cpu_dec_bc();
void cpu_inc_c();
void cpu_dec_c();
void cpu_ld_c_n();
void cpu_rrca();

void cpu_stop();
void cpu_ld_de_nn();
void cpu_ld_de_a();
void cpu_inc_de();
void cpu_inc_d();
void cpu_dec_d();
void cpu_ld_d_n();
void cpu_rla();
void cpu_jr();
void cpu_add_hl_de();
void cpu_ld_a_de();
void cpu_dec_de();
void cpu_inc_e();
void cpu_dec_e();
void cpu_ld_e_n();
void cpu_rra();

void cpu_jr_nz();
void cpu_ld_hl_nn();
void cpu_ld_hli_a();
void cpu_inc_hl();
void cpu_inc_h();
void cpu_dec_h();
void cpu_ld_h_n();
void cpu_daa();
void cpu_jr_z();
void cpu_add_hl_hl();
void cpu_ld_a_hli();
void cpu_dec_hl();
void cpu_inc_l();
void cpu_dec_l();
void cpu_ld_l_n();
void cpu_cpl();

void cpu_jr_nc();
void cpu_ld_sp_nn();
void cpu_ldd_hl_a();
void cpu_inc_sp();
void cpu_inc_hl_mem();
void cpu_dec_hl_mem();
void cpu_ld_hl_n();
void cpu_scf();
void cpu_jr_c();
void cpu_add_hl_sp();
void cpu_ld_a_hld();
void cpu_dec_sp();
void cpu_inc_a();
void cpu_dec_a();
void cpu_ld_a_n();
void cpu_ccf();

void cpu_ld_r_r();
void cpu_halt();

void cpu_alu_r();

void cpu_ret_nz();
void cpu_pop_bc();
void cpu_jp_nz();
void cpu_jp_nn();
void cpu_call_nz();
void cpu_push_bc();
void cpu_add_a_n();
void cpu_rst_00();
void cpu_ret_z();
void cpu_ret();
void cpu_jp_z();
void cpu_prefix_cb();
void cpu_call_z();
void cpu_call_nn();
void cpu_adc_a_n();
void cpu_rst_08();

void cpu_ret_nc();
void cpu_pop_de();
void cpu_jp_nc();
void cpu_call_nc();
void cpu_push_de();
void cpu_sub_n();
void cpu_rst_10();
void cpu_ret_c();
void cpu_reti();
void cpu_jp_c();
void cpu_call_c();
void cpu_sbc_a_n();
void cpu_rst_18();

void cpu_ldh_na();
void cpu_pop_hl();
void cpu_ld_c_io_a();
void cpu_push_hl();
void cpu_and_n();
void cpu_rst_20();
void cpu_add_sp_r8();
void cpu_jp_hl();
void cpu_ld_nn_a();
void cpu_xor_n();
void cpu_rst_28();

void cpu_ldh_a_n();
void cpu_pop_af();
void cpu_ld_a_c_io();
void cpu_di();
void cpu_push_af();
void cpu_or_n();
void cpu_rst_30();
void cpu_ld_hl_sp_r8();
void cpu_ld_sp_hl();
void cpu_ld_a_nn();
void cpu_ei();
void cpu_cp_n();
void cpu_rst_38();

void cpu_cb_execute();
