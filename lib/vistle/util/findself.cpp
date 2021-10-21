#include "findself.h"
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <climits>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#include <dirent.h>
#else
#include <vistle/util/sysdep.h>
#include <direct.h>
#endif
#include <errno.h>

//#define DEBUG

namespace vistle {

static void splitpath(const std::string &value, std::vector<std::string> *components)
{
#ifdef _WIN32
    const char *sep = ";";
#else
    const char *sep = ":";
#endif

    std::string::size_type begin = 0;
    do {
        std::string::size_type end = value.find(sep, begin);

        std::string c;
        if (end != std::string::npos) {
            c = value.substr(begin, end - begin);
            ++end;
        } else {
            c = value.substr(begin);
        }
        begin = end;

        if (!c.empty())
            components->push_back(c);
    } while (begin != std::string::npos);
}

std::string getbindir(int argc, char *argv[])
{
    char *wd = getcwd(NULL, 0);
    if (!wd) {
        std::cerr << "findself.cpp: failed to determine working directory: " << strerror(errno) << std::endl;
        exit(1);
    }
    std::string cwd(wd);
    free(wd);

    // determine complete path to executable
    std::string executable;
#ifdef _WIN32
    char buf[2000];
    DWORD sz = GetModuleFileName(NULL, buf, sizeof(buf));
    if (sz != 0) {
        executable = buf;
    } else {
        std::cerr << "Communicator: getbindir(): GetModuleFileName failed - error: " << GetLastError() << std::endl;
    }
#else
    char buf[PATH_MAX];
#ifdef __APPLE__
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        executable = buf;
    } else
#else
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf));
    if (len != -1) {
        executable = std::string(buf, len);
    } else
#endif
        if (argc >= 1) {

        bool found = false;
        if (!strchr(argv[0], '/')) {
            if (const char *path = getenv("PATH")) {
                std::vector<std::string> components;
                splitpath(path, &components);
                for (size_t i = 0; i < components.size(); ++i) {
                    std::string component = components[i];
                    if (component[0] != '/')
                        component = cwd + "/" + component;

                    DIR *dir = opendir(component.c_str());
                    if (!dir) {
                        std::cerr << "findself.cpp: failed to open directory " << component << ": " << strerror(errno)
                                  << std::endl;
                        continue;
                    }

                    while (struct dirent *ent = readdir(dir)) {
                        if (!strcmp(ent->d_name, argv[0])) {
                            found = true;
                            break;
                        }
                    }

                    if (found) {
                        executable = component + "/" + argv[0];
                        break;
                    }
                }
            }
        } else if (argv[0][0] != '/') {
            executable = cwd + "/" + argv[0];
        } else {
            executable = argv[0];
        }
    }
#endif

    // guess vistle bin directory
    if (!executable.empty()) {
        std::string dir = executable;
#ifdef _WIN32
        std::string::size_type idx = dir.find_last_of("\\/");
#else
#if defined(__APPLE__)
        {
            std::string::size_type idx = dir.rfind('/');
            if (idx != std::string::npos) {
                std::string macosdir = executable.substr(0, idx);
                const std::string bundle = ".app/Contents/MacOS";
                const auto len = bundle.length();
                if (macosdir.length() >= len) {
                    std::string tail = macosdir.substr(macosdir.length() - len);
                    if (tail == bundle) {
                        dir = macosdir.substr(0, macosdir.length() - len);
#ifdef DEBUG
                        std::cerr << "findself.cpp: stripped .app bundle dir: " << dir << std::endl;
#endif
                    }
                }
            }
        }
#endif
        std::string::size_type idx = dir.rfind('/');
#endif
        if (idx == std::string::npos) {
            dir = cwd;
        } else {
            dir = executable.substr(0, idx);
        }

#ifdef DEBUG
        std::cerr << "findself.cpp: vistle bin directory determined to be " << dir << std::endl;
#endif
        return dir;
    }

    return std::string();
}

} // namespace vistle
