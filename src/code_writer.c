#include "code_writer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOCAL_INIT_INLINE_LIMIT 4

static int emit(CodeWriter *writer, const char *format, ...) {
    va_list args;
    int written;

    if (writer == NULL || writer->out == NULL || format == NULL) {
        return -1;
    }

    va_start(args, format);
    written = vfprintf(writer->out, format, args);
    va_end(args);

    return written < 0 ? -1 : 0;
}

static int copy_text(char *dest, size_t dest_size, const char *src) {
    int written;

    if (dest == NULL || dest_size == 0 || src == NULL) {
        return -1;
    }

    written = snprintf(dest, dest_size, "%s", src);
    return written < 0 || (size_t)written >= dest_size ? -1 : 0;
}

static int is_arithmetic(const char *cmd) {
    return strcmp(cmd, "add") == 0 ||
           strcmp(cmd, "sub") == 0 ||
           strcmp(cmd, "neg") == 0 ||
           strcmp(cmd, "eq") == 0 ||
           strcmp(cmd, "gt") == 0 ||
           strcmp(cmd, "lt") == 0 ||
           strcmp(cmd, "and") == 0 ||
           strcmp(cmd, "or") == 0 ||
           strcmp(cmd, "not") == 0;
}

static const char *base_segment_symbol(const char *segment) {
    if (strcmp(segment, "argument") == 0) {
        return "ARG";
    }
    if (strcmp(segment, "local") == 0) {
        return "LCL";
    }
    if (strcmp(segment, "this") == 0) {
        return "THIS";
    }
    if (strcmp(segment, "that") == 0) {
        return "THAT";
    }

    return NULL;
}

static const char *pointer_symbol(int index) {
    if (index == 0) {
        return "THIS";
    }
    if (index == 1) {
        return "THAT";
    }

    return NULL;
}

static int push_d(CodeWriter *writer) {
    return emit(writer,
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n");
}

static int pop_to_d(CodeWriter *writer) {
    return emit(writer,
                "@SP\n"
                "AM=M-1\n"
                "D=M\n");
}

static int write_scoped_label(CodeWriter *writer, const char *label, char *out, size_t out_size) {
    const char *scope;
    int written;

    if (writer == NULL || label == NULL || label[0] == '\0' || out == NULL || out_size == 0) {
        return -1;
    }

    scope = writer->current_function[0] != '\0' ? writer->current_function : writer->file_name;
    if (scope[0] == '\0') {
        return copy_text(out, out_size, label);
    }

    written = snprintf(out, out_size, "%s$%s", scope, label);
    return written < 0 || (size_t)written >= out_size ? -1 : 0;
}

static int write_comparison(CodeWriter *writer, const char *routine_label, int *uses_routine) {
    int label_id;

    if (writer == NULL || routine_label == NULL || uses_routine == NULL) {
        return -1;
    }

    *uses_routine = 1;
    label_id = writer->compare_counter++;
    return emit(writer,
                "@VM_INTERNAL$ret.cmp.%d\n"
                "D=A\n"
                "@R15\n"
                "M=D\n"
                "@%s\n"
                "0;JMP\n"
                "(VM_INTERNAL$ret.cmp.%d)\n",
                label_id, routine_label, label_id);
}

static int write_local_init(CodeWriter *writer, int nlocals) {
    int label_id;

    if (writer == NULL || nlocals < 0) {
        return -1;
    }

    writer->uses_local_init = 1;
    label_id = writer->local_init_counter++;
    return emit(writer,
                "@%d\n"
                "D=A\n"
                "@R13\n"
                "M=D\n"
                "@VM_INTERNAL$ret.init.%d\n"
                "D=A\n"
                "@R15\n"
                "M=D\n"
                "@VM_INTERNAL$INIT_LOCALS\n"
                "0;JMP\n"
                "(VM_INTERNAL$ret.init.%d)\n",
                nlocals, label_id, label_id);
}

int code_writer_init(CodeWriter *writer, FILE *out, const char *file_name) {
    if (writer == NULL || out == NULL) {
        return -1;
    }

    memset(writer, 0, sizeof(*writer));
    writer->out = out;

    return code_writer_set_file(writer, file_name);
}

int code_writer_set_file(CodeWriter *writer, const char *file_name) {
    const char *base;
    const char *slash;
    const char *backslash;
    char *dot;

    if (writer == NULL) {
        return -1;
    }

    base = file_name == NULL || file_name[0] == '\0' ? "Static" : file_name;
    slash = strrchr(base, '/');
    backslash = strrchr(base, '\\');

    if (slash != NULL && slash + 1 > base) {
        base = slash + 1;
    }
    if (backslash != NULL && backslash + 1 > base) {
        base = backslash + 1;
    }

    if (copy_text(writer->file_name, sizeof(writer->file_name), base) != 0) {
        return -1;
    }

    dot = strrchr(writer->file_name, '.');
    if (dot != NULL && dot != writer->file_name) {
        *dot = '\0';
    }

    writer->current_function[0] = '\0';
    return 0;
}

CommandType command_type(const char *cmd) {
    if (cmd == NULL || cmd[0] == '\0') {
        return C_EMPTY;
    }

    if (is_arithmetic(cmd)) {
        return C_ARITHMETIC;
    }
    if (strcmp(cmd, "push") == 0) {
        return C_PUSH;
    }
    if (strcmp(cmd, "pop") == 0) {
        return C_POP;
    }
    if (strcmp(cmd, "label") == 0) {
        return C_LABEL;
    }
    if (strcmp(cmd, "goto") == 0) {
        return C_GOTO;
    }
    if (strcmp(cmd, "if-goto") == 0) {
        return C_IF;
    }
    if (strcmp(cmd, "function") == 0) {
        return C_FUNCTION;
    }
    if (strcmp(cmd, "return") == 0) {
        return C_RETURN;
    }
    if (strcmp(cmd, "call") == 0) {
        return C_CALL;
    }

    return C_INVALID;
}

VMLine make_vm_line(const char *cmd, const char *arg1, int arg2) {
    VMLine line;

    line.type = command_type(cmd);
    line.cmd[0] = '\0';
    line.arg1[0] = '\0';
    line.arg2 = arg2;

    if (cmd != NULL) {
        snprintf(line.cmd, sizeof(line.cmd), "%s", cmd);
    }
    if (arg1 != NULL) {
        snprintf(line.arg1, sizeof(line.arg1), "%s", arg1);
    }

    return line;
}

int write_command(CodeWriter *writer, const VMLine *line) {
    if (writer == NULL || line == NULL) {
        return -1;
    }

    switch (line->type) {
        case C_EMPTY:
            return 0;
        case C_ARITHMETIC:
            return write_arithmetic(writer, line->cmd);
        case C_PUSH:
        case C_POP:
            return write_push_pop(writer, line->type, line->arg1, line->arg2);
        case C_LABEL:
            return write_label(writer, line->arg1);
        case C_GOTO:
            return write_goto(writer, line->arg1);
        case C_IF:
            return write_if(writer, line->arg1);
        case C_FUNCTION:
            return write_function(writer, line->arg1, line->arg2);
        case C_CALL:
            return write_call(writer, line->arg1, line->arg2);
        case C_RETURN:
            return write_return(writer);
        case C_INVALID:
            return -1;
    }

    return -1;
}

int write_arithmetic(CodeWriter *writer, const char *cmd) {
    if (writer == NULL || cmd == NULL) {
        return -1;
    }

    if (strcmp(cmd, "add") == 0) {
        return emit(writer, "@SP\nAM=M-1\nD=M\nA=A-1\nM=D+M\n");
    }
    if (strcmp(cmd, "sub") == 0) {
        return emit(writer, "@SP\nAM=M-1\nD=M\nA=A-1\nM=M-D\n");
    }
    if (strcmp(cmd, "and") == 0) {
        return emit(writer, "@SP\nAM=M-1\nD=M\nA=A-1\nM=D&M\n");
    }
    if (strcmp(cmd, "or") == 0) {
        return emit(writer, "@SP\nAM=M-1\nD=M\nA=A-1\nM=D|M\n");
    }
    if (strcmp(cmd, "neg") == 0) {
        return emit(writer, "@SP\nA=M-1\nM=-M\n");
    }
    if (strcmp(cmd, "not") == 0) {
        return emit(writer, "@SP\nA=M-1\nM=!M\n");
    }
    if (strcmp(cmd, "eq") == 0) {
        return write_comparison(writer, "VM_INTERNAL$EQ", &writer->uses_eq);
    }
    if (strcmp(cmd, "gt") == 0) {
        return write_comparison(writer, "VM_INTERNAL$GT", &writer->uses_gt);
    }
    if (strcmp(cmd, "lt") == 0) {
        return write_comparison(writer, "VM_INTERNAL$LT", &writer->uses_lt);
    }

    return -1;
}

int write_push_pop(CodeWriter *writer, CommandType type, const char *segment, int index) {
    const char *base;
    const char *pointer;

    if (writer == NULL || segment == NULL || index < 0 || (type != C_PUSH && type != C_POP)) {
        return -1;
    }

    if (type == C_PUSH && strcmp(segment, "constant") == 0) {
        if (emit(writer, "@%d\nD=A\n", index) != 0) {
            return -1;
        }
        return push_d(writer);
    }

    base = base_segment_symbol(segment);
    if (base != NULL) {
        if (type == C_PUSH) {
            if (emit(writer, "@%s\nD=M\n@%d\nA=D+A\nD=M\n", base, index) != 0) {
                return -1;
            }
            return push_d(writer);
        }

        if (emit(writer, "@%s\nD=M\n@%d\nD=D+A\n@R13\nM=D\n", base, index) != 0) {
            return -1;
        }
        if (pop_to_d(writer) != 0) {
            return -1;
        }
        return emit(writer, "@R13\nA=M\nM=D\n");
    }

    if (strcmp(segment, "temp") == 0) {
        if (index > 7) {
            return -1;
        }

        if (type == C_PUSH) {
            if (emit(writer, "@R%d\nD=M\n", index + 5) != 0) {
                return -1;
            }
            return push_d(writer);
        }

        if (pop_to_d(writer) != 0) {
            return -1;
        }
        return emit(writer, "@R%d\nM=D\n", index + 5);
    }

    if (strcmp(segment, "pointer") == 0) {
        pointer = pointer_symbol(index);
        if (pointer == NULL) {
            return -1;
        }

        if (type == C_PUSH) {
            if (emit(writer, "@%s\nD=M\n", pointer) != 0) {
                return -1;
            }
            return push_d(writer);
        }

        if (pop_to_d(writer) != 0) {
            return -1;
        }
        return emit(writer, "@%s\nM=D\n", pointer);
    }

    if (strcmp(segment, "static") == 0) {
        if (type == C_PUSH) {
            if (emit(writer, "@%s.%d\nD=M\n", writer->file_name, index) != 0) {
                return -1;
            }
            return push_d(writer);
        }

        if (pop_to_d(writer) != 0) {
            return -1;
        }
        return emit(writer, "@%s.%d\nM=D\n", writer->file_name, index);
    }

    return -1;
}

int write_label(CodeWriter *writer, const char *label) {
    char scoped[VM_ARG_MAX * 2];

    if (write_scoped_label(writer, label, scoped, sizeof(scoped)) != 0) {
        return -1;
    }

    return emit(writer, "(%s)\n", scoped);
}

int write_goto(CodeWriter *writer, const char *label) {
    char scoped[VM_ARG_MAX * 2];

    if (write_scoped_label(writer, label, scoped, sizeof(scoped)) != 0) {
        return -1;
    }

    return emit(writer, "@%s\n0;JMP\n", scoped);
}

int write_if(CodeWriter *writer, const char *label) {
    char scoped[VM_ARG_MAX * 2];

    if (write_scoped_label(writer, label, scoped, sizeof(scoped)) != 0) {
        return -1;
    }
    if (pop_to_d(writer) != 0) {
        return -1;
    }

    return emit(writer, "@%s\nD;JNE\n", scoped);
}

int write_function(CodeWriter *writer, const char *name, int nlocals) {
    int i;

    if (writer == NULL || name == NULL || name[0] == '\0' || nlocals < 0) {
        return -1;
    }
    if (copy_text(writer->current_function, sizeof(writer->current_function), name) != 0) {
        return -1;
    }
    if (emit(writer, "(%s)\n", name) != 0) {
        return -1;
    }

    if (nlocals > LOCAL_INIT_INLINE_LIMIT) {
        return write_local_init(writer, nlocals);
    }

    for (i = 0; i < nlocals; i++) {
        if (emit(writer, "@0\nD=A\n") != 0 || push_d(writer) != 0) {
            return -1;
        }
    }

    return 0;
}

int write_call(CodeWriter *writer, const char *name, int nargs) {
    char return_label[VM_ARG_MAX * 2];
    const char *return_scope;
    int written;

    if (writer == NULL || name == NULL || name[0] == '\0' || nargs < 0) {
        return -1;
    }

    return_scope = writer->current_function[0] != '\0' ? writer->current_function : name;
    written = snprintf(return_label, sizeof(return_label), "%s$ret.%d", return_scope, writer->call_counter++);
    if (written < 0 || (size_t)written >= sizeof(return_label)) {
        return -1;
    }

    writer->uses_call = 1;
    return emit(writer,
                "@%s\n"
                "D=A\n"
                "@R13\n"
                "M=D\n"
                "@%s\n"
                "D=A\n"
                "@R14\n"
                "M=D\n"
                "@%d\n"
                "D=A\n"
                "@R15\n"
                "M=D\n"
                "@VM_INTERNAL$CALL\n"
                "0;JMP\n"
                "(%s)\n",
                return_label, name, nargs + 5, return_label);
}

int write_return(CodeWriter *writer) {
    if (writer == NULL) {
        return -1;
    }

    writer->uses_return = 1;
    return emit(writer,
                "@VM_INTERNAL$RETURN\n"
                "0;JMP\n");
}

int write_bootstrap(CodeWriter *writer) {
    if (emit(writer, "@256\nD=A\n@SP\nM=D\n") != 0) {
        return -1;
    }

    return write_call(writer, "Sys.init", 0);
}

static int write_comparison_routine(CodeWriter *writer,
                                    const char *label,
                                    const char *true_label,
                                    const char *jump) {
    return emit(writer,
                "(%s)\n"
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "A=A-1\n"
                "D=M-D\n"
                "M=-1\n"
                "@%s\n"
                "D;%s\n"
                "@SP\n"
                "A=M-1\n"
                "M=0\n"
                "(%s)\n"
                "@R15\n"
                "A=M\n"
                "0;JMP\n",
                label, true_label, jump, true_label);
}

static int write_call_routine(CodeWriter *writer) {
    return emit(writer,
                "(VM_INTERNAL$CALL)\n"
                "@R13\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n"
                "@LCL\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n"
                "@ARG\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n"
                "@THIS\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n"
                "@THAT\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n"
                "@SP\n"
                "D=M\n"
                "@R15\n"
                "D=D-M\n"
                "@ARG\n"
                "M=D\n"
                "@SP\n"
                "D=M\n"
                "@LCL\n"
                "M=D\n"
                "@R14\n"
                "A=M\n"
                "0;JMP\n");
}

static int write_return_routine(CodeWriter *writer) {
    return emit(writer,
                "(VM_INTERNAL$RETURN)\n"
                "@LCL\n"
                "D=M\n"
                "@R13\n"
                "M=D\n"
                "@5\n"
                "A=D-A\n"
                "D=M\n"
                "@R14\n"
                "M=D\n"
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@ARG\n"
                "A=M\n"
                "M=D\n"
                "@ARG\n"
                "D=M+1\n"
                "@SP\n"
                "M=D\n"
                "@R13\n"
                "AM=M-1\n"
                "D=M\n"
                "@THAT\n"
                "M=D\n"
                "@R13\n"
                "AM=M-1\n"
                "D=M\n"
                "@THIS\n"
                "M=D\n"
                "@R13\n"
                "AM=M-1\n"
                "D=M\n"
                "@ARG\n"
                "M=D\n"
                "@R13\n"
                "AM=M-1\n"
                "D=M\n"
                "@LCL\n"
                "M=D\n"
                "@R14\n"
                "A=M\n"
                "0;JMP\n");
}

static int write_local_init_routine(CodeWriter *writer) {
    return emit(writer,
                "(VM_INTERNAL$INIT_LOCALS)\n"
                "@SP\n"
                "A=M\n"
                "M=0\n"
                "@SP\n"
                "M=M+1\n"
                "@R13\n"
                "MD=M-1\n"
                "@VM_INTERNAL$INIT_LOCALS\n"
                "D;JGT\n"
                "@R15\n"
                "A=M\n"
                "0;JMP\n");
}

int write_shared_routines(CodeWriter *writer) {
    if (writer == NULL) {
        return -1;
    }

    if (!writer->uses_eq &&
        !writer->uses_gt &&
        !writer->uses_lt &&
        !writer->uses_call &&
        !writer->uses_return &&
        !writer->uses_local_init) {
        return 0;
    }

    if (emit(writer,
             "(VM_INTERNAL$END)\n"
             "@VM_INTERNAL$END\n"
             "0;JMP\n") != 0) {
        return -1;
    }

    if (writer->uses_eq &&
        write_comparison_routine(writer, "VM_INTERNAL$EQ", "VM_INTERNAL$EQ_TRUE", "JEQ") != 0) {
        return -1;
    }
    if (writer->uses_gt &&
        write_comparison_routine(writer, "VM_INTERNAL$GT", "VM_INTERNAL$GT_TRUE", "JGT") != 0) {
        return -1;
    }
    if (writer->uses_lt &&
        write_comparison_routine(writer, "VM_INTERNAL$LT", "VM_INTERNAL$LT_TRUE", "JLT") != 0) {
        return -1;
    }
    if (writer->uses_call && write_call_routine(writer) != 0) {
        return -1;
    }
    if (writer->uses_return && write_return_routine(writer) != 0) {
        return -1;
    }
    if (writer->uses_local_init && write_local_init_routine(writer) != 0) {
        return -1;
    }

    return 0;
}
