#ifndef COMPONENTS_ANEMOMETER_TASK_HPP_
#define COMPONENTS_ANEMOMETER_TASK_HPP_

/* Includes ------------------------------------------------------------------*/
#include "SystemDefines.hpp"

#include "../../SoarOS/Core/Inc/Task.hpp"

/* Macros ------------------------------------------------------------------*/
enum ANEMOMETER_TASK_COMMANDS {
	ANEMOMETER_TASK_COMMAND_NONE = 0,
	ANEMOMETER_TASK_COMMAND_MAX
};

/* Class ------------------------------------------------------------------*/
class AnemometerTask : public Task
{
public:
	static AnemometerTask& Inst() {
		static AnemometerTask inst;
		return inst;
	}

	void InitTask();

protected:
	static void RunTask(void* pvParams) { AnemometerTask::Inst().Run(pvParams); }

	void Run(void* pvParams);

	void HandleCommand(Command& cm);

private:
	AnemometerTask() : Task(TASK1_QUEUE_DEPTH_OBJS) {}
	AnemometerTask(const AnemometerTask&);
	AnemometerTask& operator=(const AnemometerTask&);
};

#endif /* COMPONENTS_ANEMOMETER_TASK_HPP_ */
