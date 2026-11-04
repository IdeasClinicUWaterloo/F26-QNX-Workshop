#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>
#include "alarm_manager.h"
#include <string.h>

#define SENSOR_TIMEOUT_NS 1500000000ULL
#define CHECK_INTERVAL_NS 100000000ULL

#define BINARY_PATH "/home/qnxuser/pulse-oximeter"
#define BUFFER_SIZE 512
#define ALARM_COUNT 4
#define HR_LOW_THRESHOLD 40
#define HR_HIGH_THRESHOLD 170
#define SPO2_LOW_THRESHOLD 85
#define TIMEOUT 1.5

typedef enum {
    ALARM_SPO2_LOW= 0,      
    ALARM_HR_LOW,      
    ALARM_HR_HIGH,
    ALARM_SENSOR_DISCONNECT
} Alarm_Id_t;

typedef enum {
    PRIORITY_LOW = 0,         // Informational / Technical
    PRIORITY_MEDIUM,      // Requires prompt response
    PRIORITY_HIGH         // Immediate danger to patient
} Alarm_Priority_t;

typedef enum {
    STATE_INACTIVE = 0,
    STATE_ACTIVE,        
    STATE_SILENCED 
} Alarm_State_t;

typedef struct{
    Alarm_Id_t id; 
    Alarm_Priority_t priority;
    Alarm_State_t state;
    char message[BUFFER_SIZE];
} Alarm_t;

Alarm_t alarms[ALARM_COUNT] = {
    [ALARM_SPO2_LOW] = {
        .id = ALARM_SPO2_LOW, 
        .priority = PRIORITY_HIGH, 
        .state = STATE_INACTIVE, 
        .message = "[Alarm] Low SpO2 detected"
    },
    [ALARM_HR_LOW] = {
        .id = ALARM_HR_LOW, 
        .priority = PRIORITY_MEDIUM, 
        .state = STATE_INACTIVE, 
        .message = "[Alarm] Low heart rate detected"
    },
    [ALARM_HR_HIGH] = {
        .id = ALARM_HR_HIGH, 
        .priority = PRIORITY_HIGH, 
        .state = STATE_INACTIVE, 
        .message = "[Alarm] High heart rate detected"
    },
    [ALARM_SENSOR_DISCONNECT] = {
        .id = ALARM_SENSOR_DISCONNECT, 
        .priority = PRIORITY_LOW, 
        .state = STATE_INACTIVE, 
        .message = "[Alarm] Sensor disconnected"
    }
};

bool update_alarm_states(int32_t SPO2, int32_t heartRate, double elasped) {
    for(int i = 0; i < ALARM_COUNT; i++){
        switch(alarms[i].id){
            case ALARM_SPO2_LOW:
                alarms[i].state = (SPO2 < SPO2_LOW_THRESHOLD)? STATE_ACTIVE : STATE_INACTIVE;
                break;
            case ALARM_HR_HIGH:
                alarms[i].state = (heartRate > HR_HIGH_THRESHOLD)? STATE_ACTIVE : STATE_INACTIVE;

                break;
            case ALARM_HR_LOW:
                alarms[i].state = (heartRate  < HR_LOW_THRESHOLD)? STATE_ACTIVE : STATE_INACTIVE;

                break;
            case ALARM_SENSOR_DISCONNECT:
                alarms[i].state = (elasped > TIMEOUT)? STATE_ACTIVE : STATE_INACTIVE;
                pid_t pulse_oximeter_pid;
                pid_t new_pid = restart_pulse_oximeter(pulse_oximeter_pid);
                printf("[ERROR] Restarting pulse oximeter...");
                
                break;
            default:
                alarms[i].state = STATE_INACTIVE;
                break;

        }
    }
    return 1; 
}

extern char **environ;

void display_alarms(){
    int8_t non_active_alarms = 0;
    for(int i = 0; i < ALARM_COUNT; i++){
        if(alarms[i].state == STATE_ACTIVE){
            printf("%s; ", alarms[i].message);
        } else if (alarms[i].state == STATE_INACTIVE){
            non_active_alarms += 1;
        }
    }

    if(non_active_alarms == ALARM_COUNT) printf("[Alarm] System OK");
}

pid_t restart_pulse_oximeter(pid_t current_pid) {

    if (current_pid <= 0) {
        fprintf(stderr, "Invalid pulse oximeter PID\n");
        return -1;
    }

    // Terminate existing process
    if (kill(current_pid, SIGKILL) == -1 && errno != ESRCH) {
        perror("kill");
        return -1;
    }

    // Wait for old process to terminate (must be our child)
    while (waitpid(current_pid, NULL, 0) == -1) {
        if (errno == EINTR) continue;

        perror("waitpid");
        return -1;
    }

    // Restart process
    pid_t new_pid;
    char *argv[] = {"pulse-oximeter", NULL};

    int ret = posix_spawn(
        &new_pid,
        "/home/qnxuser/pulse-oximeter",
        NULL,
        NULL,
        argv,
        environ
    );

    if (ret != 0) {
        fprintf(stderr, "posix_spawn: %s\n", strerror(ret));
        return -1;
    }

    printf("[SYSTEM] Restarted pulse oximeter: PID=%d\n", new_pid);

    return new_pid;
}