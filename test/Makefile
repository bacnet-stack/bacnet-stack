# Main Makefile for BACnet-stack applications, tests, and sample ports

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

# all: demos router-ipv6 ${DEMO_LINUX}

.PHONY: all
all: test

BUILD_DIR=build

CTEST_OPTIONS := --quiet --output-on-failure
CTEST_OPTIONS += --output-junit test-results.xml

# determine the compile environment name
ifeq ($(OS),Windows_NT)
SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring mingw, $(SYS)))
UNAME_S := MinGW
else ifneq (, $(findstring cygwin, $(SYS)))
UNAME_S := Cygwin
else
UNAME_S := Windows
endif
else
UNAME_S := $(shell uname -s)
UNAME_S := $(patsubst BSD%,BSD,$(UNAME_S))
endif

# unlimited parallel makefile jobs can
# overwhelm builders with limited resources
# set job limits based on number of core processors
ifeq (${JOBS},)
	ifeq ($(UNAME_S),Windows)
		JOBS = "-j %NUMBER_OF_PROCESSORS%"
	endif
	ifeq ($(UNAME_S),Linux)
		JOBS = "-j $(shell nproc)"
	endif
	ifeq ($(UNAME_S),Darwin)
		JOBS = "-j $(sysctl -n hw.ncpu)"
	endif
	ifeq ($(UNAME_S),BSD)
		JOBS = "-j $(sysctl -n hw.ncpu)"
	endif
	ifeq ($(UNAME_S),MinGW)
		JOBS = "-j"
	endif
endif

.PHONY: test
test:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake .. && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ctest $(CTEST_OPTIONS)
	[ -d $(BUILD_DIR) ] && $(MAKE) -C $(BUILD_DIR) lcov

.PHONY: retest
retest:
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ctest $(CTEST_OPTIONS)
	[ -d $(BUILD_DIR) ] && $(MAKE) -C $(BUILD_DIR) lcov

.PHONY: report
report:
	[ -d $(BUILD_DIR) ] && cat $(BUILD_DIR)/Testing/Temporary/LastTest*.log

.PHONY: rebuild
rebuild:
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . --clean-first && cd ..

.PHONY: env
env:
	@echo "Makefile environment variables"
	@echo "UNAME_S=$(UNAME_S)"
	@echo "JOBS=$(JOBS)"
	@echo "MAKEFLAGS=$(MAKEFLAGS)"
	@echo "CTEST_OPTIONS=$(CTEST_OPTIONS)"
	@echo "BUILD_DIR=$(BUILD_DIR)"

BSC_DATALINK_DIR := $(realpath ./bacnet/datalink/bsc-datalink)
.PHONY: bsc-datalink
bsc-datalink:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake $(BSC_DATALINK_DIR) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ./test_bsc-datalink && cd ..

BSC_NODE_DIR := $(realpath ./bacnet/datalink/bsc-node)
.PHONY: bsc-node
bsc-node:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake $(BSC_NODE_DIR) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ./test_bsc-node && cd ..

BSC_HUB_DIR := $(realpath ./bacnet/datalink/hub-sc)
.PHONY: bsc-hub
bsc-hub:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake $(BSC_HUB_DIR) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ./test_hub-sc && cd ..

BSC_BVLC_DIR := $(realpath ./bacnet/datalink/bvlc-sc)
.PHONY: bsc-bvlc
bsc-bvlc:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake $(BSC_BVLC_DIR) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ./test_bvlc-sc && cd ..

BSC_SOCKET_DIR := $(realpath ./bacnet/datalink/bsc-socket)
.PHONY: bsc-socket
bsc-socket:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake $(BSC_SOCKET_DIR) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ./test_bsc-socket && cd ..

BSC_WEBSOCKET_DIR := $(realpath ./bacnet/datalink/websockets)
.PHONY: websockets
websockets:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake $(BSC_WEBSOCKET_DIR) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ./test_websockets && cd ..

.PHONY: libwebsockets
libwebsockets:
	sudo apt-get update -qq
	sudo apt-get install -qq libconfig-dev
	sudo apt-get install -qq libcap-dev
	sudo apt-get install -qq libssl-dev
	sudo apt-get install -qq libuv1-dev
	git clone --branch v4.3-stable https://github.com/warmcat/libwebsockets.git
	bash -c 'cd libwebsockets;mkdir build;cd build;cmake .. -DLWS_WITH_LIBUV=ON -DLWS_WITH_MINIMAL_EXAMPLES=0 -DLWS_MAX_SMP=32 -DCMAKE_POLICY_VERSION_MINIMUM=3.5;make'
	sudo bash -c 'cd libwebsockets;cd build;make install'

test-bsc: bsc-datalink bsc-node bsc-hub bsc-bvlc bsc-socket websockets

.PHONY: clean
clean:
	-rm -rf $(BUILD_DIR)
