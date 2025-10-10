#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <assert.h>
#include <string.h>

int main (int argc, char *argv[]) {
  int fds[2];
  int err = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
  assert(err == 0);

  pid_t pid = fork();
  assert(pid >= 0);

  if (pid == 0) {

    close(fds[0]);

    err = dup2(fds[1], 3);
    assert(err == 3);

    close(fds[1]);

    char *args[] = {
      "./build/bin/bare",
      "examples/socket-app.js",
      NULL
    };

    execv(args[0], args);

    perror("execv failed");
    exit(1);

  } else {

    close(fds[1]);

    printf("Launcher: Sending PING to the Bare app...\n");

    err = write(fds[0], "PING\n", 5);
    assert(err == 5);

    char response[16];
    err = read(fds[0], response, sizeof(response) - 1);
    assert(err > 0);
    response[err] = '\0';

    printf("Launcher: Received response: %s", response);

    assert(strcmp(response, "PONG\n") == 0);

    printf("Launcher: Success! The Bare app responded correctly.\n");

    close(fds[0]);
    waitpid(pid, NULL, 0);
  }

  return 0;
}
