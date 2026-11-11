# Shared QNX build configuration
include ../common/config.mk

# Artifact configuration
ARTIFACT_TYPE = exe
ARTIFACT = pulse-oximeter

# Build architecture
PLATFORM ?= aarch64le

# Build profile
BUILD_PROFILE ?= debug

CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

# Include paths
INCLUDES = -Isrc
INCLUDES += -I../common/rpi_i2c/public/
INCLUDES += -I../common/rpi_gpio/public/

# Shared alarm IPC headers, if stored here
INCLUDES += -I../common/alarm/

# Library paths
LIBS += -L../common/rpi_i2c/build/$(CONFIG_NAME)
LIBS += -lm -lrpi_i2c

LIBS += -L../common/rpi_gpio/build/$(CONFIG_NAME)
LIBS += -lrpi_gpio

# Compiler flags
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

# Generic compiler flags
CCFLAGS_all += -Wall -fmessage-length=0
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))

# Dependency generation
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

# Find source files recursively
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

# Source files
SRCS = $(call rwildcard,src,c)

# Object files
OBJS = $(addprefix $(OUTPUT_DIR)/,$(addsuffix .o,$(basename $(SRCS))))

# Compile C files into object files
$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<

# Build required libraries
.PHONY: rpi_i2c_lib rpi_gpio_lib

rpi_i2c_lib:
	$(MAKE) -C ../common/rpi_i2c PLATFORM=$(PLATFORM) BUILD_PROFILE=$(BUILD_PROFILE)

rpi_gpio_lib:
	$(MAKE) -C ../common/rpi_gpio PLATFORM=$(PLATFORM) BUILD_PROFILE=$(BUILD_PROFILE)



# Link executable
$(TARGET): $(OBJS) rpi_i2c_lib rpi_gpio_lib
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS_$(ARTIFACT_TYPE)) $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)

# Default build target
.DEFAULT_GOAL := all

.PHONY: all clean rebuild

all: $(TARGET)

clean:
	rm -fr $(OUTPUT_DIR)

rebuild: clean all

# Include generated dependencies
-include $(OBJS:%.o=%.d)
