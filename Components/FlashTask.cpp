/**
 ********************************************************************************
 * @file    FlashTask.cpp
 * @author  Christy
 * @date    Apr 25, 2026
 * @brief
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "FlashTask.hpp"
/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * VARIABLES
 ************************************/

/************************************
 * FUNCTION DECLARATIONS
 ************************************/

/************************************
 * FUNCTION DEFINITIONS
 ************************************/
void FlashTask::Run(void *pvParams)
{
  SOAR_PRINT("FlashTask::Run() - Starting task\n");

    InitializeFlash();

    while (1)
    {
        Command cm;
        bool res = qEvtQueue->ReceiveWait(cm);
        if (res)
        {
            HandleCommand(cm);
        }
    }
}


void FlashTask::InitTask()
{
  // Make sure the task is not already initialized
  SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize Flash task twice");

  // Start the task
  BaseType_t rtValue = xTaskCreate(
      (TaskFunction_t)FlashTask::RunTask, (const char *)"FlashTask",
      (uint16_t)TASK_FLASH_STACK_DEPTH_WORDS, (void *)this,
      (UBaseType_t)TASK_FLASH_PRIORITY, (TaskHandle_t *)&rtTaskHandle);

  // Ensure creation succeded
  SOAR_ASSERT(rtValue == pdPASS, "FlashTask::InitTask - xTaskCreate() failed");
}






void FlashTask::HandleCommand(Command &cm)
{
    if (cm.GetCommand() == TASK_SPECIFIC_COMMAND)
    {
        switch (cm.GetTaskCommand())
        {
        case EVENT_FLASH_INIT:
            InitializeFlash();
            break;
        case EVENT_FLASH_TEST:
            RunFlashTests();
            break;
        case FLASH_DUMP:
        {
        	FlashPayload payload;
            memcpy(&payload, cm.GetDataPointer(), sizeof(payload));
            ReadFlash(payload.page, payload.offset, payload.size, payload.data);
        	break;
        }
        case FLASH_WRITE:
        {
            FlashPayload payload;
            memcpy(&payload, cm.GetDataPointer(), sizeof(payload));
            ProgramFlash(payload.page, payload.offset, payload.size, payload.data);
            break;
        }   
        default:
            SOAR_PRINT("FlashTask - Received Unsupported Task Command {%d}\n", cm.GetTaskCommand());
            break;
        }
    }
    else
    {
        SOAR_PRINT("FlashTask - Received Unsupported Global Command {%d}\n", cm.GetCommand());
    }

    cm.Reset();
}

void FlashTask::InitializeFlash() {
    W25N_reset();
    FlashInitialized = true;
}

void FlashTask::RunFlashTests() {
    if (!FlashInitialized)
        return;

    uint32_t jedecID = W25N_read_id();
    if (jedecID == 0xFFFFFFFF)
    {
        SOAR_PRINT("FlashTask::RunFlashTests() - Failed to read flash ID\n");
        return;
    }
    SOAR_PRINT("FlashTask::RunFlashTests() - JEDEC ID: 0x%06lX\n", jedecID);

    // W25N04 has 4096 blocks of 64 pages each, use the last block
    constexpr uint32_t TEST_BLOCK = 4095;
    constexpr uint32_t TEST_PAGE  = TEST_BLOCK * 64;  // first page of last block

    // --- Write ---
    uint8_t txBuf[PAGE_SIZE_BYTES];
    for (uint32_t i = 0; i < PAGE_SIZE_BYTES; i++)
        txBuf[i] = static_cast<uint8_t>(i & 0xFF);

    if (W25N_program_data(TEST_PAGE, 0, PAGE_SIZE_BYTES, txBuf) != 0)
    {
        SOAR_PRINT("FlashTask::RunFlashTests() - Write FAILED\n");
        return;
    }
    SOAR_PRINT("FlashTask::RunFlashTests() - Write OK\n");

    // --- Read back and verify ---
    uint8_t rxBuf[PAGE_SIZE_BYTES];
    memset(rxBuf, 0, sizeof(rxBuf));

    if (W25N_read(TEST_PAGE, 0, PAGE_SIZE_BYTES, rxBuf) != 0)
    {
        SOAR_PRINT("FlashTask::RunFlashTests() - Readback FAILED\n");
        return;
    }

    for (uint32_t i = 0; i < PAGE_SIZE_BYTES; i++)
    {
        if (rxBuf[i] != txBuf[i])
        {
            SOAR_PRINT("FlashTask::RunFlashTests() - Verify FAILED at offset %lu (wrote 0x%02X, read 0x%02X)\n",
                       i, txBuf[i], rxBuf[i]);
            return;
        }
    }
    SOAR_PRINT("FlashTask::RunFlashTests() - Readback verify OK\n");

    // --- Erase ---
    if (W25N_block_erase(TEST_BLOCK) != 0)
    {
        SOAR_PRINT("FlashTask::RunFlashTests() - Erase FAILED\n");
        return;
    }
    SOAR_PRINT("FlashTask::RunFlashTests() - Erase OK\n");

    // --- Verify erased (all 0xFF) ---
    memset(rxBuf, 0, sizeof(rxBuf));

    if (W25N_read(TEST_PAGE, 0, PAGE_SIZE_BYTES, rxBuf) != 0)
    {
        SOAR_PRINT("FlashTask::RunFlashTests() - Read after erase FAILED\n");
        return;
    }

    for (uint32_t i = 0; i < PAGE_SIZE_BYTES; i++)
    {
        if (rxBuf[i] != 0xFF)
        {
            SOAR_PRINT("FlashTask::RunFlashTests() - Erase verify FAILED at offset %lu (0x%02X)\n",
                       i, rxBuf[i]);
            return;
        }
    }

    SOAR_PRINT("FlashTask::RunFlashTests() - Erase verify OK\n");
    SOAR_PRINT("FlashTask::RunFlashTests() - All tests passed\n");
}

void FlashTask::ReadFlash(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data) {
    if (!FlashInitialized)
    {
        return;
    }

    if (data == nullptr || size == 0)
    {
        return;
    }

    if (W25N_read(page, offset, size, data) != 0)
    {
        return;
    }
}

void FlashTask::ProgramFlash(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data) {
    if (!FlashInitialized)
    {
        return;
    }

    if (data == nullptr || size == 0)
    {
        return;
    }

    if (W25N_program_data(page, offset, size, data) != 0)
    {
        return;
    }

}