#ifndef VISTLE_MANAGER_H
#define VISTLE_MANAGER_H

#include "export.h"

#ifdef MODULE_THREAD
#define COVER_ON_MAINTHREAD
#ifdef __APPLE__
#ifndef COVER_ON_MAINTHREAD
#define COVER_ON_MAINTHREAD
#endif
#endif

#ifdef COVER_ON_MAINTHREAD
static std::mutex main_thread_mutex;
static std::condition_variable main_thread_cv;
static std::deque<std::function<void()>> main_func;
static bool main_done = false;

void run_on_main_thread(std::function<void()>& func) {

    {
        std::unique_lock<std::mutex> lock(main_thread_mutex);
        main_func.emplace_back(func);
    }
    main_thread_cv.notify_all();
    std::unique_lock<std::mutex> lock(main_thread_mutex);
    main_thread_cv.wait(lock, [] { return main_done || main_func.empty(); });
}
#endif
#endif

#endif
