/**
 ******************************************************************************
 * File Name          : MAX31856TaskControl.hpp
 * Description        : MAX31856 task command identifiers and send helper
 ******************************************************************************
 */
#ifndef COMPONENTS_MAX31856_TASK_CONTROL_HPP_
#define COMPONENTS_MAX31856_TASK_CONTROL_HPP_

#include <stdint.h>

enum MAX31856_TASK_COMMANDS {
    MAX31856_TASK_COMMAND_NONE = 0,
    MAX31856_TASK_COMMAND_TOGGLE,
    MAX31856_TASK_COMMAND_ON,
    MAX31856_TASK_COMMAND_OFF,
    MAX31856_TASK_COMMAND_STATUS,
    MAX31856_TASK_COMMAND_ENABLE_LOG,
    MAX31856_TASK_COMMAND_DISABLE_LOG,
    MAX31856_TASK_COMMAND_READ_TC1,
    MAX31856_TASK_COMMAND_READ_TC2,
    MAX31856_TASK_COMMAND_READ_TC3,
    MAX31856_TASK_COMMAND_READ_FORCE_TC1,
    MAX31856_TASK_COMMAND_READ_REGS_TC1,
    MAX31856_TASK_COMMAND_TOGGLE_CS_TC1,
    MAX31856_TASK_COMMAND_TOGGLE_TC1,
    MAX31856_TASK_COMMAND_TOGGLE_TC2,
    MAX31856_TASK_COMMAND_TOGGLE_TC3,
    MAX31856_TASK_COMMAND_MAX
};

bool SendMax31856TaskCommand(uint16_t taskCommand);

#ifdef __cplusplus
extern "C" {
#endif
void MAX31856Task_HandleDrdyInterrupt(uint16_t gpioPin);
#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_MAX31856_TASK_CONTROL_HPP_ */