#include "spaceship.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Generate a random number between (inclusive) min_travel_distance and
 * max_travel_distance
 */
int get_random_progress(const int min_travel_distance,
                        const int max_travel_distance) {
  return (rand() % (max_travel_distance - min_travel_distance + 1)) +
         min_travel_distance;
}

void setup_spaceship(struct spaceship_stats* spaceship, const int pipefd[2]) {
  // Set the spaceship's ID to the process PID
  if (snprintf(spaceship->id, sizeof(spaceship->id), "%d", getpid()) < 0) {
    perror("setup_spaceship snprintf failed");
    exit(EXIT_FAILURE);
  }

  // Duplicate stdout to write end
  if (dup2(pipefd[1], 1) == -1) {
    perror("setup_spaceship dup2 failed");
    exit(EXIT_FAILURE);
  }

  // Set piped output to be line buffered
  if (setvbuf(stdout, NULL, _IOLBF, 0) != 0) {
    perror("setup_spaceship setvbuf failed");
    exit(EXIT_FAILURE);
  }

  // Randomize seed for distances travelled
  srand(getpid());
}

void race_spaceship(const struct spaceship_stats* spaceship,
                    const int goal_distance, const int delay) {
  int current_fuel = spaceship->max_fuel;
  int distance_travelled = 0;
  int random_progress = 0;

  // Send spaceship information to control program before anything else
  if (printf("%d,%d,%d,%s\n", spaceship->min_distance, spaceship->max_distance,
             spaceship->max_fuel, spaceship->id) < 0) {
    perror("race_spaceship failed");
    exit(EXIT_FAILURE);
  }

  while (distance_travelled < goal_distance) {
    random_progress =
        get_random_progress(spaceship->min_distance, spaceship->max_distance);

    // Travel
    if (random_progress <= current_fuel) {
      distance_travelled += random_progress;
      current_fuel -= random_progress;
    }
    // Refuel
    else {
      current_fuel = spaceship->max_fuel;
    }
    if (printf("%d,%d\n", distance_travelled, current_fuel) < 0) {
      perror("race_spaceship failed");
      exit(EXIT_FAILURE);
    }
    sleep(1);
  }
}
