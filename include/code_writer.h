#ifndef CODE_WRITER_H
#define CODE_WRITER_H

#include "common.h"
#include <stdio.h>

typedef struct {
    FILE *out;
    char file_name[VM_ARG_MAX];
    char current_function[VM_ARG_MAX];
    int compare_counter;
    int call_counter;
    int local_init_counter;
    int uses_eq;
    int uses_gt;
    int uses_lt;
    int uses_call;
    int uses_return;
    int uses_local_init;
} CodeWriter;

int code_writer_init(CodeWriter *writer, FILE *out, const char *file_name);
int code_writer_set_file(CodeWriter *writer, const char *file_name);

CommandType command_type(const char *cmd);
VMLine make_vm_line(const char *cmd, const char *arg1, int arg2);

int write_command(CodeWriter *writer, const VMLine *line);
int write_arithmetic(CodeWriter *writer, const char *cmd);
int write_push_pop(CodeWriter *writer, CommandType type, const char *segment, int index);
int write_label(CodeWriter *writer, const char *label);
int write_goto(CodeWriter *writer, const char *label);
int write_if(CodeWriter *writer, const char *label);
int write_function(CodeWriter *writer, const char *name, int nlocals);
int write_call(CodeWriter *writer, const char *name, int nargs);
int write_return(CodeWriter *writer);
int write_bootstrap(CodeWriter *writer);
int write_shared_routines(CodeWriter *writer);

#endif
