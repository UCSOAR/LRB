/**
 ********************************************************************************
 * @file    LoggingService.cpp
 * @author  Christy
 * @date    May 2, 2026
 * @brief
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "LoggingService.hpp"
/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * VARIABLES
 ************************************/
uint8_t LoggingService::ramLog[RAM_LOG_SIZE] = {0};
uint32_t LoggingService::ramHead = 0;
uint32_t LoggingService::currentPage = 0;
uint16_t LoggingService::pageOffset = 0;
uint8_t LoggingService::done = 0;
uint8_t LoggingService::doneDump = 0;

static uint8_t txBuf[RAM_LOG_SIZE];
static uint8_t rxBuf[RAM_LOG_SIZE];
static uint8_t sectorBuf[RAM_LOG_SIZE];

/************************************
 * FUNCTION DECLARATIONS
 ************************************/

/************************************
 * FUNCTION DEFINITIONS
 ************************************/
/**
 * @brief constructor
 */
LoggingService::LoggingService(LoggingDest dest, LoggingData dataType, uint8_t* ldata, uint32_t dataSize, LoggingPriority priority) {
    loggingData.dest = dest;
    loggingData.dataType = dataType;
    loggingData.data = ldata;
    loggingData.dataSize = dataSize;
    loggingData.priority = priority;
}


/**
 * @brief main logging function, routes to the correct logging method based on the destination
 */
LoggingStatus LoggingService::LogData() {
    switch (loggingData.dest) {
        case LoggingDest::RAM: return LogToInternalMemory();
        case LoggingDest::FLASH_EXTERN: return LogToW25N();
        case LoggingDest::FILE_SYSTEM: return LoggingStatus::LOGGING_ERR;
        case LoggingDest::DMA: return LoggingStatus::LOGGING_ERR;
    }
    return LoggingStatus::LOGGING_ERR;
}


/**
 * @brief logs data to the W25N flash
 */
LoggingStatus LoggingService::LogToW25N() {
    if (done) return LoggingStatus::FLASH_FULL;

    LoggingStatus status = MemAppend(&loggingData);

    if (status == LoggingStatus::LOG_FLASH_READY) {
        memcpy(txBuf, ramLog, RAM_LOG_SIZE);
        FlashTask::Inst().AppendFlash(RAM_LOG_SIZE, txBuf);
        return LoggingStatus::LOGGING_SUCCESS;
    }

    return LoggingStatus::LOG_FLASH_NOT_READY;
}


/**
 * @brief helper function to convert logging data type to string for printing during flash dump
 */
static const char* SensorTypeName(LoggingData type) {
    switch (type) {
        // case LoggingData::IMU32G: return "IMU32G";
        // case LoggingData::IMU16G: return "IMU16G";
        // case LoggingData::MAG: return "MAG";
        // case LoggingData::BARO07: return "BARO07";
        // case LoggingData::BARO11: return "BARO11";
        // case LoggingData::GPS: return "GPS";
        // case LoggingData::FILTER: return "FILTER";
        default: return "UNKNOWN";
    }
}


/**
 * @brief logs data to internal memory buffer
 */
LoggingStatus LoggingService::LogToInternalMemory() {
    if (loggingData.data == nullptr || loggingData.dataSize == 0)
        return LoggingStatus::LOGGING_ERR;
    return MemAppend(&loggingData);
}


/**
 * @brief appends data to the internal memory buffer
 */
LoggingStatus LoggingService::MemAppend(const LoggingPacket* data) {
    if (!data) return LoggingStatus::LOGGING_ERR;

    uint32_t size = data->dataSize;
    if (size > MAX_LOG_SIZE) size = MAX_LOG_SIZE;

    if (ramHead + MAX_LOG_SIZE > RAM_LOG_SIZE) {
        ramHead = 0;
        return LoggingStatus::LOG_FLASH_READY;
    }

    for (uint32_t i = 0; i < size; i++)
        ramLog[ramHead++] = data->data[i];

    for (uint32_t i = size; i < MAX_LOG_SIZE; i++)
        ramLog[ramHead++] = 0;

    return LoggingStatus::LOG_FLASH_NOT_READY;
}


/**
 * @brief reads flash contents and formats in human readable format
 */
void LoggingService::ProcessFlashDump() {
    done = true;
	uint32_t readAddr = 0;
    uint32_t totalBytes = FlashTask::Inst().GetWriteAddr();

    while (readAddr < totalBytes) {
		// read flash contents into cleared sector buffer
        memset(sectorBuf, 0xFF, sizeof(sectorBuf));
        FlashTask::Inst().ReadFlash(readAddr, RAM_LOG_SIZE, sectorBuf);

		// read records in 20 byte chunks
        for (uint32_t i = 0; i + 20 <= RAM_LOG_SIZE; i += 20) {
			// form logging data entry
            LoggingData type = static_cast<LoggingData>(sectorBuf[i]);
            uint32_t timestamp;
            memcpy(&timestamp, sectorBuf + i + 1, sizeof(timestamp));
            uint8_t id = sectorBuf[i + 19];

			// convert to human readable format based on data type
            if (type == LoggingData::IMU16G || type == LoggingData::IMU32G) {
                int16_t accel[3], gyro[3], temp;

				memcpy(accel, sectorBuf + i + 5, sizeof(accel));
				memcpy(gyro, sectorBuf + i + 11, sizeof(gyro));
				memcpy(&temp, sectorBuf + i + 17, sizeof(temp));

				SOAR_PRINT("%s(ID=%u) Timestamp=%lu Accel=[%d,%d,%d] Gyro=[%d,%d,%d] Temp=%d\n",
									SensorTypeName(type), id, timestamp,
									accel[0], accel[1], accel[2],
									gyro[0], gyro[1], gyro[2],
									temp);
            }
            else if (type == LoggingData::BARO07 || type == LoggingData::BARO11) {
                int32_t pressure;
				int16_t temperature;
				memcpy(&pressure, sectorBuf + i + 5, sizeof(pressure));
				memcpy(&temperature, sectorBuf + i + 9, sizeof(temperature));

				int16_t temp_c = temperature / 100;          // integer part
				int16_t temp_frac = temperature % 100;       // fractional part
				if (temp_frac < 0) temp_frac = -temp_frac;   // handle negative temperatures

				SOAR_PRINT("%s(ID=%u) Timestamp=%lu Pressure=%ld Temp=%d.%02d\n",
							SensorTypeName(type), id, timestamp, pressure, temp_c, temp_frac);
            }
            else if (type == LoggingData::MAG) {
                int32_t magX, magY, magZ;

				memcpy(&magX, sectorBuf + i + 5,  sizeof(int32_t));
				memcpy(&magY, sectorBuf + i + 9,  sizeof(int32_t));
				memcpy(&magZ, sectorBuf + i + 13, sizeof(int32_t));

				SOAR_PRINT("%s Timestamp=%lu Mag=[%ld,%ld,%ld]\n",
					SensorTypeName(type), timestamp,
					(long)magX, (long)magY, (long)magZ);
            }
            osDelay(50);
        }
        readAddr += RAM_LOG_SIZE;	// increment read address by buffer size
    }
}
