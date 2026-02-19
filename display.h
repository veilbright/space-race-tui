#ifndef DISPLAY
#define DISPLAY

#include "spaceship.h"

/*
 * Refresh the output using the information from ship_stats and ship_status
 */
extern void update_display(const struct spaceship_stats* ship_stats,
                           const struct spaceship_status* ship_status,
                           const int size, const int goal_distance);

/*
 * Output a leaderboard of finishers
 * Doesn't assume every ship is a finisher; will check ship_status
 * finishing_order should list the winning indices descending. Any invalid index
 * will be ignored.
 */
extern void display_leaderboard(const struct spaceship_stats* ship_stats,
                                const struct spaceship_status* ship_status,
                                const int* finishing_order, const int size);

#endif  // DISPLAY
