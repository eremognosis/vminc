//
/*
 CREATED BY : Raj, April 12
 Last Updated : April 12

 Specs (for future me):
 - trims in place
 - removes everything after //
 - ignores blank/comment only lines
 - rejects more than 3 words (only "I love U")
 - error is error
 */





#include "parser.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parse_error(const char *code, const char *expected, const char *got) {
    printf("%s: Expected %s got %s\n", code, expected, got);
    exit(EXIT_FAILURE);
}

static void clear_output(char *cmd, char *arg1, int *arg2) {
    // so that no garbage thingws doesnt affect (imagine "push constant 77x77sweh2jui")
    if (cmd != NULL) {
        cmd[0] = '\0';
    }
    if (arg1 != NULL) {
        arg1[0] = '\0';
    }
    if (arg2 != NULL) {
        *arg2 = 0;
    }
}

static void trim(char *line) {
    // botwway trim
    char *start = line;
    char *end;

    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (*start == '\0') {
        line[0] = '\0';
        return;
    }

    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    if (start != line) {
        memmove(line, start, strlen(start) + 1);
    }
}

static void remcom(char *line) {
    // while comment wont bve there anywaqy sicne machine generated but still
    char *comment = strstr(line, "//");

    if (comment != NULL) {
        *comment = '\0';
    }
}

static void validate_ascii(const char *line) {
    const unsigned char *current = (const unsigned char *)line;

    while (*current != '\0') {
        if (*current > 127) {
            parse_error("ERR_PRS_2", "ASCII input", "non-ASCII input");
        }
        current++;
    }
}

static void copy_word(char *dest, const char *start, size_t length) {
    memcpy(dest, start, length);
    dest[length] = '\0';
}

static int parse_address(const char *word) {
    char *end;
    long value;

    errno = 0;
    value = strtol(word, &end, 10);

    if (*word == '\0' || *end != '\0') {
        parse_error("ERR_PRS_4", "integer for addrwess/number", word);
    }

    if (errno == ERANGE || value > INT_MAX || value < INT_MIN) {
        parse_error("ERR_PRS_5", "integer inside int range", word);
    }

    if (value < 0) {
        parse_error("ERR_PRS_6", "non-negative constant/address", word);
    }

    return (int)value;
}

void parse(char *line, char *cmd, char *arg1, int *arg2) {
    char *word_starts[3];
    size_t word_lengths[3];
    int count = 0;
    char *current;

    if (line == NULL) {
        parse_error("ERR_PRS_0", "input line", "NULL");
    }
    if (cmd == NULL || arg1 == NULL || arg2 == NULL) {
        parse_error("ERR_PRS_1", "output pointer", "NULL");
    }

    clear_output(cmd, arg1, arg2);
    remcom(line);
    trim(line);

    if (line[0] == '\0') {
        return;
    }

    validate_ascii(line);

    current = line;
    while (*current != '\0') {
        char *start;

        while (isspace((unsigned char)*current)) {
            current++;
        }

        if (*current == '\0') {
            break;
        }

        if (count == 3) {
            parse_error("ERR_PRS_3", "1 to 3 words", "more than 3 words"); /// please dont write your home address
        }

        start = current;
        while (*current != '\0' && !isspace((unsigned char)*current)) {
            current++;
        }

        word_starts[count] = start;
        word_lengths[count] = (size_t)(current - start);
        count++;
    }

    copy_word(cmd, word_starts[0], word_lengths[0]);

    if (count >= 2) {
        copy_word(arg1, word_starts[1], word_lengths[1]);
    }

    if (count == 3) {
        char original = word_starts[2][word_lengths[2]];

        word_starts[2][word_lengths[2]] = '\0';
        *arg2 = parse_address(word_starts[2]);
        word_starts[2][word_lengths[2]] = original;
    }
}
