ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PLATFORMIO_CORE_DIR ?= $(ROOT_DIR)/.pio
CC ?= gcc
CXX ?= g++

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

GCOV ?= gcov

coverage:
	@if [ "$(shell uname)" = "Darwin" ]; then \
		$(MAKE) coverage-llvm; \
	else \
		CC=$(CC) CXX=$(CXX) PLATFORMIO_CORE_DIR=$(PLATFORMIO_CORE_DIR) pio test -e native_coverage; \
		command -v gcovr >/dev/null 2>&1 || { echo "gcovr not installed; install with 'pip install gcovr'"; exit 1; }; \
		gcovr -r . --object-directory .pio/build/native_coverage --gcov-executable "$(GCOV)" \
		  --exclude 'Unity/' --exclude 'unity_config/' --exclude '\\.pio/libdeps/' --exclude '(^|/)test/' \
		  --html-details coverage.html --xml coverage.xml --print-summary; \
	fi

coverage-llvm:
	PLATFORMIO_CORE_DIR=$(PLATFORMIO_CORE_DIR) LLVM_PROFILE_FILE=$(PLATFORMIO_CORE_DIR)/build/native_llvmcov/%p.profraw pio test -e native_llvmcov
	@command -v llvm-profdata >/dev/null 2>&1 || { echo "llvm-profdata not found (install Xcode Command Line Tools or llvm)"; exit 1; }
	@command -v llvm-cov >/dev/null 2>&1 || { echo "llvm-cov not found (install Xcode Command Line Tools or llvm)"; exit 1; }
	@profiles=$$(find $(PLATFORMIO_CORE_DIR)/build/native_llvmcov -name '*.profraw'); \
	llvm-profdata merge -sparse $$profiles -o $(PLATFORMIO_CORE_DIR)/build/native_llvmcov/default.profdata; \
	llvm-cov report $(PLATFORMIO_CORE_DIR)/build/native_llvmcov/program -instr-profile $(PLATFORMIO_CORE_DIR)/build/native_llvmcov/default.profdata \
	  -ignore-filename-regex='Unity|unity_config|\\.pio/libdeps|(^|/)test/'; \
	llvm-cov show $(PLATFORMIO_CORE_DIR)/build/native_llvmcov/program -format=html -output-dir coverage-llvm \
	  --instr-profile $(PLATFORMIO_CORE_DIR)/build/native_llvmcov/default.profdata \
	  -ignore-filename-regex='Unity|unity_config|\\.pio/libdeps|(^|/)test/'

.PHONY: all wroom_s3 tinypico tinypico_status tinys3 tinys3_status pico pico_status monitor clean test coverage coverage-llvm
