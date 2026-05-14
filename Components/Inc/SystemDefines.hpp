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
#include "../../SoarOS/Drivers/Inc/UARTDriver.hpp"

/* Cube++ Required Configuration ------------------------------------------------------------------*/
#include "../../SoarOS/CubeDefines.hpp"
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
constexpr bool DEBUG_ROUTE_TO_FSB_DEFAULT = false;    // Default route for SOAR_PRINT/assert output (false = USART2, true = USART3/FSB)

// DEBUG TASK
constexpr uint8_t TASK_DEBUG_PRIORITY = 2;             // Priority of the debug task
constexpr uint8_t TASK_DEBUG_QUEUE_DEPTH_OBJS = 10;    // Size of the debug task queue
constexpr uint16_t TASK_DEBUG_STACK_DEPTH_WORDS = 512; // Size of the debug task stack

// FILESYSTEM (virus) TASK
// constexpr uint8_t TASK_FILESYSTEM_TASK_PRIORITY = 3;         // Priority of the filesystem task
// constexpr uint8_t TASK_FILESYSTEM_QUEUE_DEPTH_OBJS = 8;      // Size of the filesystem task queue
// constexpr uint16_t TASK_FILESYSTEM_STACK_DEPTH_WORDS = 1024; // Size of the filesystem task stack
// constexpr uint32_t FILESYSTEM_TASK_QUEUE_TIMEOUT_MS = 100;   // Queue timeout for filesystem task
// constexpr uint32_t FILESYSTEM_TASK_LOOP_DELAY_MS = 1000;     // Main loop delay for filesystem task

// TODO: PROFILER 4
// Profiler task
constexpr uint8_t TASK_PROFILER_PRIORITY = 3;  // Priority of the profiler task
constexpr uint8_t TASK_PROFILER_QUEUE_DEPTH_OBJS =10;  // Size of the profiler task queue
constexpr uint16_t TASK_PROFILER_STACK_DEPTH_WORDS =512;  // Size of the profiler task stack

// TOP TASK
constexpr uint8_t TASK_TOP_PRIORITY = 3;  // Priority of the top task
constexpr uint8_t TASK_TOP_QUEUE_DEPTH_OBJS = 10;  // Size of the top task queue
constexpr uint16_t TASK_TOP_STACK_DEPTH_WORDS = 512;  // Size of the top task stack

// ANEM TASK
constexpr uint8_t ANEM_TASK_RTOS_PRIORITY = 2;        // Priority of ANEM TASK
constexpr uint8_t ANEM_TASK_QUEUE_DEPTH_OBJS = 10;    // Size of ANEM TASK queue
constexpr uint16_t ANEM_TASK_STACK_DEPTH_WORDS = 512; // Size of ANEM TASK stack

// LED TASK
constexpr uint8_t LED_TASK_RTOS_PRIORITY = 2;        // Priority of Task 2
constexpr uint8_t LED_TASK_QUEUE_DEPTH_OBJS = 10;    // Size of Task 2 queue
constexpr uint16_t LED_TASK_STACK_DEPTH_WORDS = 512; // Size of Task 2 stack

// BUZZER TASK
constexpr uint8_t TASK_BUZZER_RTOS_PRIORITY = 2;        // Priority of the buzzer task
constexpr uint8_t TASK_BUZZER_QUEUE_DEPTH_OBJS = 10;    // Size of the buzzer task queue
constexpr uint16_t TASK_BUZZER_STACK_DEPTH_WORDS = 512; // Size of the buzzer task stack

// MAX TASK
constexpr uint8_t MAX_TASK_RTOS_PRIORITY = 2;        // Priority of Task 2
constexpr uint8_t MAX_TASK_QUEUE_DEPTH_OBJS = 10;    // Size of Task 2 queue
constexpr uint16_t MAX_TASK_STACK_DEPTH_WORDS = 1024; // Size of Task 2 stack

// NAU TASK
constexpr uint8_t NAU_TASK_RTOS_PRIORITY = 2;        // Priority of Task 2
constexpr uint8_t NAU_TASK_QUEUE_DEPTH_OBJS = 10;    // Size of Task 2 queue
constexpr uint16_t NAU_TASK_STACK_DEPTH_WORDS = 512; // Size of Task 2 stack

#endif // CUBE_MAIN_SYSTEM_DEFINES_H
