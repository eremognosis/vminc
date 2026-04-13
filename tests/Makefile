CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -I../include

TARGET := vmtranslator_tests
SRCS := testprsr.c ../src/parser.c ../src/code_writer.c

.PHONY: all test clean

all: $(TARGET)

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	$(RM) $(TARGET)