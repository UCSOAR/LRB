/**
 * File Name          : SoarFileSystemExample.hpp
 * Description        : Example usage functions for SOAR File System wrapper
 * Author             : SOAR Team
 * Date               : October 8, 2025
 *
 * This file provides function prototypes for the SOAR File System examples.
 ******************************************************************************
 */

#ifndef __SOAR_FILE_SYSTEM_EXAMPLE_HPP
#define __SOAR_FILE_SYSTEM_EXAMPLE_HPP

/* Includes ------------------------------------------------------------------*/
#include "SoarFileSystem.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Exported function prototypes ----------------------------------------------*/

    /**
     * @brief Example function demonstrating file system initialization
     */
    void SoarFS_Example_Init(void);

    /**
     * @brief Example function demonstrating file creation
     */
    void SoarFS_Example_CreateFile(void);

    /**
     * @brief Example function demonstrating file operations (open, read, write, close)
     */
    void SoarFS_Example_FileOperations(void);

    /**
     * @brief Example function demonstrating sensor data logging
     * @param temperature Temperature value to log
     * @param humidity Humidity value to log
     * @param timestamp Timestamp for the log entry
     */
    void SoarFS_Example_LogSensorData(float temperature, float humidity, uint32_t timestamp);

    /**
     * @brief Example function demonstrating binary data storage
     */
    void SoarFS_Example_StoreBinaryData(void);

    /**
     * @brief Example function demonstrating file management
     */
    void SoarFS_Example_FileManagement(void);

    /**
     * @brief Example error handling function
     * @param result The result code to handle
     */
    void SoarFS_Example_ErrorHandling(SoarFS_Result_t result);

#ifdef __cplusplus
}
#endif

#endif /* __SOAR_FILE_SYSTEM_EXAMPLE_HPP */