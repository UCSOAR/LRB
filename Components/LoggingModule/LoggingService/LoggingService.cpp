/*
 * LoggingService.cpp
 *
 *  Created on: Jan 21, 2026
 *      Author: jaddina
 */
#include "LoggingService.hpp"
#include "stm32h7xx_hal.h"

uint8_t  LoggingService::ramLog[RAM_LOG_SIZE] = {0};
uint32_t LoggingService::ramHead = 0;
uint16_t LoggingService::sectorAddress = 0;
uint8_t LoggingService::bufferPerSector = 0;
uint8_t LoggingService::sectorCount = 0;
uint8_t LoggingService::done = 0;
uint8_t LoggingService::doneDump =0;


static uint32_t dumpIndex = 0;
static uint8_t sectorBuf[RAM_LOG_SIZE];
static uint32_t dumpSector = 0;
static uint16_t dumpOffset = 0;
static uint32_t sectorStartTickMs = 0;
static uint8_t txBuf[RAM_LOG_SIZE];
static uint8_t rxBuf[RAM_LOG_SIZE];


LoggingService::LoggingService(LoggingDest dest, LoggingData dataType, uint8_t* ldata, uint32_t dataSize, LoggingPriority priority)
{
	loggingData.dest = dest;
	loggingData.dataType = dataType;
	loggingData.data = ldata;
	loggingData.dataSize = dataSize;
	loggingData.priority = priority;


}


LoggingStatus LoggingService::LogData(){

	LoggingStatus err;

	switch(loggingData.dest){
	case LoggingDest::RAM:
		//internal flash api
		err = LogToInternalMemory();

		break;

	case LoggingDest::FLASH_EXTERN:
		err = LogToMX66();
		break;

	case LoggingDest::FILE_SYSTEM:
		//TODO FS write api

		err = LoggingStatus::LOGGING_ERR;
		break;

	case LoggingDest::DMA:

		//TODO DMA write api
		err = LoggingStatus::LOGGING_ERR;
		break;
	}

	return err;
}


LoggingStatus LoggingService::LogToMX66(){

	//appends data to buffer and returns flag determining if buffer is full (full => write data)
	LoggingStatus status = MemAppend(&loggingData);
	//stops logging, triggered by flash dump, or flashchip is full
	if(!done){
		//checks if 500 byte chunck is full (writes when it is full)
		if(status == LoggingStatus::LOG_FLASH_READY){
			if(bufferPerSector == 0){
				//if a new sector is reached erase before writing
				MX66xxQSPI_EraseSector(sectorAddress);
			}

			//Write data to bufferPerSector * 500 this calculates the offset at which the buffer will be written
			memcpy(txBuf, ramLog, RAM_LOG_SIZE);

			MX66xxQSPI_WriteSector(txBuf, sectorAddress, (bufferPerSector * 500),  RAM_LOG_SIZE);

			MX66xxQSPI_ReadSector(rxBuf, sectorAddress,(bufferPerSector * 500),  RAM_LOG_SIZE);

			//check if the bytes written equal the bytes read
			if(BytesEqual(rxBuf, txBuf, RAM_LOG_SIZE)){

				/* Each buffer is 500 bytes, 8 can fit in 4000 bytes
				 * bufferPerSector increases each successful buffer write which
				 * is used to keep trak of when a sector is full, if it is full
				 * (bufferPerSector == 8) then go to the next sector.
				 */
				bufferPerSector++;
				if(bufferPerSector == 8){
					sectorAddress++;
					bufferPerSector = 0;
					//finish logging if the sectorAddress reaches address that does not exist
					if(sectorAddress > NUM_SECTORS){
						done = true;
					}
				}

				return LoggingStatus::LOGGING_SUCCESS;
			}


			return status;

		}
		return LoggingStatus::LOG_FLASH_NOT_READY;
	}
	return LoggingStatus::FLASH_FULL;


}

const char* SensorTypeName(LoggingData type)
{
    switch(type)
    {
        case LoggingData::IMU32G: return "IMU32G";
        case LoggingData::IMU16G: return "IMU16G";
        case LoggingData::MAG:    return "MAG";
        case LoggingData::BARO07: return "BARO07";
        case LoggingData::BARO11: return "BARO11";
        case LoggingData::GPS:    return "GPS";
        case LoggingData::FILTER: return "FILTER";
        default: return "UNKNOWN";
    }
}

void LoggingService::ProcessFlashDump()
{
	 done = true;
	doneDump = false;

	constexpr uint32_t RECORD_SIZE = 20;
	constexpr uint32_t CHUNK_SIZE  = 500;

	while (dumpSector < NUM_SECTORS && !doneDump) { //go untill all sectors have been read or doneDump flag is triggered

		memset(sectorBuf, 0, sizeof(sectorBuf));
		MX66xxQSPI_ReadSector(sectorBuf, dumpSector, dumpOffset, CHUNK_SIZE);

		for (uint32_t i = 0; i + RECORD_SIZE <= CHUNK_SIZE; i += RECORD_SIZE) //each record is 20 bytes, Chunck size is 500 to prevent hardfault
		{
			LoggingData type = static_cast<LoggingData>(sectorBuf[i]); //get type byte first
			uint32_t timestamp;
			memcpy(&timestamp, sectorBuf + i + 1, sizeof(timestamp));
			uint8_t id = sectorBuf[i + 19];

			//parse the data back into structs

			if(type == LoggingData::IMU16G || type == LoggingData::IMU32G)
			{
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
			else if(type == LoggingData::BARO07 || type == LoggingData::BARO11)
			{
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
			else if (type == LoggingData::MAG){

				int32_t magX, magY, magZ;

				memcpy(&magX, sectorBuf + i + 5,  sizeof(int32_t));
				memcpy(&magY, sectorBuf + i + 9,  sizeof(int32_t));
				memcpy(&magZ, sectorBuf + i + 13, sizeof(int32_t));

				SOAR_PRINT("%s Timestamp=%lu Mag=[%ld,%ld,%ld]\n",
					SensorTypeName(type), timestamp,
					(long)magX, (long)magY, (long)magZ);

			}

		}
		/*the dump offset at this point should be 3500, this means sector is full
		 * (writtern up to 4000 bytes, if this is true set the offset back to 0 and
		 * move to the next sector, otherwise increment dumpoffset to the next chunk
		 */
		if (dumpOffset == SECTOR_READ - CHUNK_SIZE) {
			dumpOffset = 0;
			dumpSector++;
		} else {
			dumpOffset += 500;
		}
	}

	SOAR_PRINT("------FLASH DUMP COMPLETE------");
}





void LoggingService::StopDump(){
	doneDump = true;
}


LoggingStatus LoggingService::LogToInternalMemory(){

	if(loggingData.data == nullptr || loggingData.dataSize == 0){
		return LoggingStatus::LOGGING_ERR;
	}

	return MemAppend(&loggingData);

}

bool LoggingService::BytesEqual(const uint8_t* a, const uint8_t* b, uint32_t n){

	for(uint32_t i = 0; i < n; i++){

		if(a[i] != b[i]){
			return false;
		}

	}

	return true;
}

LoggingStatus LoggingService::MemAppend(const LoggingPacket *data){

	if(!data){return LoggingStatus::LOGGING_ERR;}

	uint32_t size = data->dataSize;

	if (size > MAX_LOG_SIZE){
		size = MAX_LOG_SIZE;
	}

	if(ramHead + MAX_LOG_SIZE > RAM_LOG_SIZE){ //check if word will fit in buffer
		ramHead = 0;

		return LoggingStatus::LOG_FLASH_READY;
	}


	for(uint32_t i = 0; i < size; i++){

		ramLog[ramHead++] = data->data[i];
	}

	for(uint32_t i = size; i < MAX_LOG_SIZE; i++){
		ramLog[ramHead++] = 0;
	}


	return LoggingStatus::LOG_FLASH_NOT_READY;



}


