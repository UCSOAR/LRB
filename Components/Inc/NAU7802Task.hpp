/**
 ******************************************************************************
 * File Name          : NAU7802Task.hpp
 * Description        : NAU7802 sensor polling task
 ******************************************************************************
*/
#ifndef COMPONENTS_NAU7802_TASK_HPP_
#define COMPONENTS_NAU7802_TASK_HPP_

/* Includes ------------------------------------------------------------------*/
#include <i2c_wrapper.hpp>
#include <NAU7802.hpp>
#include "SystemDefines.hpp"
#include "../../SoarOS/Core/Inc/Task.hpp"

/* Macros ------------------------------------------------------------------*/
enum NAUTASK_COMMANDS {
    NAUTASK_COMMAND_NONE = 0,
    NAUTASK_COMMAND_NAU_TOGGLE,
    NAUTASK_COMMAND_NAU_ON,
    NAUTASK_COMMAND_NAU_OFF,
    NAUTASK_COMMAND_NAU_STATUS,
    NAUTASK_COMMAND_NAU_ENABLE_LOG,
    NAUTASK_COMMAND_NAU_DISABLE_LOG,
    NAUTASK_COMMAND_NAU_READ,
    NAUTASK_COMMAND_NAU_READ_ISR,
    NAUTASK_COMMAND_NAU_DRDY,
    NAUTASK_COMMAND_NAU_DUMP_REGS,
    NAUTASK_COMMAND_NAU_TARE,
    NAUTASK_COMMAND_NAU_BASELINE,
    NAUTASK_COMMAND_NAU_SET_GAIN_1X,
    NAUTASK_COMMAND_NAU_SET_GAIN_2X,
    NAUTASK_COMMAND_NAU_SET_GAIN_4X,
    NAUTASK_COMMAND_NAU_SET_GAIN_8X,
    NAUTASK_COMMAND_NAU_SET_GAIN_128,
    NAUTASK_COMMAND_NAU_BUSCHK,
    NAUTASK_COMMAND_MAX
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
    void SetTaskActive(bool enabled);
    void ToggleTaskActive();
    void SetLoggingEnabled(bool enabled);
    bool IsTaskActive() const { return _enableReading; }
    void PrintStatus();
    void PrintDrdy();

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
    bool _outputInKg;
    long _tareCounts;
    bool _baselineRunning;
    unsigned int _baselineSamples;
    long long rawToMilliKg(long rawReading) const;
    long long milliKgToMilliLb(long long milliKg) const;
};

#endif /* COMPONENTS_NAU7802_TASK_HPP_ */
