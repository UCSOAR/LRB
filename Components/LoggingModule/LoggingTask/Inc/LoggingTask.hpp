/*
 * LoggingTask.hpp
 *
 *  Created on: Jan 23, 2026
 *      Author: jaddina
 */

#ifndef LOGGINGTASK_LOGGINGTASK_HPP_
#define LOGGINGTASK_LOGGINGTASK_HPP_

#include "Task.hpp"
/************************************
 * MACROS AND DEFINES
 ************************************/


/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * CLASS DEFINITIONS
 ************************************/

/************************************
 * FUNCTION DECLARATIONS
 ************************************/
class LoggingTask: public Task
{
	public:
		static LoggingTask& Inst() {
			static LoggingTask inst;
			return inst;
		}

		void InitTask();



	protected:
		bool RecieveData();
		static void RunTask(void* pvParams) { LoggingTask::Inst().Run(pvParams); } // Static Task Interface, passes control to the instance Run();
		void Run(void * pvParams); // Main run code
		void HandleCommand(Command& cm);
		//uint8_t debugBuffer[LOGGING_RX_BUFFER_SZ_BYTES + 1];

	private:
		// Private Functions
		LoggingTask();        // Private constructor
		LoggingTask(const LoggingTask&);                        // Prevent copy-construction
		LoggingTask& operator=(const LoggingTask&);														// Prevent assignment
		static uint8_t buf[20];
		static bool highAltitude;
		static bool lowAltitude;
		static bool logEnabled_log;
		static bool firstAltFlag;
//		static uint8_t firstAlt;
};


#endif /* LOGGINGTASK_LOGGINGTASK_HPP_ */
