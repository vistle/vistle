#ifndef VISTLE_MANAGER_H
#define VISTLE_MANAGER_H

#include "export.h"
#include <functional>

#ifdef MODULE_THREAD
#define COVER_ON_MAINTHREAD
#ifdef __APPLE__
#ifndef COVER_ON_MAINTHREAD
#define COVER_ON_MAINTHREAD
#endif
#endif

#ifdef COVER_ON_MAINTHREAD
#include <mutex>
#include <deque>
static  std::mutex main_thread_mutex;
static  std::condition_variable main_thread_cv;
static  std::deque<std::function<void()>> main_func;
static  bool main_done = false;
void V_MAINTHREADEXPORT run_on_main_thread(std::function<void()> &func);
#endif
#endif

#endif
