/**
 ********************************************************************************
 * @file    LoggingService.hpp
 * @author  Christy
 * @date    May 2, 2026
 * @brief
 ********************************************************************************
 */

#ifndef INC_LOGGINGSERVICE_HPP_
#define INC_LOGGINGSERVICE_HPP_

/************************************
 * INCLUDES
 ************************************/
#include "Log.hpp"
#include "DataBroker.hpp"
#include "Command.hpp"
#include "W25N04KVZEIR.hpp"

/************************************
 * MACROS AND DEFINES
 ************************************/
#define LOG_PAGE_SIZE_BYTES 2048
#define LOG_PAGES_PER_BLOCK 64
#define LOG_NUM_BLOCKS 4096
#define LOG_TOTAL_PAGES (LOG_NUM_BLOCKS * LOG_PAGES_PER_BLOCK)

// Logging buffer
#define MAX_LOG_SIZE 20
#define RAM_LOG_SIZE 500

/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * CLASS DEFINITIONS
 ************************************/
class LoggingService {
public:
    LoggingService(LoggingDest dest, LoggingData dataType,
                   uint8_t* data, uint32_t dataSize, LoggingPriority priority);
    LoggingStatus LogData();
    static void ProcessFlashDump();
    static void StopDump();
	static uint32_t GetCurrentPage() { return currentPage; }
	static uint16_t GetPageOffset() { return pageOffset; }

private:
    LoggingStatus LogToW25N();
    LoggingStatus LogToInternalMemory();
    bool BytesEqual(const uint8_t* a, const uint8_t* b, uint32_t n);
    LoggingStatus MemAppend(const LoggingPacket* data);
    LoggingPacket loggingData;
    static uint8_t ramLog[RAM_LOG_SIZE];
    static uint32_t ramHead;

    // Position tracking in terms of W25N pages
    static uint32_t currentPage;
    static uint16_t pageOffset;
    static uint8_t done;
    static uint8_t doneDump;
};
/************************************
 * FUNCTION DECLARATIONS
 ************************************/

#endif /* INC_LOGGINGSERVICE_HPP_ */