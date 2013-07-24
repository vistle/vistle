#include "mainwindow.h"
#include <QApplication>

/*
class UiRunner {

    public:
        UiRunner(UserInterface &ui): m_ui(ui) {}

        void cancel() {
            boost::unique_lock<boost::mutex> lock(m_mutex);
            m_done = true;
        }

        void operator()() {
            while(m_ui.dispatch()) {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                if (m_done)
                    break;
                usleep(10000);
            }
        }

    private:
        UserInterface &m_ui;
        bool m_done = false;
        boost::mutex m_mutex;
};

class StatePrinter: public StateObserver {

    public:
        StatePrinter(std::ostream &out): m_out(out) {}
        void newModule(int moduleId, const std::string &moduleName) {
            m_out << "   module " << moduleName << " started: " << moduleId << std::endl;
        }

        void deleteModule(int moduleId) {
            m_out << "   module deleted: " << moduleId << std::endl;
        }

        void moduleStateChanged(int moduleId, int stateBits) {
            m_out << "   module state change: " << moduleId << " (";
            if (stateBits & StateObserver::Initialized) m_out << "I";
            if (stateBits & StateObserver::Killed) m_out << "K";
            if (stateBits & StateObserver::Busy) m_out << "B";
            m_out << ")" << std::endl;
        }

        void newParameter(int moduleId, const std::string &parameterName) {
            m_out << "   new parameter: " << moduleId << ":" << parameterName << std::endl;
        }

        void parameterValueChanged(int moduleId, const std::string &parameterName) {
            m_out << "   parameter value changed: " << moduleId << ":" << parameterName << std::endl;
        }

        void newPort(int moduleId, const std::string &portName) {
            m_out << "   new port: " << moduleId << ":" << portName << std::endl;
        }

        void newConnection(int fromId, const std::string &fromName,
                           int toId, const std::string &toName) {
            m_out << "   new connection: "
                  << fromId << ":" << fromName << " -> "
                  << toId << ":" << toName << std::endl;
        }

        void deleteConnection(int fromId, const std::string &fromName,
                              int toId, const std::string &toName) {
            m_out << "   connection removed: "
                  << fromId << ":" << fromName << " -> "
                  << toId << ":" << toName << std::endl;
        }

    private:
        std::ostream &m_out;

};

*/

int main(int argc, char *argv[])
{
/*
    try {
        std::string host = "localhost";
        unsigned short port = 8193;

        if (argc > 1)
            host = argv[1];
        if (argc > 2)
            host = atoi(argv[2]);

        UserInterface ui(host, port);
        // is the stateprinter registered correctly here?
        ui.registerObserver(new StatePrinter(std::cout));
        UiRunner runner(ui);
        boost::thread runnerThread(boost::ref(runner));

        runner.cancel();
        runnerThread.join();

        QApplication a(argc, argv);
        MainWindow w;
        w.show();

        return a.exec();
    } catch(std::exception &ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "unknown exception" << std::endl;
        return 1;
    }
*/
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
