#ifndef ALARM_IPC_H
#define ALARM_IPC_H

#define ALARM_NAME "alarm_manager"

enum event_t {
    EVENT_SENSOR_CONFIGURED = _PULSE_CODE_MINAVAIL,
    EVENT_HEARTRATE,
    EVENT_SPO2
};

#endif