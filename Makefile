ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: tinypico_status

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
