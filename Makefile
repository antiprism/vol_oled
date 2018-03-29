
# Makefile itself dir
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# hw platform as from autogen.sh choose
HWPLAT:=$(shell cat $(ROOT_DIR)/hwplatform)

CMNFLAGS=-W -Wall -Wno-unused-variable -Wno-unused-parameter -Ofast
# sets CPPFLAGS hw platform dependant
ifeq ($(HWPLAT),BananaPI)
	CPPFLAGS=${CMNFLAGS} -mfpu=vfpv4 -mfloat-abi=hard -march=armv7 -mtune=cortex-a7 -DBANANAPI
else # fallback to raspberry
	# The recommended compiler flags for the Raspberry Pi
	CPPFLAGS=${CMNFLAGS} -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
endif

PROG_NAME=vol_oled
includes = $(wildcard *.h)

# make all
all: ${PROG_NAME}

# Make the library
OBJECTS=main.o timer.o status.o ArduiPi_OLED.o Adafruit_GFX.o \
	bcm2835.o display.o
LDLIBS=-lmpdclient -lcurl -lpthread
${OBJECTS}: ${includes}
${PROG_NAME}: ${OBJECTS}
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# clear build files
clean:
	rm -rf *.o ${PROG_NAME}


