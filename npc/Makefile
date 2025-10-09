# Makefile for NPC (New Processor Core) by caozhanhao

### *Get a more readable version of this Makefile* by `make html` (requires python-markdown)
html:
	cat Makefile | sed 's/^\([^#]\)/    \1/g' | markdown_py > Makefile.html
.PHONY: html

## 1. Basic Setup and Checks

### Project Info
PRJ = npc
TOPNAME = Top

### Run checks only when `sim`
ifneq ($(findstring $(MAKECMDGOALS),sim),)

### Print build info message
$(info # Building NPC [$(HW)])

### Check: environment variable `HW` must be in the supported list
HWS = $(basename $(notdir $(shell ls ./scripts/*.mk)))
ifeq ($(filter $(HWS), $(HW)), )
  $(error Expected $$HW in {$(HWS)}, Got "$(HW)")
endif

### Checks end here
endif



## 2. General Compilation Targets

### Create the destination directory (`build`)
BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
$(shell mkdir -p $(BUILD_DIR))

### Compilation targets
BIN = $(BUILD_DIR)/$(TOPNAME)-$(HW)

### Collect the files to be built and linked
LINKAGE ?=
CSRCS ?=
SCALASRCS = $(shell find $(abspath ./$(PRJ)) -name "*.scala")



## 3. General Compilation Flags

### Verilator
VERILATOR = verilator

### Compilation flags
INC_PATH ?=
VERILATOR_CFLAGS += -MMD --build -cc  \
				-O3 --x-assign fast --x-initial fast --noassert
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""



## 4. Hardware-Specific Configurations
-include ./scripts/$(HW).mk



## 5. Compilation Rules

### Rule (compile)
$(BIN): $(SCALASRCS) $(CSRCS) $(LINKAGE)
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BUILD_DIR)/*.sv
	./mill -i $(PRJ).runMain EmitVerilog --target-dir $(BUILD_DIR)/
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $(BUILD_DIR)/*.sv $(CSRCS) $(LINKAGE) \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))



## 6. Miscellaneous

### Simulation
sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	$(MAKE) $(BIN)
	$(BIN)

### Chisel Test
test:
	./mill -i $(PRJ).test

### Generate Verilog
verilog:
	$(call git_commit, "generate verilog")
	./mill -i $(PRJ).runMain EmitVerilog --target-dir $(BUILD_DIR)

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