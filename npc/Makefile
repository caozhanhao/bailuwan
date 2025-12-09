# Makefile for BailuWan (白鹭湾)

### *Get a more readable version of this Makefile* by `make html` (requires python-markdown)
html:
	cat Makefile | sed 's/^\([^#]\)/    \1/g' | markdown_py > Makefile.html
.PHONY: html

## 1. Basic Setup and Checks

### Project Info
PRJ = bailuwan
TOPNAME = ysyxSoCFull
RESET_VECTOR = 0x30000000
WITHOUT_SOC = false
TRACE ?= notrace
IMG ?=

### Run checks only when `sim`
ifneq ($(findstring $(MAKECMDGOALS),sim),)

### Print build info message
$(info -> Building BaiLuWan [$(HW)] [Trace: $(TRACE)] [img: $(IMG)])

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
BUILD_DIR = $(abspath ./build)
OBJ_DIR = $(BUILD_DIR)/obj_dir
$(shell mkdir -p $(BUILD_DIR))

### Compilation targets
BIN = $(BUILD_DIR)/$(TOPNAME)-$(HW)-$(TRACE)
CACHESIM_BIN = $(BUILD_DIR)/cachesim
VERILOG_STAMP := $(BUILD_DIR)/bailuwan_verilog_$(TOPNAME)_$(RESET_VECTOR)_$(WITHOUT_SOC).timestamp

### Collect the files to be built and linked
LINKAGE ?=
CSRCS ?=
INC_PATH ?=
SOC_VSRCS ?=
SOC_VINC_PATH ?=
CHISEL_SRCS = $(shell find $(abspath ./$(PRJ)) -name "*.scala")
SOC_CHISEL_DEPS = $(shell find $(YSYXSOC_HOME)/src -name "*.scala")
CACHESIM_HEADERS = $(shell find $(abspath ./cachesim) -name "*.hpp" -or -name "*.h")
CACHESIM_SRCS = $(shell find $(abspath ./cachesim) -name "*.c" -or -name "*.cc" -or -name "*.cpp")


## 3. General Compilation Flags

### Verilator
VERILATOR = verilator

### Compilation flags
INC_PATH += $(abspath ./sim/common/)
CSRCS += $(shell find $(abspath ./sim/common) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
HEADERS := $(sort $(shell find $(INC_PATH) -type f \( -name '*.h' -o -name '*.hpp' \) 2>/dev/null))

SOC_VSRCS += $(shell find $(YSYXSOC_HOME)/perip -type f -name '*.v' 2>/dev/null)
SOC_VSRCS += $(YSYXSOC_HOME)/build/ysyxSoCFull.v
SOC_VINC_PATH += $(abspath $(YSYXSOC_HOME)/perip/uart16550/rtl)
SOC_VINC_PATH += $(abspath $(YSYXSOC_HOME)/perip/spi/rtl)
SOC_VINC_FLAGS += $(addprefix +incdir+, $(SOC_VINC_PATH))

VERILATOR_CFLAGS += -MMD --build -cc  \
				-O3 --x-assign fast --x-initial fast --noassert \
				 --autoflush --timescale "1ns/1ns" --no-timing -Wno-fatal
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\"" -DRESET_VECTOR=$(RESET_VECTOR)
# Don't forget to update `VERILOG_STAMP` when `EMIT_VERILOG_FLAGS` is changed.
EMIT_VERILOG_FLAGS = --reset-vector $(RESET_VECTOR) --without-soc $(WITHOUT_SOC) -- --target-dir $(BUILD_DIR) --split-verilog

ifneq ($(TRACE), notrace)
TRACE_FILENAME ?= $(BUILD_DIR)/trace-$(TOPNAME)-$(HW).$(TRACE)
CXXFLAGS += -DTRACE -DTRACE_$(TRACE) -DTRACE_FILENAME="\"$(TRACE_FILENAME)\""
endif

TRACE_FLAG_vcd     := --trace
TRACE_FLAG_fst     := --trace-fst
TRACE_FLAG_notrace := --no-trace

TRACE_FLAG := $(TRACE_FLAG_$(TRACE))
VERILATOR_CFLAGS += $(TRACE_FLAG)

## 4. Hardware-Specific Configurations
-include ./scripts/$(HW).mk



## 5. Compilation Rules

$(VERILOG_STAMP): $(CHISEL_SRCS)
	@rm -rf $(BUILD_DIR)/*.sv
	./mill -i $(PRJ).runMain utils.EmitVerilog $(EMIT_VERILOG_FLAGS)
	# rename some ports to connect ysyxSoC
	sed -i -e 's/_\(aw\|ar\|w\|r\|b\)_\(\|bits_\)/_\1/g' $(BUILD_DIR)/*.sv
	@touch $@


$(BIN): $(HEADERS) $(CSRCS) $(LINKAGE) $(VERILOG_STAMP) $(SOC_CHISEL_DEPS)
	$(MAKE) -C $(YSYXSOC_HOME) verilog
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) $(SOC_VINC_FLAGS) \
		--top-module $(TOPNAME) $(BUILD_DIR)/*.sv $(SOC_VSRCS) $(CSRCS) $(LINKAGE) \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

$(CACHESIM_BIN): $(CACHESIM_HEADERS) $(CACHESIM_SRCS)
	$(CXX) $(CACHESIM_SRCS) -o $(abspath $(BIN))

## 6. Miscellaneous

### Simulation
sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	$(MAKE) $(BIN)
	$(BIN) $(ARGS) $(IMG)

### Perf
perf:
	$(MAKE) -C $(AM_KERNELS_HOME)/benchmarks/microbench ARCH=riscv32e-ysyxsoc run NPC_TRACE=notrace NPC_HW=fast mainargs=train
	python $(NPC_HOME)/parse_statistics.py $(AM_KERNELS_HOME)/benchmarks/microbench/build/statistics.txt
	# TODO: More statistics processing

### Chisel Test
test:
	./mill -i $(PRJ).test

### Generate Verilog
verilog:
	$(call git_commit, "generate verilog")
	./mill -i $(PRJ).runMain utils.EmitVerilog $(EMIT_VERILOG_FLAGS)

### Cache Sim
cachesim:
	$(MAKE) $(CACHESIM_BIN)
	$(CACHESIM_BIN) $(IMG)

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

clean-all:
	-rm -rf $(BUILD_DIR)
	-rm -rf $(YSYXSOC_HOME)/build/

.PHONY: test verilog reformat checkformat clean sim cachesim
-include ../Makefile
