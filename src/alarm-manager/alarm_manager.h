#ifndef ALARM_MANAGER_H_
#define ALARM_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/neutrino.h>

#define ALARM_NAME "alarm_manager"

enum event_t {
    EVENT_SENSOR_CONFIGURED = _PULSE_CODE_MINAVAIL,
    EVENT_HEARTRATE,
    EVENT_SPO2
};

bool update_alarm_states(int32_t SPO2, int32_t heartRate, double elasped);
void display_alarms();

#endif