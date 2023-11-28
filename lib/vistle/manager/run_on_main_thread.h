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

void V_MANAGEREXPORT set_manager_in_cover_plugin();
bool V_MANAGEREXPORT manager_in_cover_plugin();
void V_MANAGEREXPORT wait_for_cover_plugin();
void V_MANAGEREXPORT mark_cover_plugin_done();

#ifdef COVER_ON_MAINTHREAD
void V_MANAGEREXPORT run_on_main_thread(std::function<void()> &func);
#endif
#endif

#endif
