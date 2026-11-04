#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <unistd.h>
#include <stdint.h>
#include <alarm.h>
#include <alarm_manager.h>
#include <time.h>
#include <pthread.h>

double elapsed;
int32_t SPO2; //SPO2
int32_t heartRate; //Heart-rate

void *update_alarm_thread(void *arg) {
    for (;;) {
        if(!update_alarm_states(SPO2, heartRate, elapsed)){
            perror("Failed to update alarm state");
            break;
        }

        printf("\033[1;31m[HEART RATE]\033[0m%d BPM",heartRate);
        printf("; \e[1;34m[SPO2]\e[0m%d; ", SPO2);

        display_alarms();

        printf("\n");

        usleep(1000);
    }
}

name_attach_t* init_channel(void) {
    name_attach_t *attach = name_attach(NULL, ALARM_NAME, 0);
    if (attach == NULL) {
        perror("name_attach");
    }
    return attach; 
}

int main(void) {
    struct timespec last_pulse_time;
    bool first_pulse = true;

    // Create thread to update alarms
    pthread_t update_alarm_id;
    int resp = pthread_create(&update_alarm_id, NULL, update_alarm_thread,NULL);

    if (resp != 0) {
        perror("Thread creation failed");
        return 1;
    }

    // Sensor aquistion
    name_attach_t *attach = init_channel();
    usleep(1000);

    struct _pulse pulse_msg;
    int rcvid;

    for(;;){
        // Wait to get message
        rcvid = MsgReceive(attach->chid, &pulse_msg, sizeof(pulse_msg), NULL);

        if (rcvid == -1) continue;
        if(rcvid != 0) MsgReply(rcvid, 0, NULL, 0);

        switch (pulse_msg.code){
            case EVENT_SENSOR_CONFIGURED:
                if (pulse_msg.value.sival_int == 1) {
                    printf("[INFO] Sensor reported successful configuration.\n");
                } else {
                    printf("[WARN] Sensor reported configuration FAILURE.\n");
                }
                break;
            case EVENT_HEARTRATE:
                struct timespec current_time;
                clock_gettime(CLOCK_MONOTONIC, &current_time);

                if (!first_pulse) {
                    elapsed = (current_time.tv_sec - last_pulse_time.tv_sec) + (current_time.tv_nsec - last_pulse_time.tv_nsec) / 1e9;
                }

                last_pulse_time = current_time;
                first_pulse = false;

                heartRate = pulse_msg.value.sival_int;
                break;
            case EVENT_SPO2:
                SPO2 = pulse_msg.value.sival_int;
                break;
            case _PULSE_CODE_DISCONNECT:
                printf("[SYSTEM] Sender disconnected from the channel.\n");
                ConnectDetach(pulse_msg.scoid);
                break;
            default:
                printf("[SYSTEM] Received unknown pulse code: %d\n", pulse_msg.code);
                break;

        }

    }
}