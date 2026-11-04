#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>

bool update_alarm_states(int32_t SPO2, int32_t heartRate, double elasped);
void display_alarms();