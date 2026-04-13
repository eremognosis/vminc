//
// Created by raj on 4/12/26.
//
// testcases taken from book, online sources and ai generated
//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/code_writer.h"
#include "../include/parser.h"

typedef struct {
    const char *input;
    const char *exp_cmd;
    const char *exp_arg1;
    int exp_arg2;
} ParseCase;

typedef struct {
    const char *cmd;
    CommandType exp_type;
} CommandTypeCase;

static FILE *open_test_stream(void) {
    FILE *stream = tmpfile();

    assert(stream != NULL);
    return stream;
}

static void read_stream(FILE *stream, char *buffer, size_t buffer_size) {
    size_t bytes_read;

    assert(stream != NULL);
    assert(buffer != NULL);
    assert(buffer_size > 0);

    fflush(stream);
    rewind(stream);
    bytes_read = fread(buffer, 1, buffer_size - 1, stream);
    buffer[bytes_read] = '\0';
}

static void assert_stream_equals(FILE *stream, const char *expected) {
    char actual[16384];

    read_stream(stream, actual, sizeof(actual));
    assert(strcmp(actual, expected) == 0);
}

static void assert_stream_contains(FILE *stream, const char *needle) {
    char actual[16384];

    read_stream(stream, actual, sizeof(actual));
    assert(strstr(actual, needle) != NULL);
}

static void reset_writer_stream(FILE **stream, CodeWriter *writer, const char *file_name) {
    if (*stream != NULL) {
        fclose(*stream);
    }

    *stream = open_test_stream();
    assert(code_writer_init(writer, *stream, file_name) == 0);
}

static void run_parse_case(const ParseCase *test, size_t index) {
    char buffer[128];
    char cmd[50];
    char arg1[50];
    int arg2;

    snprintf(buffer, sizeof(buffer), "%s", test->input);
    parse(buffer, cmd, arg1, &arg2);

    assert(strcmp(cmd, test->exp_cmd) == 0);
    assert(strcmp(arg1, test->exp_arg1) == 0);
    assert(arg2 == test->exp_arg2);

    printf("Parse test %zu passed: %s\n", index + 1, test->input);
}

static void expect_parse_failure(const char *input) {
    pid_t pid = fork();
    int status;

    assert(pid >= 0);

    if (pid == 0) {
        char buffer[128];
        char cmd[50];
        char arg1[50];
        int arg2;

        if (input == NULL) {
            parse(NULL, cmd, arg1, &arg2);
        } else {
            snprintf(buffer, sizeof(buffer), "%s", input);
            parse(buffer, cmd, arg1, &arg2);
        }

        _exit(EXIT_SUCCESS);
    }

    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == EXIT_FAILURE);
}

static void expect_parse_failure_bytes(const char *input, size_t length) {
    pid_t pid = fork();
    int status;

    assert(pid >= 0);

    if (pid == 0) {
        char buffer[128];
        char cmd[50];
        char arg1[50];
        int arg2;

        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, input, length);
        parse(buffer, cmd, arg1, &arg2);
        _exit(EXIT_SUCCESS);
    }

    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == EXIT_FAILURE);
}

static void test_parser_success_cases(void) {
    ParseCase tests[] = {
        {"push constant 7", "push", "constant", 7},
        {"  push   constant    0  ", "push", "constant", 0},
        {"push constant 1 // inline comment", "push", "constant", 1},
        {"push constant -1", "push", "constant", -1},
        {"push constant +12", "push", "constant", 12},
        {"push constant 2\t// tab comment", "push", "constant", 2},
        {"add", "add", "", 0},
        {"  add  // comment", "add", "", 0},
        {"sub", "sub", "", 0},
        {"neg", "neg", "", 0},
        {"eq", "eq", "", 0},
        {"gt", "gt", "", 0},
        {"lt", "lt", "", 0},
        {"and", "and", "", 0},
        {"or", "or", "", 0},
        {"not", "not", "", 0},
        {"label LOOP", "label", "LOOP", 0},
        {"label LOOP  // scoped label", "label", "LOOP", 0},
        {"goto END//no space", "goto", "END", 0},
        {"if-goto a", "if-goto", "a", 0},
        {"function Main.main 3", "function", "Main.main", 3},
        {"call Sys.init 0", "call", "Sys.init", 0},
        {"return", "return", "", 0},
        {"push local 3", "push", "local", 3},
        {"pop argument 7", "pop", "argument", 7},
        {"push this 2", "push", "this", 2},
        {"pop that 5", "pop", "that", 5},
        {"push temp 6", "push", "temp", 6},
        {"pop pointer 1", "pop", "pointer", 1},
        {"push static 9", "push", "static", 9},
        {"   ", "", "", 0},
        {"// just a comment", "", "", 0}
    };
    size_t count = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < count; i++) {
        run_parse_case(&tests[i], i);
    }

    printf("All parser success tests passed.\n");
}

static void test_parser_failure_cases(void) {
    char non_ascii_input[] = {'p', 'u', 's', 'h', ' ', 'c', 'o', 'n', 's', 't', 'a', 'n', 't', ' ', '1', (char)0xFF, '\0'};

    expect_parse_failure("push constant 7 extra");
    expect_parse_failure("push constant 999999999999999999999999");
    expect_parse_failure("push constant 7x");
    expect_parse_failure(NULL);
    expect_parse_failure_bytes(non_ascii_input, sizeof(non_ascii_input));

    printf("All parser failure tests passed.\n");
}

static void test_command_helpers(void) {
    CommandTypeCase command_cases[] = {
        {"", C_EMPTY},
        {"add", C_ARITHMETIC},
        {"sub", C_ARITHMETIC},
        {"neg", C_ARITHMETIC},
        {"eq", C_ARITHMETIC},
        {"gt", C_ARITHMETIC},
        {"lt", C_ARITHMETIC},
        {"and", C_ARITHMETIC},
        {"or", C_ARITHMETIC},
        {"not", C_ARITHMETIC},
        {"push", C_PUSH},
        {"pop", C_POP},
        {"label", C_LABEL},
        {"goto", C_GOTO},
        {"if-goto", C_IF},
        {"function", C_FUNCTION},
        {"return", C_RETURN},
        {"call", C_CALL},
        {"something-else", C_INVALID},
        {NULL, C_EMPTY}
    };
    size_t command_count = sizeof(command_cases) / sizeof(command_cases[0]);

    for (size_t i = 0; i < command_count; i++) {
        assert(command_type(command_cases[i].cmd) == command_cases[i].exp_type);
    }

    {
        VMLine line = make_vm_line("push", "constant", 7);
        assert(line.type == C_PUSH);
        assert(strcmp(line.cmd, "push") == 0);
        assert(strcmp(line.arg1, "constant") == 0);
        assert(line.arg2 == 7);
    }

    {
        VMLine line = make_vm_line(NULL, NULL, 0);
        assert(line.type == C_EMPTY);
        assert(line.cmd[0] == '\0');
        assert(line.arg1[0] == '\0');
        assert(line.arg2 == 0);
    }

    printf("All command helper tests passed.\n");
}

static void test_arithmetic_emission(void) {
    FILE *stream = open_test_stream();
    CodeWriter writer;

    assert(code_writer_init(&writer, stream, "Main.vm") == 0);
    assert(write_arithmetic(&writer, "add") == 0);
    assert_stream_equals(stream, "@SP\nAM=M-1\nD=M\nA=A-1\nM=D+M\n");

    reset_writer_stream(&stream, &writer, "Main.vm");
    assert(write_arithmetic(&writer, "sub") == 0);
    assert_stream_equals(stream, "@SP\nAM=M-1\nD=M\nA=A-1\nM=M-D\n");

    reset_writer_stream(&stream, &writer, "Main.vm");
    assert(write_arithmetic(&writer, "neg") == 0);
    assert_stream_equals(stream, "@SP\nA=M-1\nM=-M\n");

    fclose(stream);
    printf("Arithmetic emission tests passed.\n");
}

static void test_comparison_emission(void) {
    FILE *stream = open_test_stream();
    CodeWriter writer;

    assert(code_writer_init(&writer, stream, "Main.vm") == 0);
    assert(write_arithmetic(&writer, "eq") == 0);
    assert(write_arithmetic(&writer, "gt") == 0);
    assert(write_arithmetic(&writer, "lt") == 0);
    assert_stream_contains(stream, "@VM_INTERNAL$ret.cmp.0\nD=A\n@R15\nM=D\n@VM_INTERNAL$EQ\n0;JMP\n(VM_INTERNAL$ret.cmp.0)\n");
    assert_stream_contains(stream, "@VM_INTERNAL$ret.cmp.1\nD=A\n@R15\nM=D\n@VM_INTERNAL$GT\n0;JMP\n(VM_INTERNAL$ret.cmp.1)\n");
    assert_stream_contains(stream, "@VM_INTERNAL$ret.cmp.2\nD=A\n@R15\nM=D\n@VM_INTERNAL$LT\n0;JMP\n(VM_INTERNAL$ret.cmp.2)\n");

    fclose(stream);
    printf("Comparison emission tests passed.\n");
}

static void test_push_pop_emission(void) {
    FILE *stream = open_test_stream();
    CodeWriter writer;

    assert(code_writer_init(&writer, stream, "Main.vm") == 0);
    assert(write_push_pop(&writer, C_PUSH, "constant", -1) == 0);
    assert(write_push_pop(&writer, C_PUSH, "constant", 0) == 0);
    assert(write_push_pop(&writer, C_PUSH, "constant", 1) == 0);
    assert(write_push_pop(&writer, C_PUSH, "constant", 2) == 0);
    assert(write_push_pop(&writer, C_PUSH, "constant", 7) == 0);
    assert_stream_contains(stream, "@SP\nAM=M+1\nA=A-1\nM=-1\n@SP\nAM=M+1\nA=A-1\nM=0\n@SP\nAM=M+1\nA=A-1\nM=1\n@SP\nAM=M+1\nA=A-1\nM=1\nM=M+1\n@7\nD=A\n@SP\nAM=M+1\nA=A-1\nM=D\n");

    reset_writer_stream(&stream, &writer, "Main.vm");
    assert(write_push_pop(&writer, C_PUSH, "local", 0) == 0);
    assert(write_push_pop(&writer, C_PUSH, "local", 2) == 0);
    assert(write_push_pop(&writer, C_POP, "local", 0) == 0);
    assert(write_push_pop(&writer, C_POP, "local", 7) == 0);
    assert_stream_contains(stream, "@LCL\nA=M\nD=M\n@SP\nAM=M+1\nA=A-1\nM=D\n");
    assert_stream_contains(stream, "@LCL\nA=M+1\nA=A+1\nD=M\n@SP\nAM=M+1\nA=A-1\nM=D\n");
    assert_stream_contains(stream, "@SP\nAM=M-1\nD=M\n@LCL\nA=M\nM=D\n");
    assert_stream_contains(stream, "@LCL\nD=M\n@7\nD=D+A\n@R13\nM=D\n@SP\nAM=M-1\nD=M\n@R13\nA=M\nM=D\n");

    reset_writer_stream(&stream, &writer, "Main.vm");
    assert(write_push_pop(&writer, C_PUSH, "temp", 0) == 0);
    assert(write_push_pop(&writer, C_POP, "temp", 7) == 0);
    assert(write_push_pop(&writer, C_PUSH, "pointer", 0) == 0);
    assert(write_push_pop(&writer, C_POP, "pointer", 1) == 0);
    assert(write_push_pop(&writer, C_PUSH, "static", 3) == 0);
    assert(write_push_pop(&writer, C_POP, "static", 4) == 0);
    assert_stream_contains(stream, "@R5\nD=M\n@SP\nAM=M+1\nA=A-1\nM=D\n");
    assert_stream_contains(stream, "@SP\nAM=M-1\nD=M\n@R12\nM=D\n");
    assert_stream_contains(stream, "@THIS\nD=M\n@SP\nAM=M+1\nA=A-1\nM=D\n");
    assert_stream_contains(stream, "@SP\nAM=M-1\nD=M\n@THAT\nM=D\n");
    assert_stream_contains(stream, "@Main.3\nD=M\n@SP\nAM=M+1\nA=A-1\nM=D\n");
    assert_stream_contains(stream, "@SP\nAM=M-1\nD=M\n@Main.4\nM=D\n");

    assert(write_push_pop(&writer, C_PUSH, "temp", 8) == -1);
    assert(write_push_pop(&writer, C_PUSH, "pointer", 2) == -1);
    assert(write_push_pop(&writer, C_PUSH, "constant", -2) == -1);

    fclose(stream);
    printf("Push/pop emission tests passed.\n");
}

static void test_label_flow_emission(void) {
    FILE *stream = open_test_stream();
    CodeWriter writer;

    assert(code_writer_init(&writer, stream, "Prog.vm") == 0);
    assert(write_label(&writer, "LOOP") == 0);
    assert(write_goto(&writer, "LOOP") == 0);
    assert(write_if(&writer, "LOOP") == 0);
    assert_stream_equals(stream, "(Prog$LOOP)\n@Prog$LOOP\n0;JMP\n@SP\nAM=M-1\nD=M\n@Prog$LOOP\nD;JNE\n");

    reset_writer_stream(&stream, &writer, "Prog.vm");
    assert(write_function(&writer, "Main.main", 3) == 0);
    assert(write_label(&writer, "LOOP") == 0);
    assert(write_goto(&writer, "LOOP") == 0);
    assert(write_if(&writer, "LOOP") == 0);
    assert_stream_contains(stream, "(Main.main)\n@SP\nAM=M+1\nA=A-1\nM=0\n@SP\nAM=M+1\nA=A-1\nM=0\n@SP\nAM=M+1\nA=A-1\nM=0\n(Main.main$LOOP)\n@Main.main$LOOP\n0;JMP\n@SP\nAM=M-1\nD=M\n@Main.main$LOOP\nD;JNE\n");

    fclose(stream);
    printf("Label flow emission tests passed.\n");
}

static void test_function_call_return_bootstrap(void) {
    FILE *stream = open_test_stream();
    CodeWriter writer;

    assert(code_writer_init(&writer, stream, "Prog.vm") == 0);
    assert(write_function(&writer, "Big.init", 6) == 0);
    assert_stream_contains(stream, "(Big.init)\n@6\nD=A\n@R13\nM=D\n@VM_INTERNAL$ret.init.0\nD=A\n@R15\nM=D\n@VM_INTERNAL$INIT_LOCALS\n0;JMP\n(VM_INTERNAL$ret.init.0)\n");

    reset_writer_stream(&stream, &writer, "Prog.vm");
    assert(write_call(&writer, "Sys.init", 0) == 0);
    assert_stream_equals(stream, "@Sys.init$ret.0\nD=A\n@R13\nM=D\n@Sys.init\nD=A\n@R14\nM=D\n@5\nD=A\n@R15\nM=D\n@VM_INTERNAL$CALL\n0;JMP\n(Sys.init$ret.0)\n");

    reset_writer_stream(&stream, &writer, "Prog.vm");
    assert(write_function(&writer, "Main.main", 1) == 0);
    assert(write_call(&writer, "Foo.bar", 2) == 0);
    assert_stream_contains(stream, "(Main.main)\n@SP\nAM=M+1\nA=A-1\nM=0\n@Main.main$ret.0\nD=A\n@R13\nM=D\n@Foo.bar\nD=A\n@R14\nM=D\n@7\nD=A\n@R15\nM=D\n@VM_INTERNAL$CALL\n0;JMP\n(Main.main$ret.0)\n");

    reset_writer_stream(&stream, &writer, "Prog.vm");
    assert(write_return(&writer) == 0);
    assert_stream_equals(stream, "@VM_INTERNAL$RETURN\n0;JMP\n");

    reset_writer_stream(&stream, &writer, "Prog.vm");
    assert(write_bootstrap(&writer) == 0);
    assert_stream_equals(stream, "@256\nD=A\n@SP\nM=D\n@Sys.init$ret.0\nD=A\n@R13\nM=D\n@Sys.init\nD=A\n@R14\nM=D\n@5\nD=A\n@R15\nM=D\n@VM_INTERNAL$CALL\n0;JMP\n(Sys.init$ret.0)\n");

    fclose(stream);
    printf("Function/call/return/bootstrap tests passed.\n");
}

static void test_shared_routines_emission(void) {
    FILE *stream = open_test_stream();
    CodeWriter writer;

    assert(code_writer_init(&writer, stream, "Prog.vm") == 0);
    assert(write_arithmetic(&writer, "eq") == 0);
    assert(write_call(&writer, "Foo.bar", 1) == 0);
    assert(write_return(&writer) == 0);
    assert(write_function(&writer, "Main.init", 6) == 0);
    assert(write_shared_routines(&writer) == 0);
    assert_stream_contains(stream, "(VM_INTERNAL$END)\n@VM_INTERNAL$END\n0;JMP\n");
    assert_stream_contains(stream, "(VM_INTERNAL$EQ)\n");
    assert_stream_contains(stream, "(VM_INTERNAL$CALL)\n");
    assert_stream_contains(stream, "(VM_INTERNAL$RETURN)\n");
    assert_stream_contains(stream, "(VM_INTERNAL$INIT_LOCALS)\n");

    fclose(stream);
    printf("Shared routines emission tests passed.\n");
}

static void test_write_command_dispatch(void) {
    FILE *stream = open_test_stream();
    CodeWriter writer;

    assert(code_writer_init(&writer, stream, "Prog.vm") == 0);
    assert(write_command(&writer, &(VMLine){.type = C_ARITHMETIC, .cmd = "add"}) == 0);
    assert(write_command(&writer, &(VMLine){.type = C_PUSH, .arg1 = "constant", .arg2 = 7}) == 0);
    assert(write_command(&writer, &(VMLine){.type = C_LABEL, .arg1 = "LOOP"}) == 0);
    assert(write_command(&writer, &(VMLine){.type = C_RETURN}) == 0);
    assert_stream_equals(stream, "@SP\nAM=M-1\nD=M\nA=A-1\nM=D+M\n@7\nD=A\n@SP\nAM=M+1\nA=A-1\nM=D\n(Prog$LOOP)\n@VM_INTERNAL$RETURN\n0;JMP\n");

    fclose(stream);
    printf("Write-command dispatch tests passed.\n");
}

int main(void) {
    test_parser_success_cases();
    test_parser_failure_cases();
    test_command_helpers();
    test_arithmetic_emission();
    test_comparison_emission();
    test_push_pop_emission();
    test_label_flow_emission();
    test_function_call_return_bootstrap();
    test_shared_routines_emission();
    test_write_command_dispatch();

    printf("\nAll unit tests passed.\n");
    return 0;
}
