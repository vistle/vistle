#include "run_on_main_thread.h"

#ifdef COVER_ON_MAINTHREAD


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
