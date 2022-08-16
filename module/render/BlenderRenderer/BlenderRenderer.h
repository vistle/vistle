#ifndef VISTLE_BLENDER_RENDERER_H
#define VISTLE_BLENDER_RENDERER_H

#include <string>

#include <vistle/renderer/renderobject.h>
#include <vistle/renderer/renderer.h>
#include <vistle/core/messages.h>
#include <boost/asio.hpp>

class BlenderRenderer: public vistle::Renderer {
public:
    BlenderRenderer(const std::string &name, int moduleId, mpi::communicator comm);
    ~BlenderRenderer() override;

    bool addColorMap(const std::string &species, vistle::Object::const_ptr texture) override;
    bool removeColorMap(const std::string &species) override;

    std::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
                                                    vistle::Object::const_ptr container,
                                                    vistle::Object::const_ptr geometry,
                                                    vistle::Object::const_ptr normals,
                                                    vistle::Object::const_ptr texture) override;
    void removeObject(std::shared_ptr<vistle::RenderObject> ro) override;

    void prepareQuit() override;

protected:
    bool tryConnect();

    template<typename T>
    bool send(T d);

    template<typename T>
    bool send(const std::vector<T> &d);

    template<typename T>
    bool send(const T *d, size_t n);

    //typedef std::map<std::string, OsgColorMap> ColorMapMap;
    //ColorMapMap m_colormaps;

    std::set<int> m_dataTypeWarnings; // set of unsupported data types for which a warning has already been printed

    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::socket m_socket;
    bool m_connected = false;
};

#endif
