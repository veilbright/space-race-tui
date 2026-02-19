#include "display.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spaceship.h"

const char* CLEAR = "\033[2J\033[1;1H";

const char* PROGRESS_STR = "-";
const int PROGRESS_WIDTH = 50;  // The max width of the race progress bars

const char* FUEL_STR = "■";
const int FUEL_WIDTH = 8;  // The max width of the race progress bars

const char* MEDALS[] = {"🥇", "🥈", "🥉"};
const int MEDALS_SIZE = 3;

/*
 * Call printf and check for an error
 */
void printf_check(const char* restrict format, ...) {
  va_list args;
  va_start(args, format);
  if (vprintf(format, args) < 0) {
    perror("printf_check failed");
    exit(EXIT_FAILURE);
  }
  va_end(args);
}

/*
 * Print a line of s mapped to show the current/max progress as of max_width
 * Always fills the line until max_width
 */
void print_progress(const int current_progress, const int max_progress,
                    const int max_width, const char* s) {
  int width = (current_progress * max_width) / max_progress;
  width = width < max_width ? width : max_width;
  for (int i = 0; i < width; ++i) {
    printf_check("%s", s);
  }
  for (int i = width; i < max_width; ++i) {
    putchar(' ');

    if (feof(stdout)) {
      perror("print_progress failed");
      exit(EXIT_FAILURE);
    }
  }
}

/*
 * Returns true if any ship is a winner
 */
bool any_ship_winner(const struct spaceship_status* ship_status,
                     const int size) {
  for (int i = 0; i < size; ++i) {
    if (ship_status[i].is_winner) {
      return true;
    }
  }
  return false;
}

void update_display(const struct spaceship_stats* ship_stats,
                    const struct spaceship_status* ship_status, const int size,
                    const int goal_distance) {
  printf_check("%s", CLEAR);
  printf_check(
      "\n ╭────────────────────────────────────────────╮\n"
      " │ 🚀 Intergalactic Space Race Leaderboard 🚀 │\n"
      "╭┴────────────────────────────────────────────┴─────────────────────────"
      "──"
      "─────────╮\n");

  // Output progress for each spaceship
  for (int i = 0; i < size; ++i) {
    // The amount of space to leave before the progress to keep everything even
    int extra_width = sizeof(ship_stats[i].id) - strlen(ship_stats[i].id) - 1;

    // Output ID and progress
    printf_check("│  Spaceship %2d %*s(%s) |", i + 1, extra_width, "",
                 ship_stats[i].id);
    print_progress(ship_status[i].distance_travelled, goal_distance,
                   PROGRESS_WIDTH, PROGRESS_STR);

    // Output chequered flag
    if (ship_status[i].is_winner) {
      printf_check("| 🏁 │\n");
    } else {
      printf_check("|    │\n");
    }

    // Output fuel
    printf_check(
        "│                            "
        "⛽ |");
    print_progress(ship_status[i].current_fuel, ship_stats[i].max_fuel,
                   FUEL_WIDTH, FUEL_STR);

    // Output distance
    printf_check("| %-4d %26d ly      │\n", ship_status[i].current_fuel,
                 ship_status[i].distance_travelled);

    // Output bottom line
    if (i == size - 1) {
      if (any_ship_winner(ship_status, size)) {
        // Last box with winner
        printf_check(
            "╰──┬───────────────────┬──────────────────────────────────────────"
            "─────────────────╯\n");
      } else {
        // Last box without winner
        printf_check(
            "╰─────────────────────────────────────────────────────────────────"
            "─────────────────╯\n");
      }
    } else {
      // Any box before the last
      printf_check(
          "├───────────────────────────────────────────────────────────────────"
          "───────────────┤\n");
    }
  }
}

void display_leaderboard(const struct spaceship_stats* ship_stats,
                         const struct spaceship_status* ship_status,
                         const int* finishing_order, const int size) {
  // Don't output anything if there aren't any winners
  if (!any_ship_winner(ship_status, size)) {
    return;
  }

  printf_check(
      "   │ 🎉🚀 Winners 🚀🎉 │\n"
      "   ╰───────────────────╯\n");

  // Index of ship from current finishing order
  int finish_i = 0;

  for (int i = 0; i < size; ++i) {
    finish_i = finishing_order[i];  // index to use for ship_stats, etc.

    if (!ship_status[finish_i].is_winner) {
      continue;
    }

    if (i < MEDALS_SIZE) {
      printf_check("   %s Spaceship %d (%s)\n", MEDALS[i], finish_i + 1,
                   ship_stats[finish_i].id);
    } else {
      printf_check("   %d. Spaceship %d (%s)\n", i + 1, finish_i + 1,
                   ship_stats[finish_i].id);
    }
  }
}
