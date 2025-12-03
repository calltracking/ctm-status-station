ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PLATFORMIO_CORE_DIR ?= $(ROOT_DIR)/.pio

all: tinypico#wroom_s3

wroom_s3:
	pio run -e s3wroom_1 -j 2 --target upload

tinypico: tinypico_status
	pio run -e tinypico -j 2 --target upload && sleep 1;  pio device monitor

tinypico_status: src/main.cpp
	pio run -e tinypico 

tinys3: tinys3_status
	pio run -e um_tinys3 -j 2 --target upload && sleep 1;  pio device monitor

tinys3_status: src/main.cpp
	pio run -e um_tinys3 

pico: pico_status
	pio run -e pico -j 2 --target upload && sleep 1;  pio device monitor

pico_status: src/main.cpp
	pio run -e pico 

monitor:
	pio device monitor

clean:
	pio run --target clean

test:
	PLATFORMIO_CORE_DIR=$(PLATFORMIO_CORE_DIR) pio test -e native

.PHONY: all wroom_s3 tinypico tinypico_status tinys3 tinys3_status pico pico_status monitor clean test
