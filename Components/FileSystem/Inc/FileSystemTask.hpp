/**
 ******************************************************************************
 * File Name          : FileSystemTask.hpp
 * Description        : File System Task for managing USB storage operations
 ******************************************************************************
 */
#ifndef CUBE_SYSTEM_FILESYSTEM_TASK_HPP_
#define CUBE_SYSTEM_FILESYSTEM_TASK_HPP_

/* Includes ------------------------------------------------------------------*/
#include "SystemDefines.hpp"
#include "SoarFileSystem.hpp"
#include <stdint.h>
#include "../../../SoarOS/Core/Inc/Task.hpp"

/* Enums ------------------------------------------------------------------*/
enum FILESYSTEM_TASK_COMMANDS
{
    FILESYSTEM_TASK_COMMAND_NONE = 0,
    EVENT_FILESYSTEM_INIT,
    EVENT_FILESYSTEM_TEST,
    EVENT_FILESYSTEM_LOG_DATA,
    EVENT_FILESYSTEM_CLEANUP
};

/* Macros ------------------------------------------------------------------*/
constexpr uint32_t FILESYSTEM_LOG_INTERVAL_MS = 10000;     // Log every 10 seconds
constexpr uint32_t FILESYSTEM_CLEANUP_INTERVAL_MS = 60000; // Cleanup every minute

/* Class ------------------------------------------------------------------*/
class FileSystemTask : public Task
{
public:
    static FileSystemTask &Inst()
    {
        static FileSystemTask inst;
        return inst;
    }

    void InitTask();

    // Public interface for other tasks to trigger operations
    void TriggerTest();
    void TriggerLogData(float temperature, float humidity, uint32_t timestamp);
    void TriggerCleanup();

protected:
    static void RunTask(void *pvParams)
    {
        FileSystemTask::Inst().Run(pvParams);
    } // Static Task Interface, passes control to the instance Run();

    void Run(void *pvParams); // Main run code
    void HandleCommand(Command &cm);

private:
    // Private Functions
    FileSystemTask();                                  // Private constructor
    FileSystemTask(const FileSystemTask &);            // Prevent copy-construction
    FileSystemTask &operator=(const FileSystemTask &); // Prevent assignment

    // Task operation functions
    void InitializeFileSystem();
    void RunFileSystemTests();
    void LogSensorData(float temperature, float humidity, uint32_t timestamp);
    void PerformCleanup();
    void CheckUSBStatus();

    // Helper functions
    void WaitForUSBMount(uint32_t maxWaitMs = 30000);
    bool IsFileSystemReady();

    // Member variables
    bool fileSystemInitialized;
    bool usbMounted;
    uint32_t lastLogTime;
    uint32_t lastCleanupTime;
    uint32_t testCounter;

    // Data for logging
    struct
    {
        float temperature;
        float humidity;
        uint32_t timestamp;
        bool hasData;
    } pendingLogData;
};

#endif // CUBE_SYSTEM_FILESYSTEM_TASK_HPP_