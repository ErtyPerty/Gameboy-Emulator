#include "serial_test.h"

#include <iostream>
#include <string>

static uint8_t serial_data = 0x00;
static uint8_t serial_control = 0x00;

static std::string serial_output;

static bool test_passed = false;
static bool test_failed = false;
static bool test_finished = false;


void serial_test_reset(){
	serial_data = 0x00;
	serial_control = 0x00;

	serial_output.clear();

	test_passed = false;
	test_failed = false;
	test_finished = false;
}


void serial_test_write_data(uint8_t value){
	serial_data = value;
}


void serial_test_write_control(uint8_t value){
	serial_control = value;

	/*
	 * 0x81:
	 *
	 * Bit 7 = Start transfer
	 * Bit 0 = Internal clock
	 *
	 * This is the pattern commonly used by test ROMs
	 * to transmit one byte through the serial port.
	 */
	if (value == 0x81){
		char character = static_cast<char>(serial_data);

		serial_output += character;

		std::cout << character;
		std::cout.flush();

		/*
		 * Look for the normal pass/fail strings used by
		 * many diagnostic ROMs.
		 */
		if (serial_output.find("Passed") != std::string::npos){
			test_passed = true;
			test_finished = true;
		}

		if (serial_output.find("Failed") != std::string::npos){
			test_failed = true;
			test_finished = true;
		}

		/*
		 * Keep transfer complete immediately.
		 */
		serial_control &= 0x7F;
	}
}


uint8_t serial_test_read_data(){
	return serial_data;
}


uint8_t serial_test_read_control(){
	return serial_control;
}


bool serial_test_passed(){
	return test_passed;
}


bool serial_test_failed(){
	return test_failed;
}


bool serial_test_finished(){
	return test_finished;
}