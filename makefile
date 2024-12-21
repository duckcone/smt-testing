SHELL = /bin/bash
CC = gcc
CXX = g++
CFLAGS = -pthread -g -O3
SRC_DIR := ./
SRC_C = $(wildcard $(SRC_DIR)*.c)
EXE_C = $(patsubst %.c,%_exe,$(notdir $(SRC_C)))
EXE = $(EXE_C)

all: ${EXE}

%_exe: $(SRC_DIR)/%.c
	@${CC} ${CFLAGS} $< -o $@

clean:
	@rm $(EXE_C)