BUILD_DIR = ./build
$(shell mkdir -p $(BUILD_DIR))

PRJ = npc
TOPNAME = Top
INC_PATH ?=
VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc  \
				-O3 --x-assign fast --x-initial fast --noassert
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

LINKAGE ?=
CSRCS ?=

# npc source
SCALASRCS = $(shell find $(abspath ./$(PRJ)) -name "*.scala")

# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

# rules for hardware
-include ./scripts/$(HW).mk

$(BIN): $(SCALASRCS) $(CSRCS) $(LINKAGE)
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BUILD_DIR)/*.sv
	./mill -i $(PRJ).runMain GenVerilog --target-dir $(BUILD_DIR)/
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $(BUILD_DIR)/*.sv $(CSRCS) $(LINKAGE) \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

test:
	./mill -i $(PRJ).test

verilog:
	$(call git_commit, "generate verilog")
	./mill -i $(PRJ).runMain GenVerilog --target-dir $(BUILD_DIR)

reformat:
	./mill -i __.reformat

checkformat:
	./mill -i __.checkFormat

bsp:
	./mill -i mill.bsp.BSP/install

idea:
	./mill -i mill.idea.GenIdea/idea

clean:
	-rm -rf $(BUILD_DIR)

.PHONY: test verilog reformat checkformat clean

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	$(MAKE) $(BIN)
	$(BIN)

-include ../Makefile
