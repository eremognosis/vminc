#ifndef COMMON_H
#define COMMON_H

#define VM_CMD_MAX 32
#define VM_ARG_MAX 128

typedef enum {
    C_EMPTY,
    C_ARITHMETIC,
    C_PUSH,
    C_POP,
    C_LABEL,
    C_GOTO,
    C_IF,
    C_FUNCTION,
    C_RETURN,
    C_CALL,
    C_INVALID
} CommandType;

typedef struct {
    CommandType type;
    char cmd[VM_CMD_MAX];
    char arg1[VM_ARG_MAX];
    int arg2;
} VMLine;

#endif
