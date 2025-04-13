#include "kernel/types.h"
#include "user/user.h"

int MAX_FILTER = 5;
int MAX_NUM = 35;
int TRUE = 1;
int FALSE = 0;
int LEFT = 1;
int RIGHT = 0;
int BUFFER_SIZE = sizeof(int);

int
main(int argc, char *argv[])
{
  int pip[2];
  pipe(pip);
  int pid = fork();
  if (pid > 0) {
    char out[BUFFER_SIZE];
    close(pip[RIGHT]);
    for (int i = 2; i <= MAX_NUM; i++) {
      memmove(out, &i, BUFFER_SIZE);
      int x;
      memmove(&x, out, BUFFER_SIZE);
      write(pip[LEFT], out, BUFFER_SIZE);
    }
    close(pip[LEFT]);
    wait((int *) 0);
  } else {
    close(pip[LEFT]);
    int prime = 0;
    int num = 0;
    int next_pipe[2] = {-1, -1};
    int pid = 0;
    char in[BUFFER_SIZE];
    char out[BUFFER_SIZE];
    while (read(pip[RIGHT], in, BUFFER_SIZE) > 0) {
      memmove(&num, in, BUFFER_SIZE);
      // printf("r= %d\n", num);
      if (prime == 0) {
        prime = num;
        printf("prime %d\n", prime);
      } else {
        if (num % prime > 0) {
          if (pid == 0) {
            int new_pipe[2];
            pipe(new_pipe);
            pid = fork();
            if (pid == 0) {
              close(pip[RIGHT]);
              pip[0] = new_pipe[0];
              pip[1] = new_pipe[1];
              close(pip[LEFT]);
              prime = 0;
            } else {
              next_pipe[0] = new_pipe[0];
              next_pipe[1] = new_pipe[1];
              close(next_pipe[RIGHT]);
              memmove(out, &num, BUFFER_SIZE);
              write(next_pipe[LEFT], out, sizeof(out));
            }
          } else {
            memmove(out, &num, BUFFER_SIZE);
            write(next_pipe[LEFT], out, sizeof(out));
          }

        }
      }
    }
    close(pip[RIGHT]);
    if (pid > 0) {
      close(next_pipe[LEFT]);
      wait((int *) 0);
    }
  }
  exit(0);
}
