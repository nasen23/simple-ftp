#ifndef __PARSE_CMD_H_
#define __PARSE_CMD_H_

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
USER = 0,
PASS,
PORT,
PASV,
QUIT,
ABOR,
SYST,
TYPE,
MKD,
CWD,
CDUP,
PWD,
LIST
} Command;

const char* const cmd_list[] = {
"USER", "PASS", "PORT", "PASV", "QUIT", "ABOR",
"SYST", "TYPE", "MKD", "CWD", "CDUP", "PWD",
"LIST"
};
const size_t CMD_LEN = 13;

const char* const argless_cmd_list[] = {
"PASV", "QUIT", "SYST", "PWD", "ABOR", "LIST", "CDUP"
};
const size_t ARGLESS_CMD_LEN = 7;

struct CommandList {
    Command cmd;
    char *arg;
};

typedef enum {
EMPTY_COMMAND = 1,
UNKNOWN_COMMAND,
MISSING_ARG,
SYNTAX_ERROR
} ParseError;

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

int parse_addr(char *addr, int *args) {
    char *cur = addr;
    for (int i = 0; i < 5; ++i) {
      while (isdigit(*cur))
        ++cur;
      // *cur is not ',' or cur is on the same pos with addr
      if ( *cur != ',' || cur == addr )
          return SYNTAX_ERROR;
      *cur = 0;
      args[i] = atoi(addr); // this atoi shouldn't fail since addr is ensured
                            // as string of digit
      ++cur;
      addr = cur;
    }

    // deal with the last number seperately
    while ( isdigit(*cur) ) ++cur;
    // *cur could be 0 or any whitespace char, but cur must not equal to addr
    if ( (*cur && !isspace(*cur)) || cur == addr ) return SYNTAX_ERROR;
    *cur = 0;
    args[5] = atoi(addr);

    return 0;
}

#endif // __PARSE_CMD_H_
