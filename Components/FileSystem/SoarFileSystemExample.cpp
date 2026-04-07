/**
 * File Name          : SoarFileSystemExample.cpp
 * Description        : Example usage of SOAR File System wrapper
 * Author             : SOAR Team
 * Date               : October 8, 2025
 *
 * This file demonstrates how to use the SOAR File System wrapper functions
 * for creating, reading, writing, and managing files on USB storage.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "SoarFileSystem.hpp"
#include <string.h>
#include <cstdio>
/* Example usage functions ---------------------------------------------------*/

/**
 * @brief Example function demonstrating file system initialization
 */
void SoarFS_Example_Init(void)
{
    SoarFS_Result_t result = SoarFS_Init();

    if (result == SOAR_FS_OK)
    {
        // File system initialized successfully
        if (SoarFS_IsMounted())
        {
            // USB storage is connected and ready
            uint32_t freeSpace;
            if (SoarFS_GetFreeSpace(&freeSpace) == SOAR_FS_OK)
            {
                // Display available space (freeSpace contains bytes available)
            }
        }
        else
        {
            // USB storage not connected
        }
    }
    else
    {
        // Initialization failed
    }
}

/**
 * @brief Example function demonstrating file creation
 */
void SoarFS_Example_CreateFile(void)
{
    // Example data to write
    const char *sampleData = "Hello, SOAR File System!\\nThis is a test file.";

    SoarFS_Result_t result = SoarFS_CreateFile("test.txt", (const uint8_t *)sampleData, strlen(sampleData));

    switch (result)
    {
    case SOAR_FS_OK:
        // File created successfully
        break;
    case SOAR_FS_FILE_EXISTS:
        // File already exists
        break;
    case SOAR_FS_DISK_FULL:
        // Not enough space on storage device
        break;
    case SOAR_FS_NOT_MOUNTED:
        // USB storage not connected
        break;
    default:
        // Other error occurred
        break;
    }
}

/**
 * @brief Example function demonstrating file operations (open, read, write, close)
 */
void SoarFS_Example_FileOperations(void)
{
    const char *filename = "data.log";

    // Check if file exists, create if it doesn't
    if (!SoarFS_FileExists(filename))
    {
        const char *initialData = "SOAR Data Log\\n";
        SoarFS_CreateFile(filename, (const uint8_t *)initialData, strlen(initialData));
    }

    // Open the file
    SoarFS_Result_t result = SoarFS_OpenFile(filename);
    if (result != SOAR_FS_OK)
    {
        return; // Failed to open file
    }

    // Read existing content
    uint8_t readBuffer[256];
    uint32_t bytesRead;
    result = SoarFS_ReadFile(filename, readBuffer, sizeof(readBuffer) - 1, &bytesRead);
    if (result == SOAR_FS_OK)
    {
        readBuffer[bytesRead] = '\\0'; // Null terminate for string operations
        // Process the read data
    }

    // Append new data
    const char *newData = "New log entry\\n";
    result = SoarFS_WriteFile(filename, (const uint8_t *)newData, strlen(newData));
    if (result == SOAR_FS_OK)
    {
        // Data appended successfully
    }

    // Close the file
    SoarFS_CloseFile(filename);
}

/**
 * @brief Example function demonstrating sensor data logging
 */
void SoarFS_Example_LogSensorData(float temperature, float humidity, uint32_t timestamp)
{
    const char *logFile = "sensors.csv";

    // Create CSV header if file doesn't exist
    if (!SoarFS_FileExists(logFile))
    {
        const char *header = "Timestamp,Temperature,Humidity\\n";
        SoarFS_CreateFile(logFile, (const uint8_t *)header, strlen(header));
    }

    // Open file for appending
    if (SoarFS_OpenFile(logFile) == SOAR_FS_OK)
    {
        // Format data as CSV
        char dataLine[128];
        //snprintf(dataLine, sizeof(dataLine), "%lu,%.2f,%.2f\\n", timestamp, temperature, humidity);

        // Write data
        SoarFS_WriteFile(logFile, (const uint8_t *)dataLine, strlen(dataLine));

        // Close file
        SoarFS_CloseFile(logFile);
    }
}

/**
 * @brief Example function demonstrating binary data storage
 */
void SoarFS_Example_StoreBinaryData(void)
{
    // Example: Store flight data structure
    typedef struct
    {
        uint32_t timestamp;
        float altitude;
        float velocity;
        float acceleration[3]; // x, y, z
        uint16_t battery_voltage;
    } FlightData_t;

    FlightData_t flightData = {
        .timestamp = 12345678,
        .altitude = 1000.5f,
        .velocity = 25.8f,
        .acceleration = {0.1f, 0.2f, 9.8f},
        .battery_voltage = 3700 // mV
    };

    const char *filename = "flight_data.bin";

    // Create binary file
    SoarFS_Result_t result = SoarFS_CreateFile(filename, (const uint8_t *)&flightData, sizeof(FlightData_t));

    if (result == SOAR_FS_OK)
    {
        // Binary data stored successfully

        // Later, read it back
        if (SoarFS_OpenFile(filename) == SOAR_FS_OK)
        {
            FlightData_t readData;
            uint32_t bytesRead;

            SoarFS_ReadFile(filename, (uint8_t *)&readData, sizeof(FlightData_t), &bytesRead);

            if (bytesRead == sizeof(FlightData_t))
            {
                // Data read successfully, verify integrity
                if (readData.timestamp == flightData.timestamp &&
                    readData.altitude == flightData.altitude)
                {
                    // Data integrity verified
                }
            }

            SoarFS_CloseFile(filename);
        }
    }
}

/**
 * @brief Example function demonstrating file management
 */
void SoarFS_Example_FileManagement(void)
{
    const char *filename = "temp_file.txt";

    // Check file size
    if (SoarFS_FileExists(filename))
    {
        uint32_t fileSize;
        if (SoarFS_GetFileSize(filename, &fileSize) == SOAR_FS_OK)
        {
            // File size obtained in bytes

            // If file is too large, delete it
            if (fileSize > 1024)
            { // 1KB limit
                SoarFS_DeleteFile(filename);
            }
        }
    }

    // Emergency: Close all open files
    SoarFS_CloseAllFiles();

    // Get available space
    uint32_t freeBytes;
    if (SoarFS_GetFreeSpace(&freeBytes) == SOAR_FS_OK)
    {
        // Check if we have enough space for critical operations
        if (freeBytes < 1024)
        { // Less than 1KB free
            // Implement cleanup or alert mechanism
        }
    }
}

/**
 * @brief Example error handling function
 */
void SoarFS_Example_ErrorHandling(SoarFS_Result_t result)
{
    switch (result)
    {
    case SOAR_FS_OK:
        // Operation successful
        break;

    case SOAR_FS_ERROR:
        // Generic error - check hardware connections
        break;

    case SOAR_FS_NOT_MOUNTED:
        // USB device not connected or not ready
        // Try re-initializing or wait for device
        SoarFS_Init();
        break;

    case SOAR_FS_FILE_NOT_FOUND:
        // File doesn't exist - create it or handle accordingly
        break;

    case SOAR_FS_FILE_EXISTS:
        // File already exists - decide whether to overwrite
        break;

    case SOAR_FS_DISK_FULL:
        // Storage device is full - cleanup old files
        break;

    case SOAR_FS_INVALID_PARAMETER:
        // Check function parameters
        break;

    case SOAR_FS_FILE_ALREADY_OPEN:
        // File is already open - close it first
        break;

    case SOAR_FS_FILE_NOT_OPEN:
        // File must be opened before read/write operations
        break;

    case SOAR_FS_WRITE_PROTECTED:
        // Storage device is write-protected
        break;

    case SOAR_FS_TIMEOUT:
        // Operation timed out - retry or check hardware
        break;

    default:
        // Unknown error
        break;
    }
}
