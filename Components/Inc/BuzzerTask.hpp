/**
 ******************************************************************************
 * File Name          : BuzzerTask.hpp
 * Description        : Startup chirp and watchdog alert buzzer task
 ******************************************************************************
*/
#ifndef COMPONENTS_BUZZER_TASK_HPP_
#define COMPONENTS_BUZZER_TASK_HPP_

/* Includes ------------------------------------------------------------------*/
#include "SystemDefines.hpp"

#include "../../SoarOS/Core/Inc/Task.hpp"

/* Macros ------------------------------------------------------------------*/
enum BUZZER_TASK_COMMANDS {
    BUZZER_TASK_COMMAND_NONE = 0,
    BUZZER_TASK_COMMAND_PLAY_WATCHDOG_ALERT,
    BUZZER_TASK_COMMAND_PLAY_BOMB_SOUND,
    BUZZER_TASK_COMMAND_MAX
};

/* Class ------------------------------------------------------------------*/
class BuzzerTask : public Task
{
public:
    static BuzzerTask& Inst() {
        static BuzzerTask inst;
        return inst;
    }

    void InitTask();

    // Queues the watchdog alert sound pattern for future fault/watchdog events. TODO: watchdog integration
    void RequestWatchdogAlert();

    // Alias for future watchdog integration call-sites. TODO: implement it
    void OnWatchdogFault() { RequestWatchdogAlert(); }

protected:
    static void RunTask(void* pvParams) { BuzzerTask::Inst().Run(pvParams); }

    void Run(void* pvParams);

    void HandleCommand(Command& cm);

private:
    BuzzerTask() : Task(TASK_BUZZER_QUEUE_DEPTH_OBJS) {}
    BuzzerTask(const BuzzerTask&);
    BuzzerTask& operator=(const BuzzerTask&);

    void PlayInitChirp();
    void PlayWatchdogAlertPattern();
    void BombHasBeenPlanted();
};

#endif /* COMPONENTS_BUZZER_TASK_HPP_ */
