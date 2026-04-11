/**
 ******************************************************************************
 * File Name          : Task2.hpp
 * Description        : Task 2 - Empty task template
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
class Task2 : public Task
{
public:
    static Task2& Inst() {
        static Task2 inst;
        return inst;
    }

    void InitTask();

protected:
    static void RunTask(void* pvParams) { Task2::Inst().Run(pvParams); }

    void Run(void* pvParams);

    void HandleCommand(Command& cm);

private:
    Task2() : Task(TASK2_QUEUE_DEPTH_OBJS) {}
    Task2(const Task2&);
    Task2& operator=(const Task2&);
};

#endif /* COMPONENTS_TASK2_HPP_ */
