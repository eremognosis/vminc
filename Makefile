CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Iinclude
LDFLAGS ?=

TARGET := vmtrans
SRCS := src/main.c src/parser.c src/code_writer.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c include/common.h include/parser.h include/code_writer.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)
