CSRCS += $(shell find $(abspath ./csrc/debug) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CLI_ARGS = $(CYCLES) $(FILENAME)