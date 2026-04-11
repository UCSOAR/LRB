/**
 ******************************************************************************
 * File Name          : Task3.hpp
 * Description        : Task 3 - Empty task template
 ******************************************************************************
*/
#ifndef COMPONENTS_TASK3_HPP_
#define COMPONENTS_TASK3_HPP_

/* Includes ------------------------------------------------------------------*/
#include "Task.hpp"
#include "SystemDefines.hpp"
#include "UARTDriver.hpp"

/* Macros ------------------------------------------------------------------*/
enum TASK3_COMMANDS {
    TASK3_COMMAND_NONE = 0,
    TASK3_COMMAND_SEND_DEBUG,
    TASK3_COMMAND_RX_COMPLETE,
    TASK3_COMMAND_MAX
};

/* Class ------------------------------------------------------------------*/
class Task3 : public Task, public UARTReceiverBase
{
public:
    static Task3& Inst() {
        static Task3 inst;
        return inst;
    }

    void InitTask();
    void InterruptRxData(uint8_t errors);

protected:
    static void RunTask(void* pvParams) { Task3::Inst().Run(pvParams); }

    void Run(void* pvParams);

    bool ReceiveData();
    void HandleCommand(Command& cm);

    uint8_t rxChar_;
    UARTDriver* const kUart_;

private:
    Task3() : Task(TASK3_QUEUE_DEPTH_OBJS), rxChar_(0), kUart_(UART::Debug) {}
    Task3(const Task3&);
    Task3& operator=(const Task3&);
};

#endif /* COMPONENTS_TASK3_HPP_ */
