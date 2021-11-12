ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: status

upload: status
	pio run -j 2 --target upload && sleep 1;  pio device monitor

status: src/main.cpp
	pio run

monitor:
	pio device monitor

configure:
	pio lib install "TinyPICO Helper Library"

clean:
	pio run --target clean
