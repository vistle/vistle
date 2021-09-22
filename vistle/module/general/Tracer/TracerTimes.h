#ifndef TRACERTIMES_H
#define TRACERTIMES_H
#include <chrono>

namespace times {
extern std::chrono::high_resolution_clock::time_point celloc_start;
extern std::chrono::high_resolution_clock::time_point integr_start;
extern std::chrono::high_resolution_clock::time_point interp_start;
extern std::chrono::high_resolution_clock::time_point comm_start;
extern std::chrono::high_resolution_clock::time_point total_start;
extern double celloc_dur;
extern double integr_dur;
extern double interp_dur;
extern double comm_dur;
extern double total_dur;
extern unsigned int no_interp;

double stop(std::chrono::high_resolution_clock::time_point start);
std::chrono::high_resolution_clock::time_point start();
void output();
} // namespace times
#endif
