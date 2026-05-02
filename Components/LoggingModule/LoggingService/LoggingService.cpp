#include "LoggingService.hpp"

uint8_t   LoggingService::ramLog[RAM_LOG_SIZE] = {0};
uint32_t  LoggingService::ramHead              = 0;
uint32_t  LoggingService::currentPage          = 0;
uint16_t  LoggingService::pageOffset           = 0;
uint8_t   LoggingService::done                 = 0;
uint8_t   LoggingService::doneDump             = 0;

static uint8_t txBuf[RAM_LOG_SIZE];
static uint8_t rxBuf[RAM_LOG_SIZE];
static uint8_t sectorBuf[RAM_LOG_SIZE];

LoggingService::LoggingService(LoggingDest dest, LoggingData dataType,
                                uint8_t* ldata, uint32_t dataSize,
                                LoggingPriority priority)
{
    loggingData.dest     = dest;
    loggingData.dataType = dataType;
    loggingData.data     = ldata;
    loggingData.dataSize = dataSize;
    loggingData.priority = priority;
}

LoggingStatus LoggingService::LogData() {
    switch (loggingData.dest) {
        case LoggingDest::RAM:          return LogToInternalMemory();
        case LoggingDest::FLASH_EXTERN: return LogToW25N();
        case LoggingDest::FILE_SYSTEM:  return LoggingStatus::LOGGING_ERR;
        case LoggingDest::DMA:          return LoggingStatus::LOGGING_ERR;
    }
    return LoggingStatus::LOGGING_ERR;
}

LoggingStatus LoggingService::LogToW25N() {
    if (done) return LoggingStatus::FLASH_FULL;

    LoggingStatus status = MemAppend(&loggingData);

    if (status == LoggingStatus::LOG_FLASH_READY) {
        memcpy(txBuf, ramLog, RAM_LOG_SIZE);

        // Erase block when we land on the first page of a new block
        if (pageOffset == 0 && (currentPage % LOG_PAGES_PER_BLOCK) == 0) {
            uint32_t block = currentPage / LOG_PAGES_PER_BLOCK;
            if (W25N_block_erase(block) != 0)
                return LoggingStatus::LOGGING_ERR;
        }

        // Write
        if (W25N_program_data(currentPage, pageOffset, RAM_LOG_SIZE, txBuf) != 0)
            return LoggingStatus::LOGGING_ERR;

        // Read back and verify
        if (W25N_read(currentPage, pageOffset, RAM_LOG_SIZE, rxBuf) != 0)
            return LoggingStatus::LOGGING_ERR;

        if (!BytesEqual(txBuf, rxBuf, RAM_LOG_SIZE))
            return LoggingStatus::LOGGING_ERR;

        // Advance position — 4 x 500B chunks fit in one 2048B page
        pageOffset += RAM_LOG_SIZE;
        if (pageOffset >= LOG_PAGE_SIZE_BYTES) {
            pageOffset = 0;
            currentPage++;
            if (currentPage >= LOG_TOTAL_PAGES)
                done = true;
        }

        return LoggingStatus::LOGGING_SUCCESS;
    }

    return LoggingStatus::LOG_FLASH_NOT_READY;
}

static const char* SensorTypeName(LoggingData type) {
    switch (type) {
        case LoggingData::IMU32G: return "IMU32G";
        case LoggingData::IMU16G: return "IMU16G";
        case LoggingData::MAG:    return "MAG";
        case LoggingData::BARO07: return "BARO07";
        case LoggingData::BARO11: return "BARO11";
        case LoggingData::GPS:    return "GPS";
        case LoggingData::FILTER: return "FILTER";
        default:                  return "UNKNOWN";
    }
}

void LoggingService::ProcessFlashDump() {
    done     = true;
    doneDump = false;

    constexpr uint32_t RECORD_SIZE = 20;
    constexpr uint32_t CHUNK_SIZE  = RAM_LOG_SIZE; // 500

    uint32_t dumpPage = 0;
    uint16_t dumpOff  = 0;

    while (dumpPage < LOG_TOTAL_PAGES && !doneDump) {
        memset(sectorBuf, 0, sizeof(sectorBuf));

        if (W25N_read(dumpPage, dumpOff, CHUNK_SIZE, sectorBuf) != 0)
            break;

        for (uint32_t i = 0; i + RECORD_SIZE <= CHUNK_SIZE; i += RECORD_SIZE) {
            LoggingData type = static_cast<LoggingData>(sectorBuf[i]);
            uint32_t timestamp;
            memcpy(&timestamp, sectorBuf + i + 1, sizeof(timestamp));
            uint8_t id = sectorBuf[i + 19];

            if (type == LoggingData::IMU16G || type == LoggingData::IMU32G) {
                int16_t accel[3], gyro[3], temp;
                memcpy(accel, sectorBuf + i + 5,  sizeof(accel));
                memcpy(gyro,  sectorBuf + i + 11, sizeof(gyro));
                memcpy(&temp, sectorBuf + i + 17, sizeof(temp));
                SOAR_PRINT("%s(ID=%u) Timestamp=%lu Accel=[%d,%d,%d] Gyro=[%d,%d,%d] Temp=%d\n",
                    SensorTypeName(type), id, timestamp,
                    accel[0], accel[1], accel[2],
                    gyro[0], gyro[1], gyro[2], temp);
            }
            else if (type == LoggingData::BARO07 || type == LoggingData::BARO11) {
                int32_t pressure; int16_t temperature;
                memcpy(&pressure,    sectorBuf + i + 5, sizeof(pressure));
                memcpy(&temperature, sectorBuf + i + 9, sizeof(temperature));
                int16_t tc = temperature / 100;
                int16_t tf = temperature % 100;
                if (tf < 0) tf = -tf;
                SOAR_PRINT("%s(ID=%u) Timestamp=%lu Pressure=%ld Temp=%d.%02d\n",
                    SensorTypeName(type), id, timestamp, pressure, tc, tf);
            }
            else if (type == LoggingData::MAG) {
                int32_t magX, magY, magZ;
                memcpy(&magX, sectorBuf + i + 5,  sizeof(int32_t));
                memcpy(&magY, sectorBuf + i + 9,  sizeof(int32_t));
                memcpy(&magZ, sectorBuf + i + 13, sizeof(int32_t));
                SOAR_PRINT("MAG Timestamp=%lu Mag=[%ld,%ld,%ld]\n",
                    timestamp, (long)magX, (long)magY, (long)magZ);
            }
        }

        // Advance through the page in 500B chunks
        dumpOff += CHUNK_SIZE;
        if (dumpOff >= LOG_PAGE_SIZE_BYTES) {
            dumpOff = 0;
            dumpPage++;
        }
    }

    SOAR_PRINT("------FLASH DUMP COMPLETE------\n");
}

void LoggingService::StopDump() { doneDump = true; }

LoggingStatus LoggingService::LogToInternalMemory() {
    if (loggingData.data == nullptr || loggingData.dataSize == 0)
        return LoggingStatus::LOGGING_ERR;
    return MemAppend(&loggingData);
}

bool LoggingService::BytesEqual(const uint8_t* a, const uint8_t* b, uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
        if (a[i] != b[i]) return false;
    return true;
}

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