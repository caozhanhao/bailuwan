# constraint file
NXDC_FILES = constr/top.nxdc
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

CSRCS += $(shell find $(abspath ./csrc/nvboard) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CSRCS += $(SRC_AUTO_BIND)

LINKAGE += $(NVBOARD_ARCHIVE)

# rules for NVBoard
include $(NVBOARD_HOME)/scripts/nvboard.mk