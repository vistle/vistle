#include "TracerTimes.h"
#include <fstream>
#include <string>
#include <boost/mpi.hpp>
#include <sstream>
#include <iostream>
#include <chrono>


std::chrono::high_resolution_clock::time_point times::celloc_start = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point times::integr_start = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point times::interp_start = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point times::comm_start = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point times::total_start = std::chrono::high_resolution_clock::now();
double times::celloc_dur = 0;
double times::integr_dur = 0;
double times::interp_dur = 0;
double times::comm_dur = 0;
double times::total_dur = 0;
unsigned int times::no_interp = 0;

namespace times {

double stop(std::chrono::high_resolution_clock::time_point start)
{
    return 1e-9 *
           std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start)
               .count();
}
std::chrono::high_resolution_clock::time_point start()
{
    return std::chrono::high_resolution_clock::now();
}
void output()
{
    std::ofstream out;
    boost::mpi::communicator world;
    std::string filename = "/mnt/raid/home/hpcmpuet/vistle/vistle/module/Tracer/times_out.csv";
    if (world.rank() == 0) {
        out.open(filename, std::ios_base::app);
        for (int i = 0; i < world.size(); i++) {
            out << "," << i;
        }
        out << "\n";
        out.close();
    }
    double dur[5] = {celloc_dur, interp_dur, integr_dur, comm_dur, total_dur};
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < world.size(); j++) {
            if (world.rank() == j) {
                out.open(filename, std::ios_base::app);
                if (i < 5) {
                    out << "," << dur[i];
                } else {
                    out << "," << no_interp;
                }
                out.close();
            }

            world.barrier();
        }
        if (world.rank() == 0) {
            out.open(filename, std::ios_base::app);
            out << std::endl;
            out.close();
        }
    }

    out.close();
    return;
}
} // namespace times
