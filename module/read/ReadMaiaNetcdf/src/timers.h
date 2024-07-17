#ifndef MAIAVISTIMERS_H_
#define MAIAVISTIMERS_H_

#include "typetraits.h"
#include "timernames.h"
#include <chrono>
#include <vector>

namespace maiapv{

/// \brief Main class for measuring the duration between two function calls
///
/// \author Bjoern Peeters (bjoern) <bjoern.peeters@rwth-aachen.de>
/// \date 2016-01-06
///
/// For a measurement a timer needs to be created to get an unique identifier.
/// After that it can be started and stoped. Functions that are called within a
/// routine can be timed with sub-timers, which are created with  "create" and
/// an additional parameter for the id of the superordinate timer.
///     MInt pTimer, cTimer;
///     pTimer = create("Full name for parent timer");
///     cTimer = create("Full name for child timer", pTimer);
///     ...
///     start(pTimer); stop(pTimer);
///     ...
///     print(MPI_Comm);
///
class Timers {
 public:
  MInt create(const MString& name);
  MInt create(const MString& name, const MInt parentId);
  void start(const MInt timer);
  void stop(const MInt timer);
  void reset();
  void print(const MPI_Comm comm);

 private:
  /// \brief Class for storing the info of a single timer
  ///
  /// A Timer struct is created with a name and an identifier for its
  /// superordinate element. The identifiers of subordinate elements are added
  /// to \a m_children.
  /// The time at the start is stored in \a m_StartPoint(). If the
  /// measurement is stopped the resulting duration is added to \a m_sum.
  struct Timer {
    Timer(const MString& name_, const MInt parentId_)
      : m_name(name_),
        m_parentId(parentId_),
        m_sum(std::chrono::duration<MFloat>::zero()),
        m_startPoint(std::chrono::steady_clock::time_point()),
        m_status(Timer::Stopped) {}
    const MString m_name = "n/a";
    const MInt m_parentId = -1;
    std::vector<MInt> m_children{};
    std::chrono::duration<MFloat> m_sum{};
    std::chrono::steady_clock::time_point m_startPoint{};
    MInt m_status;
    enum { Stopped = 0, Running = 1 };
  };

  int noTimers() { return m_timers.size(); };
  std::vector<MFloat> getDurations();

  // m_times contains all the timer that are defined by the calling routine
  std::vector<Timer> m_timers{};
};

} // namespace maiapv

#endif
