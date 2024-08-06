# Main Makefile for BACnet-stack applications, tests, and sample ports

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

# all: demos router-ipv6 ${DEMO_LINUX}

.PHONY: all
all: test coverage

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

.PHONY: retest
retest:
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . $(JOBS) && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && ctest $(CTEST_OPTIONS)

.PHONY: coverage
coverage:
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

.PHONY: clean
clean:
	-rm -rf $(BUILD_DIR)
