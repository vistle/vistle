#include "timers.h"

#include "mpi.h"
#include "UTIL/functions.h"
#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stack>
#include <string>
#include <tuple>

using namespace std;
using namespace std::chrono;

namespace {
/// Get current time
steady_clock::time_point time() { return steady_clock::now(); }

/// Determines the number of digits in front of the decimal mark for
/// 0<= value <=100
MInt noDigits(const MFloat value) {
  if (value < 10.00) {
    return 1;
  }
  if (value < 100.00) {
    return 2;
  }
  return 3;
}
}

namespace maiapv {

// Creates a new top-level timer, adds it to the global list and stores the id in the
// parameter timer
MInt Timers::create(const MString& name) {
  return create(name, -1);
}

// Creates a new timer an adds its id to the child list of the parent timer
MInt Timers::create(const MString& name, const MInt parentId) {
  m_timers.push_back({name, parentId});
  const MInt timer = m_timers.size() - 1;

  // Add the timer to the list of sub-elements
  if (parentId >= 0) {
    m_timers[parentId].m_children.push_back(timer);
  }

  return timer;
}

void Timers::start(const MInt timer) {
  if (timer > static_cast<MInt>(m_timers.size()) - 1) {
    TERMM(1, "Timers: The specified timer with timerId " + to_string(timer)
                 + " doesn't exist");
  }

  Timer& curTimer = m_timers[timer];
  curTimer.m_startPoint = time();
  curTimer.m_status = Timer::Running;
}

void Timers::stop(const MInt timer) {
  if (timer > static_cast<MInt>(m_timers.size()) - 1) {
    TERMM(1, "Timers: The specified timer doesn't exist");
  }

  Timer& curTimer = m_timers[timer];
  if (curTimer.m_status != Timer::Running) {
    return;
  }

  auto curPoint = time();
  // Add the measured duration to the global runtime
  curTimer.m_sum += duration<MFloat>(curPoint - curTimer.m_startPoint);
  curTimer.m_status = Timer::Stopped;
}

// Removes all timers
void Timers::reset() { m_timers.clear(); }

// Gathers the durations from all processes and prints their statistics
// The value of the share is calculated based on the reference mean
void Timers::print(const MPI_Comm comm) {
  int mpiRank = -1;
  int mpiSize = -1;
  MPI_Comm_rank(comm, &mpiRank);
  MPI_Comm_size(comm, &mpiSize);

  std::vector<MFloat> durLocal(noTimers(), 0.0);
  std::vector<MFloat> durGlobal(noTimers() * mpiSize, 0.0);
  durLocal = getDurations();

  // Gather the durations from all processes
  MPI_Gather(&durLocal.front(), durLocal.size(),
             maia::type_traits<MFloat>::mpiType(), &durGlobal.front(),
             durLocal.size(), maia::type_traits<MFloat>::mpiType(), 0, comm);

  if (mpiRank == 0) {
    // Info on order of statistics
    cerr << setprecision(3) << fixed << setw(30) << left << "name" << setw(2)
         << "|" << right << "share" << setw(20) << "mean" << setw(12) << "min"
         << setw(12) << "max" << setw(12) << "stdDev" << endl
         << std::string(95, '-') << endl;

    // Store id of the timer, the prefix and the mean share of the top-level
    // timer in a tuple
    stack<tuple<MInt, MString, MFloat>> timerStack{};
    timerStack.push(make_tuple(0, "| ", -1.0));

    tuple<MInt, MString, MFloat> curTuple{};
    MInt curTimerId = -1;
    MString curPrefix = "";
    MFloat curRefMean = -1.0;

    // Print the statistics of the timers iteratively with an ident for every
    // sub-level
    while (!timerStack.empty()) {
      // Extract the info from the topmost element and remove it from the stack
      // afterwards
      curTuple = timerStack.top();
      tie(curTimerId, curPrefix, curRefMean) = curTuple;
      timerStack.pop();
      const Timer& curTimer = m_timers[curTimerId];

      // Store all measurements for this specific timer
      std::vector<MFloat> durSingle(mpiSize, 0.0);
      for (auto i = 0; i < mpiSize; ++i) {
        durSingle[i] = durGlobal[curTimerId + i * noTimers()];
      }

      // Calculate the first statistics
      const MFloat durMin = *min_element(durSingle.begin(), durSingle.end());
      const MFloat durMax = *max_element(durSingle.begin(), durSingle.end());
      const MFloat durMean
          = std::accumulate(durSingle.begin(), durSingle.end(), 0.0)
            / durSingle.size();

      // Determine the share in percent of the runtime on the parent runtime
      // A negative refMean indicates a timer without a superordinate element
      const MFloat durShare
          = curRefMean > 0.0 ? (durMean / curRefMean) * 100.0 : 100.0;

      // Calculate the standard deviation
      std::transform(
          durSingle.begin(), durSingle.end(), durSingle.begin(),
          [durMean](MFloat f) { return (f - durMean) * (f - durMean); });
      const MFloat durStdDev
          = sqrt((1.0 / durSingle.size())
                 * std::accumulate(durSingle.begin(), durSingle.end(), 0.0));

      // Print the statistics for the current timer
      cerr << curPrefix;
      cerr << setprecision(2) << fixed << setw(30 - curPrefix.length()) << left
           << curTimer.m_name << curPrefix << right
           << std::string(3 - noDigits(durShare), ' ') << durShare << "%"
           << setw(20 - curPrefix.length()) << right << durMean << setw(2)
           << setprecision(3) << "|" << setw(10) << durMin << setw(2) << "|"
           << setw(10) << durMax << setw(2) << "|" << setw(10)
           << setprecision(6) << durStdDev << setw(2) << "|" << setprecision(2)
           << endl;

      // Add all subordinate elements to the stack
      for (auto rit = curTimer.m_children.rbegin();
           rit != curTimer.m_children.rend();
           ++rit) {
        timerStack.push(make_tuple(*rit, curPrefix + "| ", durMean));
      }
    }
  }
}

// Returns the runtime/duration of all timers
std::vector<MFloat> Timers::getDurations() {

  std::vector<MFloat> durations{};
  for (auto it : m_timers) {
    durations.push_back(it.m_sum.count());
  }

  return durations;
}

} // namespace maiapv
