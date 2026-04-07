/**
 ******************************************************************************
 * File Name          : FileSystemTask.cpp
 * Description        : File System Task implementation for USB storage operations
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "FileSystemTask.hpp"
#include "SoarFileSystemExample.hpp"
#include "SoarFileSystem.hpp"
#include "SystemDefines.hpp"
#include <stdint.h>
#include "stm32g4xx_hal.h"

/**
 * @brief Constructor, sets up task
 */
FileSystemTask::FileSystemTask() : Task(TASK_FILESYSTEM_QUEUE_DEPTH_OBJS),
                                   fileSystemInitialized(false),
                                   usbMounted(false),
                                   lastLogTime(0),
                                   lastCleanupTime(0),
                                   testCounter(0)
{
    // Initialize pending log data
    pendingLogData.hasData = false;
    pendingLogData.temperature = 0.0f;
    pendingLogData.humidity = 0.0f;
    pendingLogData.timestamp = 0;
}

/**
 * @brief Initialize the FileSystemTask
 */
void FileSystemTask::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize FileSystem task twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)FileSystemTask::RunTask,
                    (const char *)"FileSystemTask",
                    (uint16_t)TASK_FILESYSTEM_STACK_DEPTH_WORDS,
                    (void *)this,
                    (UBaseType_t)TASK_FILESYSTEM_TASK_PRIORITY,
                    (TaskHandle_t *)&rtTaskHandle);

    // Ensure creation succeded
    SOAR_ASSERT(rtValue == pdPASS, "FileSystemTask::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Instance Run loop for the FileSystem Task, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
 */
void FileSystemTask::Run(void *pvParams)
{
    SOAR_PRINT("FileSystemTask::Run() - Starting task\n");

    // Initialize file system on startup
    InitializeFileSystem();

    while (1)
    {
        /* Process commands in blocking mode */
        Command cm;
        bool res = qEvtQueue->ReceiveWait(cm);
        if (res)
        {
            HandleCommand(cm);
        }
    }
}

/**
 * @brief Handles a command
 * @param cm Command reference to handle
 */
void FileSystemTask::HandleCommand(Command &cm)
{
    // Handle task-specific commands
    if (cm.GetCommand() == TASK_SPECIFIC_COMMAND)
    {
        switch (cm.GetTaskCommand())
        {
        case EVENT_FILESYSTEM_INIT:
            InitializeFileSystem();
            break;
        case EVENT_FILESYSTEM_TEST:
            RunFileSystemTests();
            break;
        case EVENT_FILESYSTEM_LOG_DATA:
            if (pendingLogData.hasData)
            {
                LogSensorData(pendingLogData.temperature, pendingLogData.humidity, pendingLogData.timestamp);
                pendingLogData.hasData = false;
            }
            break;
        case EVENT_FILESYSTEM_CLEANUP:
            PerformCleanup();
            break;
        default:
            SOAR_PRINT("FileSystemTask - Received Unsupported Task Command {%d}\n", cm.GetTaskCommand());
            break;
        }
    }
    else
    {
        // Handle global commands if any
        SOAR_PRINT("FileSystemTask - Received Unsupported Global Command {%d}\n", cm.GetCommand());
    }

    // No matter what happens, we must reset allocated data
    cm.Reset();
}

/**
 * @brief Initialize the file system
 */
void FileSystemTask::InitializeFileSystem()
{
    SOAR_PRINT("FileSystemTask::InitializeFileSystem() - Initializing file system\n");

    SoarFS_Result_t result = SoarFS_Init();
    if (result == SOAR_FS_OK)
    {
        fileSystemInitialized = true;
        SOAR_PRINT("FileSystemTask::InitializeFileSystem() - File system initialized successfully\n");

        // Wait for USB to mount
        WaitForUSBMount();
    }
    else
    {
        SOAR_PRINT("FileSystemTask::InitializeFileSystem() - Failed to initialize file system: %d\n", result);
        fileSystemInitialized = false;
    }
}

/**
 * @brief Run file system tests
 */
void FileSystemTask::RunFileSystemTests()
{
    if (!IsFileSystemReady())
    {
        SOAR_PRINT("FileSystemTask::RunFileSystemTests() - File system not ready\n");
        return;
    }

    SOAR_PRINT("FileSystemTask::RunFileSystemTests() - Running file system tests\n");

    // Run example functions
    SoarFS_Example_CreateFile();
    SoarFS_Example_FileOperations();
    SoarFS_Example_StoreBinaryData();

    testCounter++;
    SOAR_PRINT("FileSystemTask::RunFileSystemTests() - Tests completed (run #%lu)\n", testCounter);
}

/**
 * @brief Log sensor data to file
 */
void FileSystemTask::LogSensorData(float temperature, float humidity, uint32_t timestamp)
{
    if (!IsFileSystemReady())
    {
        SOAR_PRINT("FileSystemTask::LogSensorData() - File system not ready\n");
        return;
    }

    SOAR_PRINT("FileSystemTask::LogSensorData() - Logging sensor data: T=%.2f, H=%.2f\n", temperature, humidity);
    SoarFS_Example_LogSensorData(temperature, humidity, timestamp);
    lastLogTime = HAL_GetTick();
}

/**
 * @brief Perform file system cleanup operations
 */
void FileSystemTask::PerformCleanup()
{
    if (!IsFileSystemReady())
    {
        return;
    }

    SOAR_PRINT("FileSystemTask::PerformCleanup() - Performing cleanup\n");
    SoarFS_Example_FileManagement();
}

/**
 * @brief Check USB connection status
 */
void FileSystemTask::CheckUSBStatus()
{
    bool currentStatus = SoarFS_IsMounted();

    if (currentStatus != usbMounted)
    {
        if (currentStatus)
        {
            SOAR_PRINT("FileSystemTask::CheckUSBStatus() - USB storage connected\n");

            // Get and display free space
            uint32_t freeBytes;
            if (SoarFS_GetFreeSpace(&freeBytes) == SOAR_FS_OK)
            {
                SOAR_PRINT("FileSystemTask::CheckUSBStatus() - Available space: %lu bytes\n", freeBytes);
            }
        }
        else
        {
            SOAR_PRINT("FileSystemTask::CheckUSBStatus() - USB storage disconnected\n");
        }
        usbMounted = currentStatus;
    }
}

/**
 * @brief Wait for USB to mount with timeout
 */
void FileSystemTask::WaitForUSBMount(uint32_t maxWaitMs)
{
    SOAR_PRINT("FileSystemTask::WaitForUSBMount() - Waiting for USB storage...\n");

    uint32_t startTime = HAL_GetTick();
    uint32_t lastPrintTime = startTime;

    while (!SoarFS_IsMounted() && (HAL_GetTick() - startTime) < maxWaitMs)
    {
        // Print status every 5 seconds
        if (HAL_GetTick() - lastPrintTime > 5000)
        {
            uint32_t elapsed = (HAL_GetTick() - startTime) / 1000;
            SOAR_PRINT("FileSystemTask::WaitForUSBMount() - Still waiting... (%lu/%lu seconds)\n",
                       elapsed, maxWaitMs / 1000);
            lastPrintTime = HAL_GetTick();
        }

        osDelay(500); // Check every 500ms
    }

    if (SoarFS_IsMounted())
    {
        usbMounted = true;
        SOAR_PRINT("FileSystemTask::WaitForUSBMount() - USB storage mounted successfully\n");
    }
    else
    {
        SOAR_PRINT("FileSystemTask::WaitForUSBMount() - Timeout waiting for USB storage\n");
    }
}

/**
 * @brief Check if file system is ready for operations
 */
bool FileSystemTask::IsFileSystemReady()
{
    // Update USB status
    CheckUSBStatus();
    return fileSystemInitialized && usbMounted;
}

/**
 * @brief Trigger file system tests from external task
 */
void FileSystemTask::TriggerTest()
{
    Command cm(TASK_SPECIFIC_COMMAND, EVENT_FILESYSTEM_TEST);
    qEvtQueue->Send(cm);
}

/**
 * @brief Trigger sensor data logging from external task
 */
void FileSystemTask::TriggerLogData(float temperature, float humidity, uint32_t timestamp)
{
    pendingLogData.temperature = temperature;
    pendingLogData.humidity = humidity;
    pendingLogData.timestamp = timestamp;
    pendingLogData.hasData = true;

    Command cm(TASK_SPECIFIC_COMMAND, EVENT_FILESYSTEM_LOG_DATA);
    qEvtQueue->Send(cm);
}

/**
 * @brief Trigger cleanup from external task
 */
void FileSystemTask::TriggerCleanup()
{
    Command cm(TASK_SPECIFIC_COMMAND, EVENT_FILESYSTEM_CLEANUP);
    qEvtQueue->Send(cm);
}
