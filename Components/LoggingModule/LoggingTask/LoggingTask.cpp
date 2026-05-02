/*
 * LoggingTask.cpp
 *
 *  Created on: Jan 23, 2026
 *      Author: jaddina
 */


/**
 ********************************************************************************
 * @file    LoggingTask.cpp
 * @author  jaddina
 * @date    Sep 13, 2025
 * @brief
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "LoggingTask.hpp"
#include "DebugTask.hpp"
#include "SystemDefines.hpp"
#include "Command.hpp"
#include "LoggingService.hpp"

#include "DataBroker.hpp"
#include "Task.hpp"

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * VARIABLES
 ************************************/

/************************************
 * FUNCTION DECLARATIONS
 ************************************/


uint8_t LoggingTask::buf[20] = {0};
bool LoggingTask::highAltitude = false;
bool LoggingTask::lowAltitude = false;

//bool LoggingTask::firstAlt = false;

float firstAlt = 0.0f;
bool firstAltCaptured = false;
bool launched = false;
bool landed = false;



/************************************
 * FUNCTION DEFINITIONS
 ************************************/
LoggingTask::LoggingTask():Task(TASK_LOGGING_QUEUE_DEPTH_OBJS)
{

}



/**
 * @brief Initialize the LoggingTask
 *        Do not modify this function aside from adding the task name
 */
void LoggingTask::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize watchdog task twice");

    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)LoggingTask::RunTask,
            (const char*)"LoggingTask",
            (uint16_t)TASK_LOGGING_QUEUE_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK_LOGGING_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

                SOAR_ASSERT(rtValue == pdPASS, "LoggingTask::InitTask() - xTaskCreate() failed");

	DataBroker::Subscribe<IMUData>(this);
	DataBroker::Subscribe<BaroData>(this);
	DataBroker::Subscribe<MagData>(this);

}

void LoggingTask::Run(void * pvParams){



    while (1) {
        /* Process commands in blocking mode */
        Command cm;
        bool res = qEvtQueue->ReceiveWait(cm);
        if(res){

        	HandleCommand(cm);
        }
    }

}

void LoggingTask::HandleCommand(Command& cm){

	// SOAR_PRINT("Data In Logging Task\n");
	if(cm.GetCommand() != DATA_BROKER_COMMAND){
		return;
	}

	LoggingStatus err = LoggingStatus::LOGGING_SUCCESS;
	DataBrokerMessageTypes messageType = DataBroker::getMessageType(cm);

	switch(messageType){

	case DataBrokerMessageTypes::IMU_DATA:
	{
		IMUData data = DataBroker::ExtractData<IMUData>(cm);
		uint32_t timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;

		// Only log IDs 0 or 1
		if (data.id != 0 && data.id != 1) {
			cm.Reset();
		    return;
		}

//
		// Prepare buffer for flash logging
		buf[0] = static_cast<uint8_t>(data.id == 0 ? LoggingData::IMU16G : LoggingData::IMU32G);

		// Manual serialization to avoid padding issues
		memcpy(buf + 1, &timestamp, sizeof(timestamp));        // 4 bytes timestamp
		memcpy(buf + 5, &data.accel, sizeof(data.accel));     // 6 bytes accel
		memcpy(buf + 11, &data.gyro, sizeof(data.gyro));      // 6 bytes gyro
		memcpy(buf + 17, &data.temp, sizeof(data.temp));      // 2 bytes temp
		buf[19] = data.id;                                     // 1 byte id

		// Log to flash
		LoggingService log(
		    LoggingDest::FLASH_EXTERN,
		    data.id == 0 ? LoggingData::IMU16G : LoggingData::IMU32G,
		    buf,
		    20,  // total bytes: 1(type) + 4(timestamp) + 6+6+2+1 = 20
		    LoggingPriority::SECOND
		);
		err = log.LogData();
		break;
	}

	case DataBrokerMessageTypes:: MAG_DATA:
	{
		MagData data = DataBroker::ExtractData<MagData>(cm);

		uint32_t timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;

		//SOAR_PRINT("Mag data (%d, %d, %d)\n", data.magX, data.magY, data.magZ);
		memset(buf, 0, 20);

		buf[0] = static_cast<uint8_t>(LoggingData::MAG);

		//add timestamp 4 bytes
		memcpy(buf + 1, &timestamp, sizeof(timestamp));

		//add mag XYZ 12 bytes
		memcpy(buf + 5, &data.magX, sizeof(data.magX));
		memcpy(buf + 9, &data.magY, sizeof(data.magY));
		memcpy(buf + 13, &data.magZ, sizeof(data.magZ));

		memset(buf + 17, 0, 20 - 1 - 16); // pad rest with zero


		LoggingService log = LoggingService(LoggingDest::FLASH_EXTERN, LoggingData::MAG, buf, 20, LoggingPriority::SECOND);
		err = log.LogData();

		break;
	}

	case DataBrokerMessageTypes:: BARO_DATA:
	{
		BaroData data = DataBroker::ExtractData<BaroData>(cm);

		// Get timestamp in ms
		uint32_t timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;


		// Determine BARO type
		buf[0] = static_cast<uint8_t>(data.id == 0 ? LoggingData::BARO07 : LoggingData::BARO11);

		// Copy timestamp (4 bytes)
		memcpy(buf + 1, &timestamp, sizeof(timestamp));

		// Copy pressure (4 bytes) and temperature (2 bytes)
		memcpy(buf + 5, &data.pressure, sizeof(data.pressure));
		memcpy(buf + 9, &data.temp, sizeof(data.temp));

		// Pad remaining bytes with zeros so the total slot is 20 bytes
		memset(buf + 11, 0, 20 - 1 - 10); // 1 byte type + 10 bytes data already, last byte is ID

		// Set ID as the last byte of the 20-byte slot
		buf[19] = data.id;

		// Log the full 20-byte sample
		LoggingService log(
		    LoggingDest::FLASH_EXTERN,
		    data.id == 0 ? LoggingData::BARO07 : LoggingData::BARO11,
		    buf,
		    20,                  // full 20-byte slot
		    LoggingPriority::SECOND
		);

		err = log.LogData();

		break;
	}
	case DataBrokerMessageTypes :: INVALID:
	{
		cm.Reset();
		return;
	}

	}

	if(err == LoggingStatus::LOGGING_ERR){
		cm.Reset();
		SOAR_PRINT("Log was unsuccessful\n");
		return;
	}
	else if(err == LoggingStatus::LOG_FLASH_NOT_READY || err == LoggingStatus::LOG_FLASH_READY){
		cm.Reset();
		return;
	}

	cm.Reset();

	return;

}


