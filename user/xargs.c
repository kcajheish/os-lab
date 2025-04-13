#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

void main(int argc, char *argv[]) {
    char *args[MAXARG];
    char c;
    int i = 0;
    int size = 0;
    char buff[512];
    char *p = buff;
    while (read(0, &c, 1) > 0) {
        if (c == '\n' || c == '\0') {
            *(p) = '\0';
            char *arg = malloc(size+1);
            memmove(arg, buff, size+1);
            args[i] = arg;
            i += 1;
            size = 0;
            p = buff;
            continue;
        }
        *(p) = c;
        p += 1;
        size += 1;
    }

    for (int j = 0; j < i; j++){
        char *cmd = argv[1];
        char *exec_arg[MAXARG];
        int k;
        for (k = 1; k < argc; k++) {
            exec_arg[k-1] = argv[k];
        }
        exec_arg[k-1] = args[j];
        if (fork() == 0) {
            exec(cmd, exec_arg);
        }
        wait(0);
        free(args[j]);
    }

    exit(0);
}
