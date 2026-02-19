#ifndef CONTROL_CENTER
#define CONTROL_CENTER

extern void run_control_center(const int (*pipe_fds)[2], const int size,
                               const int goal_distance, const int delay);

#endif  // CONTROL_CENTER
