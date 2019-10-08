#ifndef __PARSE_CMD_H_
#define __PARSE_CMD_H_

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
USER = 0,
PASS,
QUIT
} Command;

struct CommandList {
    Command cmd;
    char *arg;
};

typedef enum {
EMPTY_COMMAND = 1,
UNKNOWN_COMMAND,
MISSING_ARG
} ParseError;

const char* const cmd_list[] = {
"USER", "PASS", "QUIT"
};
const size_t CMD_LEN = 3;

const char* const argless_cmd_list[] = {
"QUIT"
};
const size_t ARGLESS_CMD_LEN = 1;

void rstrip(char *src) {
    if (!src) return;

    // find end
    char *end = src;
    while (*end != '\0') ++end;

    // reverse find last non-white char
    --end;
    while (isspace(*end)) --end;
    *(end + 1) = '\0';
}

int _check_argless_cmd(Command cmd) {
    const char* cmdstr = cmd_list[cmd];
    for (int i = 0; i < ARGLESS_CMD_LEN; ++i) {
        if ( !strcmp(cmdstr, argless_cmd_list[i]) ) {
            return 1;
        }
    }

    return 0;
}

int parse_string(struct CommandList *cmdlst, char *full_command) {
    char *cmd, *arg, *cur;
    int i, len;

    len = strlen(full_command);
    cur = full_command;
    while ( isspace(*cur) ) ++cur;
    if ( *cur == 0 ) {
        // no non-whitespace character present
        return EMPTY_COMMAND;
    }

    // parse cmd
    cmd = cur;
    while ( *cur && !isspace(*cur) ) ++cur; // stops on the first whitespace encountered
    *cur = 0;

    // find proper cmd in enum
    for (i = 0; i < CMD_LEN; ++i) {
        if ( !strcmp(cmd_list[i], cmd) ) {
            cmdlst->cmd = (Command) i;
            break;
        }
    }
    // no matching command
    if ( i == CMD_LEN ) {
        return UNKNOWN_COMMAND;
    }

    // check if command is argless(require no arg)
    if ( _check_argless_cmd(cmdlst->cmd) ) {
        cmdlst->arg = NULL;
        return 0;
    }

    if ( cur - full_command >= len ) {
        // cursor out of bounds, meaning no argument present
        return MISSING_ARG;
    }

    ++cur;
    while ( isspace(*cur) ) ++cur; // stops on non-whitespace or 0
    if ( *cur == 0 ) {
        return MISSING_ARG;
    }

    arg = cur;
    while ( *cur && !isspace(*cur) ) ++cur; // stops on the first whitespace encountered
    *cur = 0;
    cmdlst->arg = arg;

    return 0;
}


#endif // __PARSE_CMD_H_
