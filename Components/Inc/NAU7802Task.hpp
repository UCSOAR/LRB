/**
 ******************************************************************************
 * File Name          : NAU7802Task.hpp
 * Description        : NAU7802 sensor polling task
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
    TASK1_COMMAND_NAU_TOGGLE,
    TASK1_COMMAND_NAU_ON,
    TASK1_COMMAND_NAU_OFF,
    TASK1_COMMAND_NAU_STATUS,
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
    NAU7802Task() : Task(TASK1_QUEUE_DEPTH_OBJS), nauReadEnabled_(true) {}
    NAU7802Task(const NAU7802Task&);
    NAU7802Task& operator=(const NAU7802Task&);

    bool nauReadEnabled_;
};

#endif /* COMPONENTS_TASK1_HPP_ */
