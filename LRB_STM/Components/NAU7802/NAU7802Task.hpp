/**
 ********************************************************************************
 * @file    NAU7802Task.hpp
 * @author  Javier
 * @brief   FreeRTOS Task Wrapper for the NAU7802 ADC Driver
 ********************************************************************************
 */

#ifndef NAU7802_TASK_HPP_
#define NAU7802_TASK_HPP_

/************************************
 * INCLUDES
 ************************************/
#include "main.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// system includes
#include "SystemDefines.hpp"
#include "Task.hpp"

// Driver Includes
#include "NAU7802.hpp"
#include "i2c_wrapper.hpp"

/************************************
 * CLASS DEFINITIONS
 ************************************/
class NAU7802Task : public Task
{
public:
    static NAU7802Task& Inst(){
        static NAU7802Task inst;
        return inst;
    }
    
    // Command Definitions:
    enum NAU7802_Commands {
        NAU_CMD_START_READ   = 1, // Start reading data
        NAU_CMD_STOP_READ    = 2, // Stop reading data
        NAU_CMD_DISABLE_LOG  = 3, // Disable logging
        NAU_CMD_ENABLE_LOG   = 4  // Enable logging
    };



    void Init(I2C_HandleTypeDef* hi2c);

    void InitTask();

protected:
    static void RunTask(void* pvParams) { NAU7802Task::Inst().Run(pvParams); }
    void Run(void* pvParams);
    void HandleCommand(Command& cm);

private:
    // Constructors
    NAU7802Task();
    NAU7802Task(const NAU7802Task&);
    NAU7802Task& operator=(const NAU7802Task&);

    // Obj to allow delayed inits
    I2C_Wrapper* _i2c_wrapper;
    NAU7802* _adc;

    // Task Control Flags
    bool _enableReading;
    bool _enableLogging;
};

#endif /* NAU7802_TASK_HPP_ */
