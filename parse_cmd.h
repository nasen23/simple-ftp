#ifndef __PARSE_CMD_H_
#define __PARSE_CMD_H_

#include <stdlib.h>
#include <string.h>

typedef enum {
USER = 0,
PASS,
QUIT
} Command;

struct CommandList {
    Command cmd;
    char *arg;
};

const char* const cmd_list[] = {
"USER", "PASS", "QUIT"
};
const size_t CMD_LEN = 3;

const char* const argless_cmd_list[] = {
"QUIT"
};
const size_t ARGLESS_CMD_LEN = 1;

int _check_argless_cmd(Command cmd) {
    const char* cmdstr = cmd_list[cmd];
    for (int i = 0; i < ARGLESS_CMD_LEN; ++i) {
        if ( !strcmp(cmdstr, argless_cmd_list[i]) ) {
            return 1;
        }
    }

    return 0;
}

struct CommandList* parse_string(char *full_command) {
    struct CommandList* cmdlst = (struct CommandList*)malloc(sizeof(struct CommandList));
    char *cmd, *arg;
    int i, argless = 0;

    // note that source string is changed after strsep
    while ( full_command && *(cmd = strsep(&full_command, " ")) != '\0' );
    for (i = 0; i < CMD_LEN; ++i) {
        if ( !strcmp(cmd_list[i], cmd) ) {
            cmdlst->cmd = (Command) i;
            break;
        }
    }
    // no matching commands
    if (i == CMD_LEN) {
        return NULL;
    }

    // check if command is argless(require no arg)
    if (_check_argless_cmd(cmdlst->cmd)) {
        cmdlst->arg = NULL;
        return cmdlst;
    }

    while ( full_command && arg && *(arg = strsep(&full_command, " ")) != '\0' );
    if (arg) {
        cmdlst->arg = (char*)malloc(sizeof(char) * strlen(arg));
        strcpy(cmdlst->arg, arg);
    } else {
        cmdlst->arg = NULL;
    }

    return cmdlst;
}


#endif // __PARSE_CMD_H_
