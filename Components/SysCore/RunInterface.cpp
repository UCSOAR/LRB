/*
 *  RunInterface.cpp
 *
 *  Created on: Apr 3, 2023
 *      Author: Chris (cjchanx)
 */

#include "main_avionics.hpp"

#include "RunInterface.hpp"

#include "../../SoarOS/Drivers/Inc/UARTDriver.hpp"

extern "C" {
void run_interface() { run_main(); }

void cpp_USART2_IRQHandler() {
		Driver::usart2.HandleIRQ_UART();
	}
}
