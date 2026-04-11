/**
 ******************************************************************************
 * File Name          : Task1.hpp
 * Description        : Task 1 - Empty task template
 ******************************************************************************
*/
#ifndef COMPONENTS_TASK1_HPP_
#define COMPONENTS_TASK1_HPP_

/* Includes ------------------------------------------------------------------*/
#include "Task.hpp"
#include "SystemDefines.hpp"

/* Macros ------------------------------------------------------------------*/
enum TASK1_COMMANDS {
    TASK1_COMMAND_NONE = 0,
    TASK1_COMMAND_MAX
};

/* Class ------------------------------------------------------------------*/
class Task1 : public Task
{
public:
    static Task1& Inst() {
        static Task1 inst;
        return inst;
    }

    void InitTask();

protected:
    static void RunTask(void* pvParams) { Task1::Inst().Run(pvParams); }

    void Run(void* pvParams);

    void HandleCommand(Command& cm);

private:
    Task1() : Task(TASK1_QUEUE_DEPTH_OBJS) {}
    Task1(const Task1&);
    Task1& operator=(const Task1&);
};

#endif /* COMPONENTS_TASK1_HPP_ */
