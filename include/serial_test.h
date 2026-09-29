#pragma once

#include <stdint.h>

void serial_test_reset();

void serial_test_write_data(uint8_t value);
void serial_test_write_control(uint8_t value);

uint8_t serial_test_read_data();
uint8_t serial_test_read_control();

bool serial_test_passed();
bool serial_test_failed();
bool serial_test_finished();