/**
 ******************************************************************************
 * File Name          : NAU7802Task.hpp
 * Description        : NAU7802 sensor polling task
 ******************************************************************************
*/
#ifndef COMPONENTS_NAU7802_TASK_HPP_
#define COMPONENTS_NAU7802_TASK_HPP_

/* Includes ------------------------------------------------------------------*/
#include "Task.hpp"
#include "SystemDefines.hpp"
#include "NAU7802.hpp"
#include "i2c_wrapper.hpp"

/* Macros ------------------------------------------------------------------*/
enum TASK1_COMMANDS {
    TASK1_COMMAND_NONE = 0,
    TASK1_COMMAND_NAU_TOGGLE,
    TASK1_COMMAND_NAU_ON,
    TASK1_COMMAND_NAU_OFF,
    TASK1_COMMAND_NAU_STATUS,
    TASK1_COMMAND_NAU_ENABLE_LOG,
    TASK1_COMMAND_NAU_DISABLE_LOG,
    TASK1_COMMAND_MAX
};

/* Class ------------------------------------------------------------------*/
class NAU7802Task : public Task
{
public:
    static NAU7802Task& Inst() {
        static NAU7802Task inst;
        return inst;
    }

    void InitTask();

protected:
    static void RunTask(void* pvParams) { NAU7802Task::Inst().Run(pvParams); }

    void Run(void* pvParams);

    void HandleCommand(Command& cm);

private:
    NAU7802Task();
    NAU7802Task(const NAU7802Task&);
    NAU7802Task& operator=(const NAU7802Task&);

    I2C_Wrapper _i2cWrapper;
    NAU7802 _adc;

    bool _enableReading;
    bool _enableLogging;
    bool _sensorReady;
};

#endif /* COMPONENTS_NAU7802_TASK_HPP_ */