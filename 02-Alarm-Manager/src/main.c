#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <unistd.h>
#include <stdint.h>
#include "alarm.h"

name_attach_t* init_channel(void) {
    name_attach_t *attach = name_attach(NULL, ALARM_NAME, 0);
    if (attach == NULL) {
        perror("name_attach");
    }
    return attach; 
}

int main(void) {
    name_attach_t *attach = init_channel();
    usleep(1000);

    struct _pulse pulse_msg;
    int rcvid;

    int32_t SPO2; //SPO2
    int32_t heartRate; //Heart-rate

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

        printf("\033[1;31mheart rate\033[0m=%d",heartRate);
        printf("; \e[1;34mSPO2\e[0m=%d\n", SPO2);

    }
}