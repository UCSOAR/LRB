/**
 ******************************************************************************
 * File Name          : DebugTask.cpp
 * Description        : Task for controlling debug input
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <NAU7802Task.hpp>
#include <SoarDebug/Inc/DebugTask.hpp>
#include "Command.hpp"
#include "CubeUtils.hpp"
#include <cctype>
#include <cstring>

#include "stm32g4xx_hal.h"

// External Tasks (to send debug commands to)

/* Macros --------------------------------------------------------------------*/

/* Structs -------------------------------------------------------------------*/

/* Constants -----------------------------------------------------------------*/
constexpr uint8_t DEBUG_TASK_PERIOD = 100;
// extern I2C_HandleTypeDef hi2c2;

/* Variables -----------------------------------------------------------------*/

/* Prototypes ----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
/**
 * @brief Constructor, sets all member variables
 */
DebugTask::DebugTask()
    : Task(TASK_DEBUG_QUEUE_DEPTH_OBJS), kUart_(UART::Debug)
{
  memset(debugBuffer, 0, sizeof(debugBuffer));
  debugMsgIdx = 0;
  isDebugMsgReady = false;
}

/**
 * @brief Init task for RTOS
 */
void DebugTask::InitTask()
{
  // Make sure the task is not already initialized
  SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize Debug task twice");

  // Start the task
  BaseType_t rtValue = xTaskCreate(
      (TaskFunction_t)DebugTask::RunTask, (const char *)"DebugTask",
      (uint16_t)TASK_DEBUG_STACK_DEPTH_WORDS, (void *)this,
      (UBaseType_t)TASK_DEBUG_PRIORITY, (TaskHandle_t *)&rtTaskHandle);

  // Ensure creation succeded
  SOAR_ASSERT(rtValue == pdPASS, "DebugTask::InitTask - xTaskCreate() failed");
}

// TODO: Only run thread when appropriate GPIO pin pulled HIGH (or by define)
/**
 *    @brief Runcode for the DebugTask
 */
void DebugTask::Run(void *pvParams)
{
  // Arm the interrupt
  ReceiveData();

  while (1)
  {
    Command cm;

    // Wait forever for a command
    qEvtQueue->ReceiveWait(cm);

    // Process the command
    if (cm.GetCommand() == DATA_COMMAND &&
        cm.GetTaskCommand() == EVENT_DEBUG_RX_COMPLETE)
    {
      HandleDebugMessage((const char *)debugBuffer);
    }

    cm.Reset();
  }
}

/**
 * @brief Handles debug messages, assumes msg is null terminated
 * @param msg Message to read, must be null termianted
 */
void DebugTask::HandleDebugMessage(const char *msg)
{
  char cleanMsg[DEBUG_RX_BUFFER_SZ_BYTES + 1] = {0};

  // Trim leading/trailing whitespace so terminals with CR/LF both work.
  const char *start = msg;
  while (*start != '\0' && std::isspace(static_cast<unsigned char>(*start))) {
    ++start;
  }
  const char *end = start + strlen(start);
  while (end > start && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
    --end;
  }
  const size_t cleanLen = static_cast<size_t>(end - start);
  if (cleanLen > 0) {
    const size_t copyLen = (cleanLen > DEBUG_RX_BUFFER_SZ_BYTES) ? DEBUG_RX_BUFFER_SZ_BYTES : cleanLen;
    memcpy(cleanMsg, start, copyLen);
    cleanMsg[copyLen] = '\0';
  }

  char lowerMsg[DEBUG_RX_BUFFER_SZ_BYTES + 1] = {0};
  for (size_t i = 0; i < strlen(cleanMsg); ++i) {
    lowerMsg[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(cleanMsg[i])));
  }


  // NAUinterface for peripheral task controls.
    if (strcmp(lowerMsg, "nau toggle") == 0 ||
        strcmp(lowerMsg, "nau on") == 0 ||
        strcmp(lowerMsg, "nau off") == 0 ||
        strcmp(lowerMsg, "nau status") == 0)
    {
      Command cmd(DATA_COMMAND, TASK1_COMMAND_NAU_STATUS);
      if (strcmp(lowerMsg, "nau toggle") == 0) {
        cmd.SetTaskCommand(TASK1_COMMAND_NAU_TOGGLE);
      } else if (strcmp(lowerMsg, "nau on") == 0) {
        cmd.SetTaskCommand(TASK1_COMMAND_NAU_ON);
      } else if (strcmp(lowerMsg, "nau off") == 0) {
        cmd.SetTaskCommand(TASK1_COMMAND_NAU_OFF);
      }
        else{
    	    SOAR_PRINT("Debug: Unsupported command: %s\n", cleanMsg);
    	    SOAR_PRINT("Debug: Use  nau [on|off|toggle|status]\n");
    	    debugMsgIdx = 0;
    	    isDebugMsgReady = false;
    	    return;
      }
      NAU7802Task::Inst().GetEventQueue()->Send(cmd);
      SOAR_PRINT("Debug: Sent %s\n", cleanMsg);
      debugMsgIdx = 0;
      isDebugMsgReady = false;
      return;
    }

  //-- FILESYSTEM COMMANDS --
  if (strcmp(lowerMsg, "fs_test") == 0)
  {
    SOAR_PRINT("Debug: Triggering file system tests\n");
  }
  else if (strcmp(lowerMsg, "fs_log") == 0)
  {
    SOAR_PRINT("Debug: Triggering sensor data logging\n");
    // Sample data for testing
    float temp = 25.5f + (HAL_GetTick() % 100) / 10.0f;     // Simulate varying temperature
    float humidity = 60.0f + (HAL_GetTick() % 200) / 10.0f; // Simulate varying humidity
  }
  else if (strcmp(lowerMsg, "fs_cleanup") == 0)
  {
    SOAR_PRINT("Debug: Triggering file system cleanup\n");
  }
  //-- SYSTEM / CHAR COMMANDS -- (Must be last)
  else if (strcmp(lowerMsg, "sysreset") == 0)
  {
    // Reset the system
    SOAR_ASSERT(false, "System reset requested");
  }
  else if (strcmp(lowerMsg, "sysinfo") == 0)
  {
    // Print message
    SOAR_PRINT("\n\n-- CUBE SYSTEM --\n");
    SOAR_PRINT("Current System Free Heap: %d Bytes\n", xPortGetFreeHeapSize());
    SOAR_PRINT("Lowest Ever Free Heap: %d Bytes\n",
               xPortGetMinimumEverFreeHeapSize());
    SOAR_PRINT("Debug Task Runtime  \t: %d ms\n\n",
               TICKS_TO_MS(xTaskGetTickCount()));
  }
  else
  {
    // Single character command, or unknown command
    switch (lowerMsg[0])
    {
    case 'h':
      SOAR_PRINT("\n-- DEBUG COMMANDS --\n");
      SOAR_PRINT("sysinfo  - System information\n");
      SOAR_PRINT("sysreset - System reset\n");
      SOAR_PRINT("NAU [on|off|toggle|status] - NAU7802 controls\n");
      SOAR_PRINT("fs_test  - Run file system tests\n");
      SOAR_PRINT("fs_log   - Log sample sensor data\n");
      SOAR_PRINT("fs_cleanup - Run file system cleanup\n");
      SOAR_PRINT("h        - Show this help\n\n");
      break;
    default:
      SOAR_PRINT("Debug, unknown command: %s (type 'h' for help)\n", cleanMsg);
      break;
    }
  }

  // We've read the data, clear the buffer
  debugMsgIdx = 0;
  isDebugMsgReady = false;
}

/**
 * @brief Receive data, currently receives by arming interrupt
 */
bool DebugTask::ReceiveData() { return kUart_->ReceiveIT(&debugRxChar, this); }

/**
 * @brief Receive data to the buffer
 * @return Whether the debugBuffer is ready or not
 */
void DebugTask::InterruptRxData(uint8_t errors)
{
  // If we already have an unprocessed debug message, ignore this byte
  if (!isDebugMsgReady)
  {
    // Check byte for end of message - note if using termite you must turn on
    // append CR
    if (debugRxChar == '\r' || debugRxChar == '\n' || debugMsgIdx >= DEBUG_RX_BUFFER_SZ_BYTES)
    {
      // Null terminate and process
      debugBuffer[debugMsgIdx++] = '\0';
      isDebugMsgReady = true;

      // Notify the debug task
      Command cm(DATA_COMMAND, EVENT_DEBUG_RX_COMPLETE);
      bool res = qEvtQueue->SendFromISR(cm);

      // If we failed to send the event, we should reset the buffer, that way
      // DebugTask doesn't stall
      if (res == false)
      {
        debugMsgIdx = 0;
        isDebugMsgReady = false;
      }
    }
    else
    {
      debugBuffer[debugMsgIdx++] = debugRxChar;
    }
  }

  // Re-arm the interrupt
  ReceiveData();
}

/* Helper Functions
 * --------------------------------------------------------------*/
/**
 * @brief Extracts an integer parameter from a string
 * @brief msg Message to extract from, MUST be at least identifierLen long, and
 * properly null terminated
 * @brief identifierLen Length of the identifier eg. 'rsc ' (Including the
 * space) is 4
 * @return ERRVAL on failure, otherwise the extracted value
 */
int32_t DebugTask::ExtractIntParameter(const char *msg,
                                       uint16_t identifierLen)
{
  // Handle a command with an int parameter at the end
  if (static_cast<uint16_t>(strlen(msg)) < identifierLen + 1)
  {
    SOAR_PRINT("Int parameter command insufficient length\r\n");
    return ERRVAL;
  }

  // Extract the value and attempt conversion to integer
  const int32_t val = Utils::StringToLong(&msg[identifierLen]);
  if (val == ERRVAL)
  {
    SOAR_PRINT("Int parameter command invalid value\r\n");
  }

  return val;
}
