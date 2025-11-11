CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
PYTHON ?= python3

OBJS = sim.o util.o cpu.o dispatch.o

all: sim

run: sim
	./sim && $(PYTHON) plot_core_trace.py

sim: $(OBJS)
	$(CC) $(CFLAGS) -o sim $(OBJS)

sim.o: sim.c sim.h util.h cpu.h dispatch.h
util.o: util.c util.h sim.h
cpu.o: cpu.c cpu.h sim.h
dispatch.o: dispatch.c dispatch.h sim.h

.PHONY: clean
clean:
	rm -f $(OBJS) sim sim_log.txt "core trace.txt" core_trace.txt run_schedule.csv

