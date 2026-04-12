#define _XOPEN_SOURCE 700

#include "code_writer.h"
#include "parser.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define VM_LINE_MAX 512

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} PathList;

static void vm_error(const char *code, const char *expected, const char *got) {
    fprintf(stderr, "%s: Expected %s got %s\n", code, expected, got);
}

static void vm_errorf(const char *code, const char *expected, const char *format, ...) {
    char got[PATH_MAX + VM_LINE_MAX];
    va_list args;
    int written;

    va_start(args, format);
    written = vsnprintf(got, sizeof(got), format, args);
    va_end(args);

    if (written < 0 || written >= (int)sizeof(got)) {
        vm_error(code, expected, "unprintable error detail");
        return;
    }

    vm_error(code, expected, got);
}

static void usage(const char *program) {
    fprintf(stderr, "Usage: %s <file.vm|folder> [output.asm]\n", program);
}

static int has_suffix(const char *text, const char *suffix) {
    size_t text_len;
    size_t suffix_len;

    if (text == NULL || suffix == NULL) {
        return 0;
    }

    text_len = strlen(text);
    suffix_len = strlen(suffix);
    return text_len >= suffix_len &&
           strcmp(text + text_len - suffix_len, suffix) == 0;
}

static const char *path_basename(const char *path) {
    const char *end;
    const char *base;

    if (path == NULL || path[0] == '\0') {
        return "";
    }

    end = path + strlen(path);
    while (end > path && end[-1] == '/') {
        end--;
    }

    base = end;
    while (base > path && base[-1] != '/') {
        base--;
    }

    return base;
}

static int path_dirname(const char *path, char *out, size_t out_size) {
    const char *slash;
    size_t length;

    if (path == NULL || out == NULL || out_size == 0) {
        return -1;
    }

    slash = strrchr(path, '/');
    if (slash == NULL) {
        return snprintf(out, out_size, ".") < (int)out_size ? 0 : -1;
    }

    if (slash == path) {
        return snprintf(out, out_size, "/") < (int)out_size ? 0 : -1;
    }

    length = (size_t)(slash - path);
    if (length >= out_size) {
        return -1;
    }

    memcpy(out, path, length);
    out[length] = '\0';
    return 0;
}

static int copy_trimmed_basename(const char *path, char *out, size_t out_size) {
    const char *base;
    const char *end;
    size_t length;

    if (path == NULL || out == NULL || out_size == 0) {
        return -1;
    }

    base = path_basename(path);
    end = path + strlen(path);
    while (end > base && end[-1] == '/') {
        end--;
    }

    length = (size_t)(end - base);
    if (length == 0 || length >= out_size) {
        return -1;
    }

    memcpy(out, base, length);
    out[length] = '\0';
    return 0;
}

static int join_path(const char *folder, const char *name, char *out, size_t out_size) {
    int written;
    size_t folder_length;

    if (folder == NULL || name == NULL || out == NULL || out_size == 0) {
        return -1;
    }

    folder_length = strlen(folder);
    if (folder_length == 0) {
        written = snprintf(out, out_size, "%s", name);
    } else if (strcmp(folder, "/") == 0) {
        written = snprintf(out, out_size, "/%s", name);
    } else if (folder[folder_length - 1] == '/') {
        written = snprintf(out, out_size, "%s%s", folder, name);
    } else {
        written = snprintf(out, out_size, "%s/%s", folder, name);
    }

    return written >= 0 && written < (int)out_size ? 0 : -1;
}

static int replace_vm_extension(const char *input_path, char *out, size_t out_size) {
    char dir[PATH_MAX];
    char name[PATH_MAX];
    char *dot;

    if (!has_suffix(input_path, ".vm")) {
        vm_errorf("ERR_CLI_3", "input file ending with .vm", "'%s'", input_path);
        return -1;
    }

    if (path_dirname(input_path, dir, sizeof(dir)) != 0 ||
        copy_trimmed_basename(input_path, name, sizeof(name)) != 0) {
        vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s'", input_path);
        return -1;
    }

    dot = strrchr(name, '.');
    if (dot != NULL) {
        *dot = '\0';
    }

    if (strcmp(dir, ".") == 0) {
        if (snprintf(out, out_size, "%s.asm", name) >= (int)out_size) {
            vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s.asm'", name);
            return -1;
        }

        return 0;
    }

    if (snprintf(name + strlen(name), sizeof(name) - strlen(name), ".asm") >=
        (int)(sizeof(name) - strlen(name))) {
        vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s.asm'", name);
        return -1;
    }

    if (join_path(dir, name, out, out_size) != 0) {
        vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s/%s'", dir, name);
        return -1;
    }

    return 0;
}

static int default_folder_output(const char *input_path, char *out, size_t out_size) {
    char folder_name[PATH_MAX];
    char folder_path[PATH_MAX];
    char output_name[PATH_MAX];
    char resolved_path[PATH_MAX];
    size_t folder_length;

    if (copy_trimmed_basename(input_path, folder_name, sizeof(folder_name)) != 0) {
        vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s'", input_path);
        return -1;
    }
    if (strcmp(folder_name, ".") == 0 || strcmp(folder_name, "..") == 0) {
        if (realpath(input_path, resolved_path) == NULL ||
            copy_trimmed_basename(resolved_path, folder_name, sizeof(folder_name)) != 0) {
            vm_errorf("ERR_CLI_5", "resolvable folder path", "'%s' (%s)", input_path, strerror(errno));
            return -1;
        }
    }

    if (snprintf(folder_path, sizeof(folder_path), "%s", input_path) >= (int)sizeof(folder_path)) {
        vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s'", input_path);
        return -1;
    }

    folder_length = strlen(folder_path);
    while (folder_length > 1 && folder_path[folder_length - 1] == '/') {
        folder_path[--folder_length] = '\0';
    }

    if (snprintf(output_name, sizeof(output_name), "%s.asm", folder_name) >= (int)sizeof(output_name)) {
        vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s.asm'", folder_name);
        return -1;
    }

    if (join_path(folder_path, output_name, out, out_size) != 0) {
        vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s/%s'", folder_path, output_name);
        return -1;
    }

    return 0;
}

static char *duplicate_text(const char *text) {
    char *copy;
    size_t length;

    if (text == NULL) {
        return NULL;
    }

    length = strlen(text) + 1;
    copy = malloc(length);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length);
    return copy;
}

static int path_list_push(PathList *list, const char *path) {
    char **resized;
    char *copy;
    size_t new_capacity;

    if (list == NULL || path == NULL) {
        return -1;
    }

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        resized = realloc(list->items, new_capacity * sizeof(*list->items));
        if (resized == NULL) {
            return -1;
        }

        list->items = resized;
        list->capacity = new_capacity;
    }

    copy = duplicate_text(path);
    if (copy == NULL) {
        return -1;
    }

    list->items[list->count++] = copy;
    return 0;
}

static void path_list_free(PathList *list) {
    size_t i;

    if (list == NULL) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int compare_paths(const void *left, const void *right) {
    const char *left_path = *(const char *const *)left;
    const char *right_path = *(const char *const *)right;

    return strcmp(left_path, right_path);
}

static int count_words(const char *line) {
    int count = 0;
    const char *current = line;

    while (current != NULL && *current != '\0') {
        while (isspace((unsigned char)*current)) {
            current++;
        }
        if (*current == '\0') {
            break;
        }

        count++;
        while (*current != '\0' && !isspace((unsigned char)*current)) {
            current++;
        }
    }

    return count;
}

static int expected_word_count(CommandType type) {
    switch (type) {
        case C_EMPTY:
            return 0;
        case C_ARITHMETIC:
        case C_RETURN:
            return 1;
        case C_LABEL:
        case C_GOTO:
        case C_IF:
            return 2;
        case C_PUSH:
        case C_POP:
        case C_FUNCTION:
        case C_CALL:
            return 3;
        case C_INVALID:
            return -1;
    }

    return -1;
}

static int validate_command_shape(const char *input_path, int line_number, const char *line, const VMLine *vm_line) {
    int actual;
    int expected;

    if (vm_line->type == C_INVALID) {
        vm_errorf("ERR_TRN_2", "known VM command", "%s:%d '%s'", input_path, line_number, vm_line->cmd);
        return -1;
    }

    actual = count_words(line);
    expected = expected_word_count(vm_line->type);
    if (actual != expected) {
        vm_errorf("ERR_TRN_3",
                  "correct word count for VM command",
                  "%s:%d '%s' expected %d words but found %d",
                  input_path,
                  line_number,
                  vm_line->cmd,
                  expected,
                  actual);
        return -1;
    }

    return 0;
}

static int collect_vm_files(const char *folder, PathList *files) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(folder);
    if (dir == NULL) {
        vm_errorf("ERR_IO_0", "readable folder", "'%s' (%s)", folder, strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX];

        if (entry->d_name[0] == '.' || !has_suffix(entry->d_name, ".vm")) {
            continue;
        }

        if (join_path(folder, entry->d_name, full_path, sizeof(full_path)) != 0) {
            vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s/%s'", folder, entry->d_name);
            closedir(dir);
            return -1;
        }

        if (path_list_push(files, full_path) != 0) {
            vm_error("ERR_TRN_0", "enough memory to collect VM files", "out of memory");
            closedir(dir);
            return -1;
        }
    }

    closedir(dir);
    qsort(files->items, files->count, sizeof(*files->items), compare_paths);
    return 0;
}

static int translate_file(CodeWriter *writer, const char *input_path) {
    FILE *in;
    char line[VM_LINE_MAX];
    int line_number = 0;

    if (code_writer_set_file(writer, input_path) != 0) {
        vm_errorf("ERR_TRN_1", "VM file context", "'%s'", input_path);
        return -1;
    }

    in = fopen(input_path, "r");
    if (in == NULL) {
        vm_errorf("ERR_IO_2", "readable input file", "'%s' (%s)", input_path, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), in) != NULL) {
        char cmd[VM_LINE_MAX];
        char arg1[VM_LINE_MAX];
        int arg2;
        VMLine vm_line;

        line_number++;
        if (strchr(line, '\n') == NULL && !feof(in)) {
            vm_errorf("ERR_IO_3", "line shorter than VM_LINE_MAX", "%s:%d", input_path, line_number);
            fclose(in);
            return -1;
        }

        parse(line, cmd, arg1, &arg2);
        if (strlen(cmd) >= VM_CMD_MAX || strlen(arg1) >= VM_ARG_MAX) {
            vm_errorf("ERR_TRN_4", "command and argument inside writer limits", "%s:%d", input_path, line_number);
            fclose(in);
            return -1;
        }

        vm_line = make_vm_line(cmd, arg1, arg2);
        if (validate_command_shape(input_path, line_number, line, &vm_line) != 0) {
            fclose(in);
            return -1;
        }

        if (write_command(writer, &vm_line) != 0) {
            vm_errorf("ERR_TRN_5", "translatable VM command", "%s:%d '%s'", input_path, line_number, cmd);
            fclose(in);
            return -1;
        }
    }

    if (ferror(in)) {
        vm_errorf("ERR_IO_4", "readable input file stream", "'%s' (%s)", input_path, strerror(errno));
        fclose(in);
        return -1;
    }

    fclose(in);
    return 0;
}

static int translate_single_file(const char *input_path, const char *output_path) {
    FILE *out;
    CodeWriter writer;
    int result;

    out = fopen(output_path, "w");
    if (out == NULL) {
        vm_errorf("ERR_IO_1", "writable output file", "'%s' (%s)", output_path, strerror(errno));
        return -1;
    }

    if (code_writer_init(&writer, out, input_path) != 0) {
        vm_error("ERR_TRN_6", "initialized code writer", "initialization failure");
        fclose(out);
        return -1;
    }

    result = translate_file(&writer, input_path);
    if (result == 0 && write_shared_routines(&writer) != 0) {
        vm_error("ERR_TRN_9", "shared VM helper routines", "write failure");
        result = -1;
    }

    if (fclose(out) != 0) {
        vm_errorf("ERR_IO_5", "closable output file", "'%s' (%s)", output_path, strerror(errno));
        return -1;
    }

    return result;
}

static int translate_folder(const char *input_path, const char *output_path) {
    PathList files = {0};
    FILE *out;
    CodeWriter writer;
    size_t i;
    int result = 0;

    if (collect_vm_files(input_path, &files) != 0) {
        return -1;
    }

    if (files.count == 0) {
        vm_errorf("ERR_TRN_8", "folder containing at least one .vm file", "'%s'", input_path);
        path_list_free(&files);
        return -1;
    }

    out = fopen(output_path, "w");
    if (out == NULL) {
        vm_errorf("ERR_IO_1", "writable output file", "'%s' (%s)", output_path, strerror(errno));
        path_list_free(&files);
        return -1;
    }

    if (code_writer_init(&writer, out, NULL) != 0) {
        vm_error("ERR_TRN_6", "initialized code writer", "initialization failure");
        fclose(out);
        path_list_free(&files);
        return -1;
    }

    if (write_bootstrap(&writer) != 0) {
        vm_error("ERR_TRN_7", "bootstrap code emission", "write failure");
        result = -1;
    }

    for (i = 0; result == 0 && i < files.count; i++) {
        result = translate_file(&writer, files.items[i]);
    }

    if (result == 0 && write_shared_routines(&writer) != 0) {
        vm_error("ERR_TRN_9", "shared VM helper routines", "write failure");
        result = -1;
    }

    if (fclose(out) != 0) {
        vm_errorf("ERR_IO_5", "closable output file", "'%s' (%s)", output_path, strerror(errno));
        result = -1;
    }

    path_list_free(&files);
    return result;
}

int main(int argc, char **argv) {
    const char *input_path;
    char output_path[PATH_MAX];
    struct stat input_stat;
    int is_folder;

    if (argc != 2 && argc != 3) {
        vm_errorf("ERR_CLI_0", "1 or 2 user arguments", "%d user arguments", argc - 1);
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    input_path = argv[1];
    if (stat(input_path, &input_stat) != 0) {
        vm_errorf("ERR_CLI_1", "accessible input path", "'%s' (%s)", input_path, strerror(errno));
        return EXIT_FAILURE;
    }

    is_folder = S_ISDIR(input_stat.st_mode);
    if (!is_folder && !S_ISREG(input_stat.st_mode)) {
        vm_errorf("ERR_CLI_2", "regular .vm file or folder", "'%s'", input_path);
        return EXIT_FAILURE;
    }

    if (argc == 3) {
        if (snprintf(output_path, sizeof(output_path), "%s", argv[2]) >= (int)sizeof(output_path)) {
            vm_errorf("ERR_CLI_4", "path shorter than PATH_MAX", "'%s'", argv[2]);
            return EXIT_FAILURE;
        }
    } else if (is_folder) {
        if (default_folder_output(input_path, output_path, sizeof(output_path)) != 0) {
            return EXIT_FAILURE;
        }
    } else if (replace_vm_extension(input_path, output_path, sizeof(output_path)) != 0) {
        return EXIT_FAILURE;
    }

    if (is_folder) {
        return translate_folder(input_path, output_path) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return translate_single_file(input_path, output_path) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
