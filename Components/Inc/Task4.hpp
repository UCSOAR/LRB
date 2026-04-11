/**
 ******************************************************************************
 * File Name          : Task4.hpp
 * Description        : Task 4 - Empty task template
 ******************************************************************************
*/
#ifndef COMPONENTS_TASK4_HPP_
#define COMPONENTS_TASK4_HPP_

/* Includes ------------------------------------------------------------------*/
#include "Task.hpp"
#include "SystemDefines.hpp"

/* Macros ------------------------------------------------------------------*/
enum TASK4_COMMANDS {
    TASK4_COMMAND_NONE = 0,
    TASK4_COMMAND_MAX
};

/* Class ------------------------------------------------------------------*/
class Task4 : public Task
{
public:
    static Task4& Inst() {
        static Task4 inst;
        return inst;
    }

    void InitTask();

protected:
    static void RunTask(void* pvParams) { Task4::Inst().Run(pvParams); }

    void Run(void* pvParams);

    void HandleCommand(Command& cm);

private:
    Task4() : Task(TASK4_QUEUE_DEPTH_OBJS) {}
    Task4(const Task4&);
    Task4& operator=(const Task4&);
};

#endif /* COMPONENTS_TASK4_HPP_ */
