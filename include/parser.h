#ifndef PARSER_H
#define PARSER_H
/*
 * Parses one VM source line in place.
 *
 * Removes comments and surrounding whitespace from line, then writes up to
 * three fields into cmd, arg1, and arg2. Blank or comment-only lines leave
 * cmd and arg1 empty and arg2 set to 0.
 */
void parse(char *line, char *cmd, char *arg1, int *arg2);

#endif
