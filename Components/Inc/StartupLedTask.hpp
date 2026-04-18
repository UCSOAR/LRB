/**
 ******************************************************************************
 * File Name          : StartupLedTask.hpp
 * Description        : Startup LED task
 ******************************************************************************
*/
#ifndef COMPONENTS_TASK2_HPP_
#define COMPONENTS_TASK2_HPP_

/* Includes ------------------------------------------------------------------*/
#include "Task.hpp"
#include "SystemDefines.hpp"

/* Macros ------------------------------------------------------------------*/
enum TASK2_COMMANDS {
    TASK2_COMMAND_NONE = 0,
    TASK2_COMMAND_MAX
};

/* Class ------------------------------------------------------------------*/
class StartupLedTask : public Task
{
public:
    static StartupLedTask& Inst() {
        static StartupLedTask inst;
        return inst;
    }

    void InitTask();

protected:
    static void RunTask(void* pvParams) { StartupLedTask::Inst().Run(pvParams); }

    void Run(void* pvParams);

    void HandleCommand(Command& cm);

private:
    StartupLedTask() : Task(TASK2_QUEUE_DEPTH_OBJS) {}
    StartupLedTask(const StartupLedTask&);
    StartupLedTask& operator=(const StartupLedTask&);
};

#endif /* COMPONENTS_TASK2_HPP_ */
