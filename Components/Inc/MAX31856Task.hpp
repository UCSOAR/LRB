/**
 ******************************************************************************
 * File Name          : MAX31856Task.hpp
 * Description        : MAX31856 thermocouple polling task
 ******************************************************************************
 */
#ifndef COMPONENTS_MAX31856_TASK_HPP_
#define COMPONENTS_MAX31856_TASK_HPP_

/* Includes ------------------------------------------------------------------*/
#include "MAX31856TaskControl.hpp"
#include "SystemDefines.hpp"

#include "../../SoarOS/Core/Inc/Task.hpp"

class MAX31856Driver;

/* Class ------------------------------------------------------------------*/
class MAX31856Task : public Task
{
public:
    static constexpr int NUM_SENSORS = 3;

    static MAX31856Task& Inst() {
        static MAX31856Task inst;
        return inst;
    }

    void InitTask();

protected:
    static void RunTask(void* pvParams) { MAX31856Task::Inst().Run(pvParams); }

    void Run(void* pvParams);

    void HandleCommand(Command& cm);

private:
    MAX31856Task();
    MAX31856Task(const MAX31856Task&);
    MAX31856Task& operator=(const MAX31856Task&);

    MAX31856Driver* _thermos[NUM_SENSORS];

    bool _enableReading[NUM_SENSORS];
    bool _enableLogging;
    bool _sensorReady[NUM_SENSORS];
    bool _outputInC;
};

#endif /* COMPONENTS_MAX31856_TASK_HPP_ */
