#include "manager.h"

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>
#include "run_on_main_thread.h"
#include <vistle/util/directory.h>
#include <vistle/core/objectmeta.h>
#include <vistle/core/object.h>
#include "executor.h"
#include "communicator.h"
#include <vistle/util/hostname.h>
#include <vistle/util/threadname.h>
#include <vistle/control/hub.h>
#include <boost/mpi.hpp>

#ifdef MODULE_THREAD
#include <thread>
#endif

#ifdef COVER_ON_MAINTHREAD
#include <functional>
#include <deque>
#include <condition_variable>
#include <mutex>

static std::mutex main_thread_mutex;
static std::condition_variable main_thread_cv;
static std::deque<std::function<void()>> main_func;
static bool main_done = false;

void run_on_main_thread(std::function<void()> &func)
{
    {
        std::unique_lock<std::mutex> lock(main_thread_mutex);
        main_func.emplace_back(func);
    }
    main_thread_cv.notify_all();
    std::unique_lock<std::mutex> lock(main_thread_mutex);
    main_thread_cv.wait(lock, [] { return main_done || main_func.empty(); });
}

#if defined(HAVE_QT) && defined(MODULE_THREAD)
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#endif
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <xcb/xcb.h>
#ifdef HAVE_X11_ICE
#include <X11/ICE/ICElib.h>

namespace {
void iceIOErrorHandler(IceConn conn)
{
    (void)conn;
    std::cerr << "Vistle: ignoring ICE IO error" << std::endl;
}
} // namespace
#endif
#endif


using namespace vistle;
namespace dir = vistle::directory;

class Vistle: public Executor {
public:
    Vistle(int argc, char *argv[], boost::mpi::communicator comm): Executor(argc, argv, comm) {}
    bool config(int argc, char *argv[]) override
    {
        if (const char *VISTLE_ROOT = getenv("VISTLE_ROOT")) {
            if (const char *VISTLE_BUILDTYPE = getenv("VISTLE_BUILDTYPE")) {
                setVistleRoot(VISTLE_ROOT, VISTLE_BUILDTYPE);
                return true;
            }
        }
        return false;
    }
};

bool VistleManager::run(int argc, char *argv[])
{
    int rank = -1;
    int flag = 0;
    MPI_Initialized(&flag);
    if (flag) {
#ifdef MODULE_THREAD
        int prov = MPI_THREAD_SINGLE;
        MPI_Query_thread(&prov);
        if (prov != MPI_THREAD_MULTIPLE) {
            std::cerr << "VistleManager: MPI_THREAD_MULTIPLE not provided" << std::endl;
            rank = 0;
            return false;
        }
#endif
        m_comm = boost::mpi::communicator(MPI_COMM_WORLD, boost::mpi::comm_duplicate);
        MPI_Comm_rank(m_comm, &rank);
    } else {
        //initialize mpi with single rank on local host?
        std::cerr << "VistleManager: MPI not initialized" << std::endl;
        rank = 0;
        return false;
    }

    std::cerr << "VistleManager: my rank is " << rank << std::endl;

    bool fromVistle = false;
    if (argc > 1) {
        std::string arg1(argv[1]);
        fromVistle = arg1 == "-from-vistle";
    }

    std::vector<std::string> args;
    args.push_back(argv[0]);
    if (fromVistle) {
        // skip -from-vistle
        for (int c = 2; c < argc; ++c) {
            args.push_back(argv[c]);
        }
    }

#ifdef MODULE_THREAD
    std::string rank0;
    unsigned short hubPort = 0, hubDataPort = 0;
    std::unique_ptr<std::thread> hubthread;
    if (!fromVistle && rank == 0) {
        std::cerr << "not called from vistle, creating hub in a thread" << std::endl;
        auto hub = new Hub(true);
        hub->init(argc, argv);
        hubPort = hub->port();
        hubDataPort = hub->dataPort();
        rank0 = hostname();

        hubthread.reset(new std::thread([hub]() {
            setThreadName("vistle:hub");
            hub->run();
            std::cerr << "Hub exited..." << std::endl;
            delete hub;
        }));
    }

    if (!fromVistle) {
        boost::mpi::broadcast(m_comm, rank0, 0);
        boost::mpi::broadcast(m_comm, hubPort, 0);
        boost::mpi::broadcast(m_comm, hubDataPort, 0);

        args.push_back(rank0);
        args.push_back(std::to_string(hubPort));
        args.push_back(std::to_string(hubDataPort));
    }
#else
    if (!fromVistle) {
        std::cerr << "should be called from vistle, expecting 1st argument to be -from-vistle" << std::endl;
        exit(1);
    }
#endif

#ifdef COVER_ON_MAINTHREAD
#if defined(HAVE_QT) && defined(MODULE_THREAD)
    if (!qApp) {
        std::cerr << "early creation of QApplication object" << std::endl;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        if (xcb_connection_t *xconn = xcb_connect(nullptr, nullptr)) {
            if (!xcb_connection_has_error(xconn)) {
                std::cerr << "X11 connection!" << std::endl;
#ifdef HAVE_X11_ICE
                IceSetIOErrorHandler(&iceIOErrorHandler);
#endif
                (void)new QApplication(argc, argv);
            }
            xcb_disconnect(xconn);
        }
#else
        (void)new QApplication(argc, argv);
#endif
        if (qApp) {
            qApp->setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
            QIcon icon(":/icons/vistle.png");
            qApp->setWindowIcon(icon);
        }
    }
#endif
    auto t = std::thread([args, this]() {
        setThreadName("vistle:main2");
#endif
        try {
            vistle::registerTypes();
            std::vector<char *> argv;
            for (auto &a: args) {
                argv.push_back(const_cast<char *>(a.data()));
            }
            executer = new Vistle(argv.size(), argv.data(), m_comm);
            std::cerr << "created Vistle executor" << std::endl;
            executer->run();
        } catch (vistle::exception &e) {
            std::cerr << "fatal exception: " << e.what() << std::endl << e.where() << std::endl;
            exit(1);
        } catch (std::exception &e) {
            std::cerr << "fatal exception: " << e.what() << std::endl;
            exit(1);
        }

#ifdef COVER_ON_MAINTHREAD
        std::unique_lock<std::mutex> lock(main_thread_mutex);
        main_done = true;
        main_thread_cv.notify_all();
    });
    for (;;) {
        std::unique_lock<std::mutex> lock(main_thread_mutex);
        main_thread_cv.wait(lock, [] { return main_done || !main_func.empty(); });
        if (main_done) {
            //main_thread_cv.notify_all();
            break;
        }

        std::function<void()> f;
        if (!main_func.empty()) {
            std::cerr << "executing on main thread..." << std::endl;
            f = main_func.front();
            if (f)
                f();
            main_func.pop_front();
        }
        lock.unlock();
        main_thread_cv.notify_all();
    }
#endif

#ifdef MODULE_THREAD
    if (t.joinable()) {
        t.join();
    }

    if (hubthread && hubthread->joinable()) {
        hubthread->join();
    }

    std::cerr << "VistleManager: threads done" << std::endl;
#endif
    return true;
}

vistle::VistleManager::~VistleManager()
{
    delete executer;
}
