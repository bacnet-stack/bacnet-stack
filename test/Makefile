# Main Makefile for BACnet-stack applications, tests, and sample ports

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

# all: demos router-ipv6 ${DEMO_LINUX}

.PHONY: all
all: test

BUILD_DIR=build
.PHONY: test
test:
	[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake .. && cd ..
	[ -d $(BUILD_DIR) ] && cd $(BUILD_DIR) && cmake --build . --clean-first && cd ..
	[ -d $(BUILD_DIR) ] && $(MAKE) -C $(BUILD_DIR) test
	[ -d $(BUILD_DIR) ] && $(MAKE) -C $(BUILD_DIR) lcov
	echo "Coverage Report"

.PHONY: clean
clean:
	-rm -rf $(BUILD_DIR)
