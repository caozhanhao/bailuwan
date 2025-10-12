CSRCS += $(shell find $(abspath ./csrc/sdb) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
LDFLAGS += -lreadline
CLI_ARGS = $(FILENAME) $(ELF)