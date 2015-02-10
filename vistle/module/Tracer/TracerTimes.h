#ifndef TRACERTIMES_H
#define TRACERTIMES_H
#include <boost/chrono.hpp>

namespace times{
extern boost::chrono::high_resolution_clock::time_point celloc_start;
extern boost::chrono::high_resolution_clock::time_point integr_start;
extern boost::chrono::high_resolution_clock::time_point interp_start;
extern boost::chrono::high_resolution_clock::time_point comm_start;
extern boost::chrono::high_resolution_clock::time_point total_start;
extern double celloc_dur;
extern double integr_dur;
extern double interp_dur;
extern double comm_dur;
extern double total_dur;
extern unsigned int no_interp;

double stop(boost::chrono::high_resolution_clock::time_point start);
boost::chrono::high_resolution_clock::time_point start();
void output();
}
#endif
