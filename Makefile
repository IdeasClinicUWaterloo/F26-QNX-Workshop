
include src/common/config.mk

ARTIFACT_TYPE = exe

#Build architecture/variant string, possible values: x86, armv7le, etc...
PLATFORM ?= aarch64le

#Build profile, possible values: release, debug, profile, coverage
BUILD_PROFILE ?= debug

CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)

# Targets for processes
ALARM_TARGET    = $(OUTPUT_DIR)/alarm-manager
OXIMETER_TARGET = $(OUTPUT_DIR)/pulse-oximeter

#User defined include/preprocessor flags and libraries
INCLUDES += -Isrc/common/system/gpio
INCLUDES += -Isrc/common/rpi_gpio/public
INCLUDES += -Isrc/common/rpi_i2c/public
INCLUDES += -Isrc/alarm-manager
INCLUDES += -Isrc/pulse-oximeter

#Compiler flags for build profiles
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -fmessage-length=0
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
#Shared library has to be compiled with -fPIC
#CCFLAGS_all += -fPIC
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

COMMON_SRCS := src/common/rpi_gpio/rpi_gpio.c
COMMON_SRCS += src/common/rpi_i2c/rpi_i2c.c
ALARM_SRCS    = $(wildcard src/alarm-manager/*.c)
OXIMETER_SRCS = $(wildcard src/pulse-oximeter/*.c)

COMMON_OBJS = $(addprefix $(OUTPUT_DIR)/,$(COMMON_SRCS:.c=.o))
ALARM_OBJS    = $(addprefix $(OUTPUT_DIR)/,$(ALARM_SRCS:.c=.o))
OXIMETER_OBJS = $(addprefix $(OUTPUT_DIR)/,$(OXIMETER_SRCS:.c=.o))
ALL_OBJS      = $(ALARM_OBJS) $(OXIMETER_OBJS) $(COMMON_OBJS)

#Rules section for default compilation and linking
all: $(ALARM_TARGET) $(OXIMETER_TARGET)

# Compile .c files into .o object files
$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<

# Link alarm-manager
$(ALARM_TARGET): $(ALARM_OBJS)
	$(LD) $(LDFLAGS_$(ARTIFACT_TYPE)) $@ $(LDFLAGS_all) $(LDFLAGS) $^ $(LIBS_all) $(LIBS)

# Link pulse-oximeter
$(OXIMETER_TARGET): $(OXIMETER_OBJS) $(COMMON_OBJS)
	$(LD) $(LDFLAGS_$(ARTIFACT_TYPE)) $@ $(LDFLAGS_all) $(LDFLAGS) $^ $(LIBS_all) $(LIBS)

clean:
	rm -fr $(OUTPUT_DIR)

rebuild: clean all

.PHONY: all clean rebuild

# Include header dependency files
-include $(ALL_OBJS:%.o=%.d)