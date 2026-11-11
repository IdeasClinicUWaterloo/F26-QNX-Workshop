#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <stdio.h>
#include <unistd.h>
#include "max30102.h"
#include "alarm.h"

static int coid;  // Connection ID

bool connect_channel(void) {
    coid = name_open(ALARM_NAME, 0);

    if (coid == -1) {
        perror("name_open");
        return false;
    }

    printf("Connected to alarm manager! coid=%d\n", coid);
    return true;
}

// Send the non-blocking pulse
bool send_pulse(int code, int command_value) {
    int status = MsgSendPulse(coid, -1, code, command_value);
    return status != -1;
}

int main(void) {

    // Connect to named channel
    /*while(!connect_channel()){
        usleep(100000);
    }*/

    // Wait
    while(!MAX30102_begin()){
        usleep(100000);
    }

    // Configure sensor 
MAX30102_sensorConfiguration(
    /*ledBrightness=*/50,
    /*sampleAverage=*/MAX30102_SAMPLEAVG_4,
    /*ledMode=*/MAX30102_MODE_RED_IR,
    /*sampleRate=*/MAX30102_SAMPLERATE_100,
    /*pulseWidth=*/MAX30102_PULSEWIDTH_411,
    /*adcRange=*/MAX30102_ADCRANGE_16384
);
    /*if(sensor_config_resp) {
        send_pulse(EVENT_SENSOR_CONFIGURED, 1);
    } else{
        send_pulse(EVENT_SENSOR_CONFIGURED, 0);
        return 1; 
    }*/
    
    // Read heart rate and spo2
    usleep(100);

    for(;;){
        int32_t SPO2; //SPO2
        bool SPO2Valid; //Flag to display if SPO2 calculation is valid
        int32_t heartRate; //Heart-rate
        bool heartRateValid; //Flag to display if heart-rate calculation is valid 

        MAX30102_heartrateAndOxygenSaturation(/**SPO2=*/&SPO2, /**SPO2Valid=*/&SPO2Valid, /**heartRate=*/&heartRate, /**heartRateValid=*/&heartRateValid);
        
        /*send_pulse(EVENT_HEARTRATE, heartRate);
        send_pulse(EVENT_SPO2, SPO2);*/

        if(heartRateValid && SPO2Valid){
            printf("heartrate=%d, ", heartRate);
            printf("spo2=%d", SPO2);
        } 

        if(!heartRateValid || !SPO2Valid){
            printf("Invalid measurement. Please press your finger firmly on the sensor.");
        } 

        printf("\n");

        usleep(100);
    }
}