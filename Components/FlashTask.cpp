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
uint32_t FlashTask::FLASHWRITEADDR = 0; //define static write address

/************************************
 * FUNCTION DECLARATIONS
 ************************************/

/************************************
 * FUNCTION DEFINITIONS
 ************************************/
/**
 * @brief constructor, sets up task
 */
 FlashTask::FlashTask()
    : Task(TASK_FLASH_QUEUE_DEPTH_OBJS),
    FLASHINIT(false)
{
}

/**
 * @brief run loop, waits for commands and sends them to the command handler
 */
void FlashTask::Run(void *pvParams) {
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


/**
 * @brief Initialize the FlashTask
 */
void FlashTask::InitTask() {
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


/**
 * @brief Handles a command
 * @param cm Command reference to handle
 */
void FlashTask::HandleCommand(Command &cm) {
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
            LoggingService::ProcessFlashDump();
        	break;
        }
        case FLASH_WRITE:
        {
            FlashPayload payload;
            memcpy(&payload, cm.GetDataPointer(), sizeof(payload));
            AppendFlash(payload.size, payload.data);
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


/**
 * @brief convert address to page and offset
 */
void FlashTask::AddrToPageOffset(uint32_t addr, uint32_t &page, uint16_t &offset)
{
    page = addr / PAGE_SIZE_BYTES;
    offset = addr % PAGE_SIZE_BYTES;
}


/**
 * @brief initialize flash, must be called before other flash operations
 */
void FlashTask::InitializeFlash() {
    W25N_reset();
    FLASHINIT = true;
    FLASHWRITEADDR = 0;
}


/**
 * @brief run test to verify id, write, and clear
 */
void FlashTask::RunFlashTests() {
    // return if flash not initialized
    if (!FLASHINIT)
        return;

    // verify JEDEC ID not invalid
    uint32_t jedecID = W25N_read_id();
    if (jedecID == 0xFFFFFFFF)
    {
        SOAR_PRINT("Failed to read flash ID\n");
        return;
    }

    // define test block and page
    constexpr uint32_t testBlock = 4095;
    constexpr uint32_t testPage  = testBlock * 64;

    // write to flash
    uint8_t txBuf[PAGE_SIZE_BYTES];
    for (uint32_t i = 0; i < PAGE_SIZE_BYTES; i++)
        txBuf[i] = static_cast<uint8_t>(i & 0xFF);

    if (W25N_program_data(testPage, 0, PAGE_SIZE_BYTES, txBuf) != 0)
    {
        SOAR_PRINT("Failed to write to flash\n");
        return;
    }

    // read back and verify
    uint8_t rxBuf[PAGE_SIZE_BYTES];
    memset(rxBuf, 0, sizeof(rxBuf));
    if (W25N_read(testPage, 0, PAGE_SIZE_BYTES, rxBuf) != 0)
    {
        SOAR_PRINT("Flash read failed\n");
        return;
    }

    // verify read data matches written data
    for (uint32_t i = 0; i < PAGE_SIZE_BYTES; i++)
    {
        if (rxBuf[i] != txBuf[i])
        {
            SOAR_PRINT("Failed to verify flash data at offset %lu (wrote 0x%02X, read 0x%02X)\n", i, txBuf[i], rxBuf[i]);
            return;
        }
    }

    // erase block
    if (W25N_block_erase(testBlock) != 0)
    {
        SOAR_PRINT("Failed to erase flash block\n");
        return;
    }

    // verify erase
    memset(rxBuf, 0, sizeof(rxBuf));
    if (W25N_read(testPage, 0, PAGE_SIZE_BYTES, rxBuf) != 0)
    {
        SOAR_PRINT("Failed to read flash after erase\n");
        return;
    }

    // verify erased data is 0xFF
    for (uint32_t i = 0; i < PAGE_SIZE_BYTES; i++)
    {
        if (rxBuf[i] != 0xFF)
        {
            SOAR_PRINT("Failed to verify erased flash data at offset %lu (0x%02X)\n", i, rxBuf[i]);
            return;
        }
    }

    SOAR_PRINT("FlashTask::RunFlashTests() - All tests passed\n");
}


/**
 * @brief append data to flash at current static address
*/
void FlashTask::AppendFlash(uint16_t size, uint8_t *data) {
    if (!FLASHINIT || data == nullptr || size == 0)
        return;

    uint32_t remaining = size;
    uint32_t addr = FLASHWRITEADDR;
    uint32_t dataIdx = 0;

    while (remaining > 0) {
        uint32_t page;
        uint16_t offset;
        AddrToPageOffset(addr, page, offset);

        // erase block if we're at the start of a block boundary
        if (offset == 0 && (page % LOG_PAGES_PER_BLOCK) == 0) {
            uint32_t block = page / LOG_PAGES_PER_BLOCK;
            if (W25N_block_erase(block) != 0) {
                return;
            }
        }

        // calculate chunk size to write within current page
        uint16_t chunk = PAGE_SIZE_BYTES - offset;
        if (chunk > remaining)
            chunk = remaining;

        // write to flash
        if (W25N_program_data(page, offset, chunk, &data[dataIdx]) != 0) {
            return;
        }

        // increment counters
        addr += chunk;
        dataIdx += chunk;
        remaining -= chunk;
    }

    FLASHWRITEADDR = addr;
}


/**
 * @brief read from flash at given address and size into provided buffer
 */
void FlashTask::ReadFlash(uint32_t addr, uint16_t size, uint8_t* buf) {
    if (!FLASHINIT || buf == nullptr || size == 0)
        return;

    uint32_t remaining = size;
    uint32_t currentAddr = addr;
    uint32_t bufIdx = 0;

    // iterate while flash contents remain
    while (remaining > 0) {
        uint32_t page;
        uint16_t offset;
        AddrToPageOffset(currentAddr, page, offset);

        // read in chunks within current page
        uint16_t chunk = PAGE_SIZE_BYTES - offset;
        if (chunk > remaining)
            chunk = remaining;

        if (W25N_read(page, offset, chunk, &buf[bufIdx]) != 0)
            return;

        // increment counters
        currentAddr += chunk;
        bufIdx += chunk;
        remaining -= chunk;
    }
}


/**
 * @brief erase flash block
 */
void FlashTask::EraseBlocks(uint32_t startBlock, uint32_t count) {
    for (uint32_t b = startBlock; b < startBlock + count; b++) {
        if (W25N_block_erase(b) != 0)
            SOAR_PRINT("FlashTask::EraseBlocks() - failed to erase block %lu\n", b);
    }
}
