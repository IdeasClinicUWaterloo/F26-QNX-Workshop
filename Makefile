PI_SSH_TARGET = qnxpi_2
REMOTE_DIR = /home/qnxuser

PULSE_EXE = 01-Pulse-Oximeter/build/aarch64le-debug/pulse-oximeter
ALARM_EXE = 02-Alarm-Manager/build/aarch64le-debug/alarm-manager

.PHONY: all build deploy run clean

all: run

build:
	$(MAKE) -C 01-Pulse-Oximeter
	$(MAKE) -C 02-Alarm-Manager

deploy: build
	-ssh $(PI_SSH_TARGET) "slay pulse-oximeter; slay alarm-manager"
	scp $(PULSE_EXE) $(ALARM_EXE) $(PI_SSH_TARGET):$(REMOTE_DIR)/

run: deploy
	ssh $(PI_SSH_TARGET) "cd $(REMOTE_DIR); ./alarm-manager & sleep 1; ./pulse-oximeter"

clean:
	$(MAKE) -C 01-Pulse-Oximeter clean
	$(MAKE) -C 02-Alarm-Manager clean