/*
 * RunInterface.hpp
 *
 *  Created on: Apr 3, 2023
 *      Author: Chris (cjchanx)
 */

#ifndef C__IFACE_HPP_
#define C__IFACE_HPP_

//#include "UARTDriver.hpp"

/**
 * @brief Interface to run the C++ codebase in the C compiler
 */
#ifdef __cplusplus
extern "C" {
#endif
void run_interface();
#ifdef __cplusplus
}
#endif

/**
 * @brief Interrupt Routing for the UART driver
 */
#ifdef __cplusplus
extern "C" {
#endif
void cpp_USART2_IRQHandler();
#ifdef __cplusplus
}
#endif

#endif /* C__IFACE_HPP_ */
