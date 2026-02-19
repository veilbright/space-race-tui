#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "control_center.h"
#include "spaceship.h"

/*
 * Create pipes for every pair in pipe_fds
 * size is the number of pairs in pipe_fds
 */
void create_pipes(int (*pipe_fds)[2], const int size) {
  for (int i = 0; i < size; i++) {
    if (pipe(pipe_fds[i]) == -1) {
      perror("setup_pipes failed");
      exit(EXIT_FAILURE);
    }
  }
}

/*
 * Sets pids to the children's pids
 * Forks size times
 * Returns the child's index (0-size) to the children,
 * and -1 to the parent process
 */
int fork_children(pid_t* pids, const int size) {
  for (int i = 0; i < size; ++i) {
    pids[i] = fork();

    // Error
    if (pids[i] == -1) {
      perror("fork_children failed");
      exit(EXIT_FAILURE);
    }

    // Child
    else if (pids[i] == 0) {
      return i;
    }
  }
  return -1;
}

/*
 * All pipes not at index in pipe_fds will be closed
 * size is the number of pairs in pipe_fds
 * Leaves write end of pipe at index open
 */
void setup_child_pipes(const int (*pipe_fds)[2], const int size,
                       const int index) {
  for (int i = 0; i < size; ++i) {
    // Close the read end of this child's pipe
    if (i == index) {
      if (close(pipe_fds[i][0]) == -1) {
        perror("setup_child_pipes failed");
        exit(EXIT_FAILURE);
      }
    }
    // Close both ends of every other pipe
    else {
      if (close(pipe_fds[i][0]) == -1 || close(pipe_fds[i][1]) == -1) {
        perror("setup_child_pipes failed");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/*
 * Close the write end of the child's pipe
 */
void close_child_pipes(const int (*pipe_fds)[2], const int index) {
  if (close(pipe_fds[index][1]) == -1) {
    perror("close_parent_pipes failed");
    exit(EXIT_FAILURE);
  }
}

/*
 * Sets up every pipe in pipe_fds to read from
 */
void setup_parent_pipes(const int (*pipe_fds)[2], const int size) {
  // Close write end of every pipe
  for (int i = 0; i < size; ++i) {
    if (close(pipe_fds[i][1]) == -1) {
      perror("setup_parent_pipes failed");
      exit(EXIT_FAILURE);
    }
  }
}

/*
 * Close any pipes that remained open after setup_parent_pipes (all read ends)
 */
void close_parent_pipes(const int (*pipe_fds)[2], const int size) {
  for (int i = 0; i < size; ++i) {
    if (close(pipe_fds[i][0]) == -1) {
      perror("close_parent_pipes failed");
      exit(EXIT_FAILURE);
    }
  }
}

void wait_for_children(const pid_t child_pids[], const int size) {
  int wstatus;
  for (int i = 0; i < size; ++i) {
    if (waitpid(child_pids[i], &wstatus, 0) == -1) {
      perror("wait_for_children failed");
      exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char* argv[]) {
  const int N = 5;  // Number of spaceships to spawn

  const int GOAL_DISTANCE = 1000;  // Amount of distance spaceships must travel
  const int DELAY = 1;             // Seconds to sleep after updates

  int pipe_fds[N][2];   // Pipe file descriptors; 1 pair per spaceship
  pid_t child_pids[N];  // Spaceship pids

  // Child variables
  int child_index;  // Where children will store their pipe index for pipe_fds
  struct spaceship_stats spaceship = {10, 50, 300, "\0"};

  create_pipes(pipe_fds, N);

  // Child (spaceship)
  if ((child_index = fork_children(child_pids, N)) >= 0) {
    setup_child_pipes(pipe_fds, N, child_index);
    setup_spaceship(&spaceship, pipe_fds[child_index]);
    race_spaceship(&spaceship, GOAL_DISTANCE, DELAY);
    close_child_pipes(pipe_fds, child_index);
  }
  // Parent (control center)
  else {
    setup_parent_pipes(pipe_fds, N);
    run_control_center(pipe_fds, N, GOAL_DISTANCE, DELAY);
    close_parent_pipes(pipe_fds, N);
    wait_for_children(child_pids, N);
  }

  return EXIT_SUCCESS;
}
