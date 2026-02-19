#ifndef SPACESHIP
#define SPACESHIP

#include <stdbool.h>

// Permanent spaceship info
struct spaceship_stats {
  int min_distance;
  int max_distance;
  int max_fuel;
  char id[9];  // At most an 8-character ID with a terminating null byte
};

// Dynamic spaceship info
struct spaceship_status {
  int distance_travelled;
  int current_fuel;
  bool is_winner;
};

/*
 * Sets up the process to be a spaceship
 * Closes the read end of the pipe, and sets write end to stdout
 * Sets a random seed
 */
extern void setup_spaceship(struct spaceship_stats* spaceship, const int pipefd[2]);

/*
 * Races a spaceship to the defined goal_distance
 * Every delay seconds, the spaceship will randomly travel between its min and
 * max travel distance if it has enough fuel. (1 fuel = 1 distance)
 * Updates will be written to stdout
 */
extern void race_spaceship(const struct spaceship_stats* spaceship,
                           int goal_distance, int delay);

#endif  // SPACESHIP
