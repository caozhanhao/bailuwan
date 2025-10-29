# Makefile for NPC (New Processor Core) by caozhanhao

### *Get a more readable version of this Makefile* by `make html` (requires python-markdown)
html:
	cat Makefile | sed 's/^\([^#]\)/    \1/g' | markdown_py > Makefile.html
.PHONY: html

## 1. Basic Setup and Checks

### Project Info
PRJ = npc
TOPNAME = ysyxSoCFull
TRACE ?= notrace
CYCLES ?= -1
IMG ?=

### Run checks only when `sim`
ifneq ($(findstring $(MAKECMDGOALS),sim),)

### Print build info message
$(info # Building NPC [$(HW)] [Trace: $(TRACE)] [sim $(CYCLES) cycles] [img: $(IMG)])

### Check: environment variable `HW` must be in the supported list
HWS = $(basename $(notdir $(shell ls ./scripts/*.mk)))
ifeq ($(filter $(HWS), $(HW)), )
$(error Expected $$HW in {$(HWS)}, Got "$(HW)")
endif

### Check: valid trace
ifeq ($(filter $(TRACE), notrace fst vcd), )
$(error Expected $$TRACE in {notrace, fst, vcd}, Got "$(TRACE)")
endif

### Check: valid image
ifeq ($(IMG), )
$(error Expected $$IMG)
endif

### Checks end here
endif



## 2. General Compilation Targets

### Create the destination directory (`build`)
BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
$(shell mkdir -p $(BUILD_DIR))

### Compilation targets
BIN = $(BUILD_DIR)/$(TOPNAME)-$(HW)-$(TRACE)
VERILOG_STAMP := $(BUILD_DIR)/verilog.timestamp

### Collect the files to be built and linked
LINKAGE ?=
CSRCS ?=
INC_PATH ?=
SOC_VSRCS ?=
SOC_VINC_PATH ?=
CHISEL_SRCS = $(shell find $(abspath ./$(PRJ)) -name "*.scala")



## 3. General Compilation Flags

### Verilator
VERILATOR = verilator

### Compilation flags
INC_PATH += $(abspath ./csrc/common/)
CSRCS += $(shell find $(abspath ./csrc/common) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
HEADERS := $(sort $(shell find $(INC_PATH) -type f \( -name '*.h' -o -name '*.hpp' \) 2>/dev/null))

SOC_VSRCS += $(shell find $(YSYXSOC_HOME)/perip -type f -name '*.v' 2>/dev/null)
SOC_VSRCS += $(YSYXSOC_HOME)/build/ysyxSoCFull.v
SOC_VINC_PATH += $(abspath $(YSYXSOC_HOME)/perip/uart16550/rtl)
SOC_VINC_PATH += $(abspath $(YSYXSOC_HOME)/perip/spi/rtl)
SOC_VINC_FLAGS += $(addprefix +incdir+, $(SOC_VINC_PATH))

VERILATOR_CFLAGS += -MMD --build -cc  \
				-O3 --x-assign fast --x-initial fast --noassert
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""
EMIT_VERILOG_FLAGS = --target-dir $(BUILD_DIR) --split-verilog

ifneq ($(TRACE), notrace)
TRACE_FILENAME ?= $(BUILD_DIR)/trace-$(TOPNAME)-$(HW).$(TRACE)
CXXFLAGS += -DTRACE -DTRACE_$(TRACE) -DTRACE_FILENAME="\"$(TRACE_FILENAME)\""
VERILATOR_CFLAGS += --trace-$(TRACE)
endif

## 4. Hardware-Specific Configurations
-include ./scripts/$(HW).mk



## 5. Compilation Rules

$(VERILOG_STAMP): $(CHISEL_SRCS)
	@rm -rf $(BUILD_DIR)/*.sv
	./mill -i $(PRJ).runMain utils.EmitVerilog $(EMIT_VERILOG_FLAGS)
	# rename some ports to connect ysyxSoC
	sed -i -e 's/_\(aw\|ar\|w\|r\|b\)_\(\|bits_\)/_\1/g' $@
	@touch $@


$(BIN): $(HEADERS) $(CSRCS) $(LINKAGE) $(VERILOG_STAMP)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) $(SOC_VINC_FLAGS) \
		--top-module $(TOPNAME) $(BUILD_DIR)/*.sv $(SOC_VSRCS) $(CSRCS) $(LINKAGE) \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN)) --timescale "1ns/1ns" --no-timing


## 6. Miscellaneous

### Simulation
sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	$(MAKE) $(BIN)
	$(BIN) $(ARGS) $(IMG)

### Chisel Test
test:
	./mill -i $(PRJ).test

### Generate Verilog
verilog:
	$(call git_commit, "generate verilog")
	./mill -i $(PRJ).runMain utils.EmitVerilog $(EMIT_VERILOG_FLAGS)

### Reformat
reformat:
	./mill -i __.reformat

### Check Format
checkformat:
	./mill -i __.checkFormat

### BSP
bsp:
	./mill -i mill.bsp.BSP/install

### IntelliJ IDEA
idea:
	./mill -i mill.idea.GenIdea/idea

### Clean
clean:
	-rm -rf $(BUILD_DIR)

.PHONY: test verilog reformat checkformat clean
-include ../Makefile
