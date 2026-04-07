/**
 * File Name          : SoarFileSystem.cpp
 * Description        : SOAR File System wrapper for FatFS implementation
 * Author             : SOAR Team
 * Date               : October 8, 2025
 *
 * This file implements a simplified interface for file operations using FatFS.
 * It allows users to create, open, close, read, and write files with USB storage.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "SoarFileSystem.hpp"
#include "app_fatfs.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
    char filename[SOAR_FS_MAX_FILENAME_LEN];
    FIL file_object;
    bool is_open;
} SoarFS_FileHandle_t;

/* Private define ------------------------------------------------------------*/
#define SOAR_FS_DRIVE_PATH "0:/"

/* Private variables ---------------------------------------------------------*/
static bool g_fs_initialized = false;
static bool g_fs_mounted = false;
static SoarFS_FileHandle_t g_file_handles[SOAR_FS_MAX_FILES_OPEN];

/* Private function prototypes -----------------------------------------------*/
static SoarFS_Result_t SoarFS_ConvertFresultToSoarResult(FRESULT fr);
static int SoarFS_FindFreeHandle(void);
static int SoarFS_FindHandleByFilename(const char *filename);
static bool SoarFS_IsValidFilename(const char *filename);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize the SOAR File System
 */
SoarFS_Result_t SoarFS_Init(void)
{
    FRESULT fr;

    if (g_fs_initialized)
    {
        return SOAR_FS_OK;
    }

    // Initialize all file handles
    for (int i = 0; i < SOAR_FS_MAX_FILES_OPEN; i++)
    {
        memset(&g_file_handles[i], 0, sizeof(SoarFS_FileHandle_t));
        g_file_handles[i].is_open = false;
    }

    // Initialize FatFS
    if (MX_FATFS_Init() != APP_OK)
    {
        return SOAR_FS_ERROR;
    }

    // Try to mount the file system
    fr = f_mount(&USERFatFs, USERPath, 1);
    if (fr == FR_OK)
    {
        g_fs_mounted = true;
    }
    else
    {
        g_fs_mounted = false;
        // Don't return error here - device might not be connected yet
    }

    g_fs_initialized = true;
    return SOAR_FS_OK;
}

/**
 * @brief Deinitialize the SOAR File System
 */
SoarFS_Result_t SoarFS_DeInit(void)
{
    if (!g_fs_initialized)
    {
        return SOAR_FS_OK;
    }

    // Close all open files
    SoarFS_CloseAllFiles();

    // Unmount the file system
    f_mount(NULL, USERPath, 0);

    g_fs_mounted = false;
    g_fs_initialized = false;

    return SOAR_FS_OK;
}

/**
 * @brief Check if the file system is mounted and ready
 */
bool SoarFS_IsMounted(void)
{
    if (!g_fs_initialized)
    {
        return false;
    }

    // Try to remount if not mounted
    if (!g_fs_mounted)
    {
        FRESULT fr = f_mount(&USERFatFs, USERPath, 1);
        if (fr == FR_OK)
        {
            g_fs_mounted = true;
        }
    }

    return g_fs_mounted;
}

/**
 * @brief Get available free space on the storage device
 */
SoarFS_Result_t SoarFS_GetFreeSpace(uint32_t *freeBytes)
{
    if (!SoarFS_IsMounted())
    {
        return SOAR_FS_NOT_MOUNTED;
    }

    if (freeBytes == NULL)
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    FATFS *fs;
    DWORD fre_clust;

    FRESULT fr = f_getfree(USERPath, &fre_clust, &fs);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    // Calculate free space in bytes
    *freeBytes = fre_clust * fs->csize * 512; // Sector size is typically 512 bytes

    return SOAR_FS_OK;
}

/**
 * @brief Create a new file with specified content
 */
SoarFS_Result_t SoarFS_CreateFile(const char *filename, const uint8_t *data, uint32_t dataSize)
{
    if (!SoarFS_IsMounted())
    {
        return SOAR_FS_NOT_MOUNTED;
    }

    if (!SoarFS_IsValidFilename(filename) || data == NULL)
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    // Check if file already exists
    if (SoarFS_FileExists(filename))
    {
        return SOAR_FS_FILE_EXISTS;
    }

    // Check available space
    uint32_t freeSpace;
    if (SoarFS_GetFreeSpace(&freeSpace) == SOAR_FS_OK)
    {
        if (dataSize > freeSpace)
        {
            return SOAR_FS_DISK_FULL;
        }
    }

    // Create full path
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "%s%s", SOAR_FS_DRIVE_PATH, filename);

    FIL file;
    FRESULT fr = f_open(&file, fullPath, FA_CREATE_NEW | FA_WRITE);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    // Write data to file
    UINT bytesWritten;
    fr = f_write(&file, data, dataSize, &bytesWritten);
    if (fr != FR_OK || bytesWritten != dataSize)
    {
        f_close(&file);
        f_unlink(fullPath); // Delete partially created file
        return (fr != FR_OK) ? SoarFS_ConvertFresultToSoarResult(fr) : SOAR_FS_ERROR;
    }

    // Close the file
    fr = f_close(&file);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    return SOAR_FS_OK;
}

/**
 * @brief Open an existing file for read/write operations
 */
SoarFS_Result_t SoarFS_OpenFile(const char *filename)
{
    if (!SoarFS_IsMounted())
    {
        return SOAR_FS_NOT_MOUNTED;
    }

    if (!SoarFS_IsValidFilename(filename))
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    // Check if file is already open
    if (SoarFS_FindHandleByFilename(filename) >= 0)
    {
        return SOAR_FS_FILE_ALREADY_OPEN;
    }

    // Find a free handle
    int handle_idx = SoarFS_FindFreeHandle();
    if (handle_idx < 0)
    {
        return SOAR_FS_ERROR; // No free handles
    }

    // Create full path
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "%s%s", SOAR_FS_DRIVE_PATH, filename);

    // Open the file
    FRESULT fr = f_open(&g_file_handles[handle_idx].file_object, fullPath, FA_READ | FA_WRITE);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    // Store filename and mark as open
    strncpy(g_file_handles[handle_idx].filename, filename, SOAR_FS_MAX_FILENAME_LEN - 1);
    g_file_handles[handle_idx].filename[SOAR_FS_MAX_FILENAME_LEN - 1] = '\0';
    g_file_handles[handle_idx].is_open = true;

    return SOAR_FS_OK;
}

/**
 * @brief Close an opened file
 */
SoarFS_Result_t SoarFS_CloseFile(const char *filename)
{
    if (!SoarFS_IsValidFilename(filename))
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    // Find the file handle
    int handle_idx = SoarFS_FindHandleByFilename(filename);
    if (handle_idx < 0)
    {
        return SOAR_FS_FILE_NOT_OPEN;
    }

    // Close the file
    FRESULT fr = f_close(&g_file_handles[handle_idx].file_object);

    // Mark handle as free
    g_file_handles[handle_idx].is_open = false;
    memset(g_file_handles[handle_idx].filename, 0, SOAR_FS_MAX_FILENAME_LEN);

    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    return SOAR_FS_OK;
}

/**
 * @brief Read data from an opened file
 */
SoarFS_Result_t SoarFS_ReadFile(const char *filename, uint8_t *buffer, uint32_t bufferSize, uint32_t *bytesRead)
{
    if (!SoarFS_IsValidFilename(filename) || buffer == NULL || bytesRead == NULL)
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    // Find the file handle
    int handle_idx = SoarFS_FindHandleByFilename(filename);
    if (handle_idx < 0)
    {
        return SOAR_FS_FILE_NOT_OPEN;
    }

    // Read from file
    UINT bytes_read;
    FRESULT fr = f_read(&g_file_handles[handle_idx].file_object, buffer, bufferSize, &bytes_read);

    *bytesRead = bytes_read;

    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    return SOAR_FS_OK;
}

/**
 * @brief Write/append data to an opened file
 */
SoarFS_Result_t SoarFS_WriteFile(const char *filename, const uint8_t *data, uint32_t dataSize)
{
    if (!SoarFS_IsValidFilename(filename) || data == NULL || dataSize == 0)
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    // Find the file handle
    int handle_idx = SoarFS_FindHandleByFilename(filename);
    if (handle_idx < 0)
    {
        return SOAR_FS_FILE_NOT_OPEN;
    }

    // Move to end of file for appending
    FRESULT fr = f_lseek(&g_file_handles[handle_idx].file_object, f_size(&g_file_handles[handle_idx].file_object));
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    // Write data to file
    UINT bytesWritten;
    fr = f_write(&g_file_handles[handle_idx].file_object, data, dataSize, &bytesWritten);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    if (bytesWritten != dataSize)
    {
        return SOAR_FS_DISK_FULL;
    }

    // Sync file to ensure data is written
    fr = f_sync(&g_file_handles[handle_idx].file_object);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    return SOAR_FS_OK;
}

/**
 * @brief Delete a file from the storage
 */
SoarFS_Result_t SoarFS_DeleteFile(const char *filename)
{
    if (!SoarFS_IsMounted())
    {
        return SOAR_FS_NOT_MOUNTED;
    }

    if (!SoarFS_IsValidFilename(filename))
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    // Close file if it's open
    int handle_idx = SoarFS_FindHandleByFilename(filename);
    if (handle_idx >= 0)
    {
        SoarFS_CloseFile(filename);
    }

    // Create full path
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "%s%s", SOAR_FS_DRIVE_PATH, filename);

    // Delete the file
    FRESULT fr = f_unlink(fullPath);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    return SOAR_FS_OK;
}

/**
 * @brief Check if a file exists
 */
bool SoarFS_FileExists(const char *filename)
{
    if (!SoarFS_IsMounted() || !SoarFS_IsValidFilename(filename))
    {
        return false;
    }

    // Create full path
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "%s%s", SOAR_FS_DRIVE_PATH, filename);

    FILINFO fno;
    FRESULT fr = f_stat(fullPath, &fno);

    return (fr == FR_OK);
}

/**
 * @brief Get file size
 */
SoarFS_Result_t SoarFS_GetFileSize(const char *filename, uint32_t *fileSize)
{
    if (!SoarFS_IsMounted())
    {
        return SOAR_FS_NOT_MOUNTED;
    }

    if (!SoarFS_IsValidFilename(filename) || fileSize == NULL)
    {
        return SOAR_FS_INVALID_PARAMETER;
    }

    // Create full path
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "%s%s", SOAR_FS_DRIVE_PATH, filename);

    FILINFO fno;
    FRESULT fr = f_stat(fullPath, &fno);
    if (fr != FR_OK)
    {
        return SoarFS_ConvertFresultToSoarResult(fr);
    }

    *fileSize = fno.fsize;
    return SOAR_FS_OK;
}

/**
 * @brief Close all open files
 */
SoarFS_Result_t SoarFS_CloseAllFiles(void)
{
    SoarFS_Result_t result = SOAR_FS_OK;

    for (int i = 0; i < SOAR_FS_MAX_FILES_OPEN; i++)
    {
        if (g_file_handles[i].is_open)
        {
            FRESULT fr = f_close(&g_file_handles[i].file_object);
            g_file_handles[i].is_open = false;
            memset(g_file_handles[i].filename, 0, SOAR_FS_MAX_FILENAME_LEN);

            if (fr != FR_OK && result == SOAR_FS_OK)
            {
                result = SoarFS_ConvertFresultToSoarResult(fr);
            }
        }
    }

    return result;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Convert FatFS FRESULT to SoarFS_Result_t
 */
static SoarFS_Result_t SoarFS_ConvertFresultToSoarResult(FRESULT fr)
{
    switch (fr)
    {
    case FR_OK:
        return SOAR_FS_OK;
    case FR_NO_FILE:
    case FR_NO_PATH:
        return SOAR_FS_FILE_NOT_FOUND;
    case FR_EXIST:
        return SOAR_FS_FILE_EXISTS;
    case FR_DISK_ERR:
    case FR_NOT_READY:
        return SOAR_FS_NOT_MOUNTED;
    case FR_WRITE_PROTECTED:
        return SOAR_FS_WRITE_PROTECTED;
    case FR_TIMEOUT:
        return SOAR_FS_TIMEOUT;
    case FR_INVALID_PARAMETER:
    case FR_INVALID_NAME:
        return SOAR_FS_INVALID_PARAMETER;
    default:
        return SOAR_FS_ERROR;
    }
}

/**
 * @brief Find a free file handle
 */
static int SoarFS_FindFreeHandle(void)
{
    for (int i = 0; i < SOAR_FS_MAX_FILES_OPEN; i++)
    {
        if (!g_file_handles[i].is_open)
        {
            return i;
        }
    }
    return -1; // No free handles
}

/**
 * @brief Find file handle by filename
 */
static int SoarFS_FindHandleByFilename(const char *filename)
{
    for (int i = 0; i < SOAR_FS_MAX_FILES_OPEN; i++)
    {
        if (g_file_handles[i].is_open && strcmp(g_file_handles[i].filename, filename) == 0)
        {
            return i;
        }
    }
    return -1; // Not found
}

/**
 * @brief Check if filename is valid
 */
static bool SoarFS_IsValidFilename(const char *filename)
{
    if (filename == NULL || strlen(filename) == 0 || strlen(filename) >= SOAR_FS_MAX_FILENAME_LEN)
    {
        return false;
    }

    // Check for invalid characters
    const char *invalid_chars = "\\/:*?\"<>|";
    for (int i = 0; invalid_chars[i] != '\0'; i++)
    {
        if (strchr(filename, invalid_chars[i]) != NULL)
        {
            return false;
        }
    }

    return true;
}
