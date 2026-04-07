/*
 *  RunInterface.cpp
 *
 *  Created on: Apr 3, 2023
 *      Author: Chris (cjchanx)
 */

#include "../../SoarOS/Drivers/Inc/UARTDriver.hpp"
#include "main_avionics.hpp"

#include "RunInterface.hpp"

extern "C" {
void run_interface() { run_main(); }

void cpp_USART2_IRQHandler() {
		Driver::usart2.HandleIRQ_UART();
	}
}
