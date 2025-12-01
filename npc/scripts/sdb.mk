CXXFLAGS += -DBAILUWAN_SIM_MODE=sdb
CSRCS += $(shell find $(abspath ./sim/sdb) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
LDFLAGS += -lreadline