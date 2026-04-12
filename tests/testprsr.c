//
// Created by raj on 4/12/26.
//
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/parser.h"

typedef struct {
    char input[100];
    char exp_cmd[50];
    char exp_arg1[50];
    int exp_arg2;
} TestCase;

int main() {
    TestCase tests[] = {
        {"push constant 7", "push", "constant", 7},
        {"  add  // comment", "add", "", 0},
        {"label LOOP  ", "label", "LOOP", 0},
        {"   ", "", "", 0}, // Blank line
        {"// just a comment", "", "", 0},
        {"goto END//no space", "goto", "END", 0},
        {"if-goto","if-goto","",0}
    };

    char cmd[50], arg1[50];
    int arg2;

    for (int i = 0; i < 7; i++) {
        char buffer[100];
        strcpy(buffer, tests[i].input);

        parse(buffer, cmd, arg1, &arg2);

        assert(strcmp(cmd, tests[i].exp_cmd) == 0);
        assert(strcmp(arg1, tests[i].exp_arg1) == 0);
        assert(arg2 == tests[i].exp_arg2);

        printf("Test %d passed: %s\n", i + 1, tests[i].input);
    }

    printf("\nAll basic tests passed. \n");
    return 0;
}