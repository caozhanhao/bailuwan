all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
#	@echo "Write this Makefile by your self."
	@echo "Simulating"
	verilator --cc --exe --build -j 0 -Wall ./csrc/main.cpp ./vsrc/example.v --trace-fst

include ../Makefile
