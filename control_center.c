#define _POSIX_C_SOURCE 200809L

#include "control_center.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "spaceship.h"

/*
 * Allocate and create file streams for every read end in pipe_fds
 * Returns an array of these file streams, with indices according to pipe_fds
 * The returned array must be freed
 */
FILE** create_pipe_streams(const int (*pipe_fds)[2], const int size) {
  // Allocate enough file pointers for every pipe
  FILE** pipe_streams = ((FILE**)malloc(size * sizeof(FILE*)));
  if (pipe_streams == NULL) {
    perror("read_progress malloc failed");
    exit(EXIT_FAILURE);
  }

  // Create streams for each pipe
  for (int i = 0; i < size; ++i) {
    pipe_streams[i] = fdopen(pipe_fds[i][0], "r");

    if (pipe_streams[i] == NULL) {
      perror("read_progress fdopen failed");
      exit(EXIT_FAILURE);
    }
  }

  return pipe_streams;
}

/*
 * Returns an array of spaceship_stats according to every pipe_stream
 * The returned array must be freed
 */
struct spaceship_stats* read_spaceship_stats(FILE** pipe_streams,
                                             const int size, char* lineptr) {
  struct spaceship_stats* ship_stats =
      ((struct spaceship_stats*)malloc(sizeof(struct spaceship_stats) * size));

  if (ship_stats == NULL) {
    perror("read_spaceship_stats malloc failed");
    exit(EXIT_FAILURE);
  }

  // Read spaceship stats from pipes and save in array
  size_t lineptr_size = sizeof(lineptr);
  for (int i = 0; i < size; ++i) {
    // Get first line from spaceship pipes
    getline(&lineptr, &lineptr_size, pipe_streams[i]);
    if (ferror(pipe_streams[i])) {
      perror("read_spaceship_stats getline failed");
      exit(EXIT_FAILURE);
    }

    // Parse line and save in array
    sscanf(lineptr, "%d,%d,%d,%s\n", &ship_stats[i].min_distance,
           &ship_stats[i].max_distance, &ship_stats[i].max_fuel,
           ship_stats[i].id);
  }

  return ship_stats;
}

/*
 * Updates an array of spaceship_status with any new information from pipes
 */
void update_spaceship_status(FILE** pipe_streams,
                             struct spaceship_status* ship_status,
                             const int size, const int goal_distance,
                             char* lineptr) {
  size_t lineptr_size = sizeof(lineptr);
  ssize_t line_len;

  // Read spaceship statuses from pipes and save in array
  for (int i = 0; i < size; ++i) {
    // Get first line from spaceship pipes
    line_len = getline(&lineptr, &lineptr_size, pipe_streams[i]);
    if (ferror(pipe_streams[i])) {
      perror("update_spaceship_status failed");
      exit(EXIT_FAILURE);
    }

    // Check if line was read before updating status
    if (line_len > 0) {
      // Parse line and save in array
      sscanf(lineptr, "%d,%d\n", &ship_status[i].distance_travelled,
             &ship_status[i].current_fuel);

      // Update is_winner status
      if (ship_status[i].distance_travelled >= goal_distance) {
        ship_status[i].is_winner = true;
      }
    }
  }
}

/*
 * Returns true if arr contains value
 */
bool array_contains(int value, int* arr, int size) {
  for (int i = 0; i < size; ++i) {
    if (value == arr[i]) {
      return true;
    }
  }
  return false;
}

/*
 * Updates finishing_order with the order of indices which became winners
 * Any index already in finishing_order will be left alone, and tiebreakers will
 * be determined by earlier indices
 */
void update_finishing_order(int* finishing_order,
                            const struct spaceship_status* ship_status,
                            int size) {
  int* new_winners = malloc(sizeof(int) * size);

  if (new_winners == NULL) {
    perror("update_finishing_order failed");
    exit(EXIT_FAILURE);
  }

  // Track next new_winners index
  int new_winners_i = 0;

  // Count of winners that are already in finishing_order
  int old_winners_count = 0;

  // Get new winners to determine proper order, and count old_winners
  for (int i = 0; i < size; ++i) {
    if (ship_status[i].is_winner) {
      // New winners
      if (!array_contains(i, finishing_order, size)) {
        new_winners[new_winners_i] = i;
        ++new_winners_i;
      } else {
        ++old_winners_count;
      }
    }
  }

  // Add new winners to finishing_order
  for (int i = 0; i < new_winners_i; ++i) {
    finishing_order[old_winners_count + i] = new_winners[i];
  }

  free(new_winners);
  new_winners = NULL;
}

/*
 * Returns true if any ship's distance travelled is less than the goal distance
 */
bool any_ship_unfinished(const struct spaceship_status* ship_status,
                         const int size, const int goal_distance) {
  for (int i = 0; i < size; ++i) {
    if (ship_status[i].distance_travelled < goal_distance) {
      return true;
    }
  }
  return false;
}

void run_control_center(const int (*pipe_fds)[2], const int size,
                        const int goal_distance, const int delay) {
  // Buffer to use with getline calls
  char* line_buffer;

  // Streams for pipe_fds read ends
  FILE** pipe_streams;

  // Stats include min_distance, max_distance, max_fuel, and ID
  struct spaceship_stats* ship_stats;
  size_t ship_status_size;

  // Status includes distance_travelled and current_fuel
  struct spaceship_status* ship_status;

  // Track order of finishers
  int* finishing_order;
  int finishing_order_size;

  // Size doesn't really matter, getline will allocate new buffer if too small
  line_buffer = malloc(32);

  if (line_buffer == NULL) {
    perror("run_control_center malloc failed");
    exit(EXIT_FAILURE);
  }

  // Convert pipe file descriptors to streams so processing is easier
  pipe_streams = create_pipe_streams(pipe_fds, size);

  // Ship Stats
  ship_stats = read_spaceship_stats(pipe_streams, size, line_buffer);

  // Ship Status
  ship_status_size = sizeof(struct spaceship_status) * size;
  ship_status = ((struct spaceship_status*)malloc(ship_status_size));

  if (ship_status == NULL) {
    perror("run_control_center malloc failed");
    exit(EXIT_FAILURE);
  }

  memset(ship_status, 0, ship_status_size);  // Zero ship_status

  // Finishing Order
  finishing_order_size = sizeof(int) * size;
  finishing_order = malloc(finishing_order_size);

  if (finishing_order == NULL) {
    perror("run_control_center malloc failed");
    exit(EXIT_FAILURE);
  }

  memset(finishing_order, -1, finishing_order_size);

  // Continuously update output with current ship_status until all ships have
  // travelled the goal distance
  while (any_ship_unfinished(ship_status, size, goal_distance)) {
    for (int i = 0; i < size; ++i) {
      update_spaceship_status(pipe_streams, ship_status, size, goal_distance,
                              line_buffer);
      update_finishing_order(finishing_order, ship_status, size);
      update_display(ship_stats, ship_status, size, goal_distance);
      display_leaderboard(ship_stats, ship_status, finishing_order, size);
    }
    sleep(delay);
  }

  // Freeing
  free(pipe_streams);
  pipe_streams = NULL;

  free(line_buffer);
  line_buffer = NULL;

  free(ship_stats);
  ship_stats = NULL;

  free(ship_status);
  ship_status = NULL;

  free(finishing_order);
  finishing_order = NULL;
}
