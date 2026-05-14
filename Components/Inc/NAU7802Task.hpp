/**
 ******************************************************************************
 * File Name          : NAU7802Task.hpp
 * Description        : NAU7802 sensor polling task
 ******************************************************************************
*/
#ifndef COMPONENTS_NAU7802_TASK_HPP_
#define COMPONENTS_NAU7802_TASK_HPP_

/* Includes ------------------------------------------------------------------*/
#include <NAU7802.h>
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
    NAUTASK_COMMAND_NAU_SET_GAIN_1X,
    NAUTASK_COMMAND_NAU_SET_GAIN_2X,
    NAUTASK_COMMAND_NAU_SET_GAIN_4X,
    NAUTASK_COMMAND_NAU_SET_GAIN_8X,
    NAUTASK_COMMAND_NAU_SET_GAIN_128,
    NAUTASK_COMMAND_NAU_TARE,
    NAUTASK_COMMAND_NAU_CAL_500G,
    NAUTASK_COMMAND_NAU_CAL_1000G,
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

    Adafruit_NAU7802 _adc;

    bool _enableReading;
    bool _enableLogging;
    bool _sensorReady;
    float _emaAlpha;
    float _emaValue;
    bool _emaInitialized;
    bool _calibHave500g;
    bool _calibHave1000g;
    int32_t _calibRaw500g;
    int32_t _calibRaw1000g;
    float _calibSlopeGPerCount;
    float _calibOffsetG;
    bool _calibValid;
    float _tareGrams;
    bool _tareValid;
};

#endif /* COMPONENTS_NAU7802_TASK_HPP_ */
