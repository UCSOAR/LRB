/**
 ******************************************************************************
 * @file           : SystemDefines.hpp
 * @brief          : Macros and wrappers
 ******************************************************************************
 *
 * Contains system wide macros, defines, and wrappers
 *
 ******************************************************************************
 */
#ifndef CUBE_MAIN_SYSTEM_DEFINES_H
#define CUBE_MAIN_SYSTEM_DEFINES_H

/* Environment Defines ------------------------------------------------------------------*/
// #define COMPUTER_ENVIRONMENT        // Define this if we're in Windows, Linux or Mac (not when flashing on DMB)

#ifdef COMPUTER_ENVIRONMENT
#define __CC_ARM
#endif

/* System Wide Includes ------------------------------------------------------------------*/
#include "main_avionics.hpp" // C++ Main File Header
#include "UARTDriver.hpp"

/* Cube++ Required Configuration ------------------------------------------------------------------*/
#include "CubeDefines.hpp"
constexpr UARTDriver *const DEFAULT_DEBUG_UART_DRIVER = UART::Debug; // UART Handle that ASSERT messages are sent over
enum GLOBAL_COMMANDS : uint8_t
{
  COMMAND_NONE = 0,      // No command, packet can probably be ignored
  TASK_SPECIFIC_COMMAND, // Runs a task specific command when given this object
  DATA_COMMAND,          // Data command, used to send data to a task. Target is stored in taskCommand
};

/* Cube++ Optional Code Configuration ------------------------------------------------------------------*/

/* Task Parameter Definitions ------------------------------------------------------------------*/
/* - Lower priority number means lower priority task ---------------------------------*/

// UART TASK
constexpr uint8_t UART_TASK_RTOS_PRIORITY = 2;        // Priority of the uart task
constexpr uint8_t UART_TASK_QUEUE_DEPTH_OBJS = 10;    // Size of the uart task queue
constexpr uint16_t UART_TASK_STACK_DEPTH_WORDS = 512; // Size of the uart task stack

// DEBUG TASK
constexpr uint8_t TASK_DEBUG_PRIORITY = 2;             // Priority of the debug task
constexpr uint8_t TASK_DEBUG_QUEUE_DEPTH_OBJS = 10;    // Size of the debug task queue
constexpr uint16_t TASK_DEBUG_STACK_DEPTH_WORDS = 512; // Size of the debug task stack

// FILESYSTEM TASK
constexpr uint8_t TASK_FILESYSTEM_TASK_PRIORITY = 3;         // Priority of the filesystem task
constexpr uint8_t TASK_FILESYSTEM_QUEUE_DEPTH_OBJS = 8;      // Size of the filesystem task queue
constexpr uint16_t TASK_FILESYSTEM_STACK_DEPTH_WORDS = 1024; // Size of the filesystem task stack
constexpr uint32_t FILESYSTEM_TASK_QUEUE_TIMEOUT_MS = 100;   // Queue timeout for filesystem task
constexpr uint32_t FILESYSTEM_TASK_LOOP_DELAY_MS = 1000;     // Main loop delay for filesystem task

// TASK 1
constexpr uint8_t TASK1_RTOS_PRIORITY = 2;        // Priority of Task 1
constexpr uint8_t TASK1_QUEUE_DEPTH_OBJS = 10;    // Size of Task 1 queue
constexpr uint16_t TASK1_STACK_DEPTH_WORDS = 512; // Size of Task 1 stack

// TASK 2
constexpr uint8_t TASK2_RTOS_PRIORITY = 2;        // Priority of Task 2
constexpr uint8_t TASK2_QUEUE_DEPTH_OBJS = 10;    // Size of Task 2 queue
constexpr uint16_t TASK2_STACK_DEPTH_WORDS = 512; // Size of Task 2 stack

// TASK 3
constexpr uint8_t TASK3_RTOS_PRIORITY = 2;        // Priority of Task 3
constexpr uint8_t TASK3_QUEUE_DEPTH_OBJS = 10;    // Size of Task 3 queue
constexpr uint16_t TASK3_STACK_DEPTH_WORDS = 512; // Size of Task 3 stack

// TASK 4
constexpr uint8_t TASK4_RTOS_PRIORITY = 2;        // Priority of Task 4
constexpr uint8_t TASK4_QUEUE_DEPTH_OBJS = 10;    // Size of Task 4 queue
constexpr uint16_t TASK4_STACK_DEPTH_WORDS = 512; // Size of Task 4 stack

#endif // CUBE_MAIN_SYSTEM_DEFINES_H
