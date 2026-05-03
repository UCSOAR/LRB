/**
 ********************************************************************************
 * @file    FlashTask.hpp
 * @author  Christy
 * @date    Apr 25, 2026
 * @brief
 ********************************************************************************
 */

#ifndef INC_FLASHTASK_HPP_
#define INC_FLASHTASK_HPP_

/************************************
 * INCLUDES
 ************************************/
#include "SystemDefines.hpp"
#include "Task.hpp"
#include "W25N04KVZEIR.hpp"
#include "LoggingService.hpp"
/************************************
 * MACROS AND DEFINES
 ************************************/
enum FLASH_TASK_COMMANDS
{
    FLASH_TASK_COMMAND_NONE = 0,
    EVENT_FLASH_INIT,
    EVENT_FLASH_TEST,
	FLASH_DUMP,
	STOP_DUMP,
    FLASH_WRITE
};

struct FlashPayload {
    uint16_t size;
    uint8_t data[256];
};
/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * CLASS DEFINITIONS
 ************************************/
class FlashTask : public Task {
public:
    static FlashTask& Inst() {
        static FlashTask inst;
        return inst;
    }

    void InitTask();
    void InitializeFlash();
    void RunFlashTests();
    void AppendFlash(uint16_t size, uint8_t *data);
    void AddrToPageOffset(uint32_t addr, uint32_t &page, uint16_t &offset);
    void ReadFlash(uint32_t addr, uint16_t size, uint8_t* buf);
    void EraseBlocks(uint32_t startBlock, uint32_t count);
    uint32_t GetWriteAddr() const { return FLASHWRITEADDR; }

    static constexpr uint16_t PAGE_SIZE_BYTES = 2048; // page size
    static constexpr uint16_t LOG_NUM_BLOCKS = 2048; // number of blocks
    static constexpr uint16_t LOG_PAGES_PER_BLOCK = 64; // pages in a block

protected:
    static void RunTask(void* pvParams) {
        FlashTask::Inst().Run(pvParams);
    }  // Static Task Interface, passes control to the instance Run();

    void Run(void* pvParams);  // Main run code
    void HandleCommand(Command &cm);
    

private:
    FlashTask();                             // Private constructor
    FlashTask(const FlashTask&);             // Prevent copy-construction
    FlashTask& operator=(const FlashTask&);  // Prevent assignment


    bool FLASHINIT = false;                           // whether the flash has been initialized or not
    static uint32_t FLASHWRITEADDR;                   // current write address
};
/************************************
 * FUNCTION DECLARATIONS
 ************************************/

#endif /* INC_FLASHTASK_HPP_ */
