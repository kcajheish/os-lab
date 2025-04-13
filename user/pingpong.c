#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int parent_to_child[2];
  pipe(parent_to_child);
  int child_to_parent[2];
  pipe(child_to_parent);
  char buffer[1];
  // if current process is child
  if (fork() == 0) {
    // close fd on parent
    close(child_to_parent[0]);
    close(parent_to_child[1]);

    // wait for child byte and close it afterward.
    read(parent_to_child[0], buffer, sizeof(buffer));
    close(parent_to_child[0]);

    // write messages to the console output(default to file descriptor 1)
    printf("%d: received ping\n", getpid());

    // send a byte to parent and close child write file descriptor
    write(child_to_parent[1], " ", sizeof(buffer));
    close(child_to_parent[1]);

    // if current process is parent
  } else {
    // close read to parent pipe
    // close write to child pipe
    close(parent_to_child[0]);
    close(child_to_parent[1]);

    // write a byte to parent pipe and close write to parent pipe
    write(parent_to_child[1], " ", 1);
    close(parent_to_child[1]);

    // read from child pipe and close it
    read(child_to_parent[0], buffer, 1);
    close(child_to_parent[0]);

    // write message to standard output
    printf("%d: received pong\n",  getpid());

  }
  exit(0);
}
