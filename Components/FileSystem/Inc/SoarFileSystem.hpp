/**
 * File Name          : SoarFileSystem.hpp
 * Description        : SOAR File System wrapper for FatFS
 * Author             : SOAR Team
 * Date               : October 8, 2025
 *
 * This file provides a simplified interface for file operations using FatFS.
 * It allows users to create, open, close, read, and write files with USB storage.
 ******************************************************************************
 */

#ifndef __SOAR_FILE_SYSTEM_HPP
#define __SOAR_FILE_SYSTEM_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Exported types ------------------------------------------------------------*/
    typedef enum
    {
        SOAR_FS_OK = 0,
        SOAR_FS_ERROR = -1,
        SOAR_FS_NOT_MOUNTED = -2,
        SOAR_FS_FILE_NOT_FOUND = -3,
        SOAR_FS_FILE_EXISTS = -4,
        SOAR_FS_DISK_FULL = -5,
        SOAR_FS_INVALID_PARAMETER = -6,
        SOAR_FS_FILE_ALREADY_OPEN = -7,
        SOAR_FS_FILE_NOT_OPEN = -8,
        SOAR_FS_WRITE_PROTECTED = -9,
        SOAR_FS_TIMEOUT = -10
    } SoarFS_Result_t;

/* Exported constants --------------------------------------------------------*/
#define SOAR_FS_MAX_FILENAME_LEN 32
#define SOAR_FS_MAX_FILES_OPEN 4
#define SOAR_FS_BUFFER_SIZE 512

    /* Exported function prototypes ----------------------------------------------*/

    /**
     * @brief Initialize the SOAR File System
     * @retval SoarFS_Result_t Status of initialization
     */
    SoarFS_Result_t SoarFS_Init(void);

    /**
     * @brief Deinitialize the SOAR File System
     * @retval SoarFS_Result_t Status of deinitialization
     */
    SoarFS_Result_t SoarFS_DeInit(void);

    /**
     * @brief Check if the file system is mounted and ready
     * @retval bool True if mounted, false otherwise
     */
    bool SoarFS_IsMounted(void);

    /**
     * @brief Get available free space on the storage device
     * @param freeBytes Pointer to store free bytes available
     * @retval SoarFS_Result_t Status of operation
     */
    SoarFS_Result_t SoarFS_GetFreeSpace(uint32_t *freeBytes);

    /**
     * @brief Create a new file with specified content
     * @param filename Name of the file to create (max 32 chars)
     * @param data Pointer to data to write to the file
     * @param dataSize Size of data in bytes
     * @retval SoarFS_Result_t Status of file creation
     */
    SoarFS_Result_t SoarFS_CreateFile(const char *filename, const uint8_t *data, uint32_t dataSize);

    /**
     * @brief Open an existing file for read/write operations
     * @param filename Name of the file to open
     * @retval SoarFS_Result_t Status of file opening
     */
    SoarFS_Result_t SoarFS_OpenFile(const char *filename);

    /**
     * @brief Close an opened file
     * @param filename Name of the file to close
     * @retval SoarFS_Result_t Status of file closing
     */
    SoarFS_Result_t SoarFS_CloseFile(const char *filename);

    /**
     * @brief Read data from an opened file
     * @param filename Name of the file to read from
     * @param buffer Pointer to buffer to store read data
     * @param bufferSize Size of the buffer
     * @param bytesRead Pointer to store actual bytes read
     * @retval SoarFS_Result_t Status of read operation
     */
    SoarFS_Result_t SoarFS_ReadFile(const char *filename, uint8_t *buffer, uint32_t bufferSize, uint32_t *bytesRead);

    /**
     * @brief Write/append data to an opened file
     * @param filename Name of the file to write to
     * @param data Pointer to data to append
     * @param dataSize Size of data in bytes
     * @retval SoarFS_Result_t Status of write operation
     */
    SoarFS_Result_t SoarFS_WriteFile(const char *filename, const uint8_t *data, uint32_t dataSize);

    /**
     * @brief Delete a file from the storage
     * @param filename Name of the file to delete
     * @retval SoarFS_Result_t Status of delete operation
     */
    SoarFS_Result_t SoarFS_DeleteFile(const char *filename);

    /**
     * @brief Check if a file exists
     * @param filename Name of the file to check
     * @retval bool True if file exists, false otherwise
     */
    bool SoarFS_FileExists(const char *filename);

    /**
     * @brief Get file size
     * @param filename Name of the file
     * @param fileSize Pointer to store file size
     * @retval SoarFS_Result_t Status of operation
     */
    SoarFS_Result_t SoarFS_GetFileSize(const char *filename, uint32_t *fileSize);

    /**
     * @brief Close all open files
     * @retval SoarFS_Result_t Status of operation
     */
    SoarFS_Result_t SoarFS_CloseAllFiles(void);

#ifdef __cplusplus
}
#endif

#endif /* __SOAR_FILE_SYSTEM_HPP */