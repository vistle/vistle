// vistle
#include <vistle/core/messages.h>
#include <vistle/core/object.h>
#include <vistle/core/placeholder.h>

#include "BlenderRenderer.h"

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>

#include <vistle/util/sysdep.h>

#include <vistle/core/polygons.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/lines.h>

namespace asio = boost::asio;
using namespace vistle;

static const unsigned short blender_port = 50007;

BlenderRenderObject::BlenderRenderObject(int senderId, const std::string &senderPort,
                                         vistle::Object::const_ptr container, vistle::Object::const_ptr geometry,
                                         vistle::Object::const_ptr normals, vistle::Object::const_ptr texture)
: vistle::RenderObject(senderId, senderPort, container, geometry, normals, texture)
{}


BlenderRenderer::BlenderRenderer(const std::string &name, int moduleId, mpi::communicator comm)
: vistle::Renderer(name, moduleId, comm), m_socket(m_ioService)
{
    m_fastestObjectReceivePolicy = message::ObjectReceivePolicy::Distribute;
    setObjectReceivePolicy(m_fastestObjectReceivePolicy);

    setIntParameter("render_mode", MasterOnly);

    if (!tryConnect(false)) {
        sendInfo("please start Blender on port %hu, will try to connect again later", blender_port);
    }
}

bool BlenderRenderer::tryConnect(bool retry)
{
    if (m_connected)
        return true;

    std::string host("localhost");

    boost::system::error_code ec;
    do {
        asio::ip::tcp::resolver resolver(m_ioService);
        asio::ip::tcp::resolver::query query(host, std::to_string(blender_port),
                                             asio::ip::tcp::resolver::query::numeric_service);
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
        if (ec) {
            return false;
            break;
        }
        asio::connect(m_socket, endpoint_iterator, ec);
        if (!ec) {
            m_connected = true;
        } else if (ec == boost::system::errc::connection_refused) {
            std::cerr << "." << std::flush;
            sleep(1);
        } else {
            break;
        }
    } while (retry && !m_connected);

    return m_connected;
}

BlenderRenderer::~BlenderRenderer()
{
    prepareQuit();

    // close connection to blender
    if (m_connected)
        m_socket.close();
}

void BlenderRenderer::prepareQuit()
{
    removeAllObjects();
    //    if (cover)
    //        cover->getObjectsRoot()->removeChild(vistleRoot);
    //    vistleRoot.release();

    Renderer::prepareQuit();
}

void BlenderRenderer::removeObject(std::shared_ptr<vistle::RenderObject> vro)
{
    if (!tryConnect())
        return;

    //--- send data to blender
    std::string operation_type = "REM";
    send(operation_type.c_str(), operation_type.length());
    sendObjectInfo(vro->senderId, vro->senderPort, vro->container);
}

template<typename T>
bool BlenderRenderer::send(T d)
{
    if (!m_connected)
        return false;

    auto buf = asio::buffer(&d, sizeof(d));
    boost::system::error_code ec;
    asio::write(m_socket, buf, ec);

    if (ec) {
        std::cerr << "BlenderRenderer: failed to send " << sizeof(d) << " bytes to Blender: " << ec.message()
                  << std::endl;
        return false;
    }

    return true;
}

template<typename T>
bool BlenderRenderer::send(const T *d, size_t n)
{
    if (!m_connected)
        return false;

    auto buf = asio::buffer(d, n * sizeof(*d));
    boost::system::error_code ec;
    asio::write(m_socket, buf, ec);

    if (ec) {
        std::cerr << "BlenderRenderer: failed to send " << n * sizeof(*d) << " bytes to Blender: " << ec.message()
                  << std::endl;
        return false;
    }

    return true;
}

template<typename T>
bool BlenderRenderer::send(const std::vector<T> &d)
{
    return send(d.data(), d.size());
}

void BlenderRenderer::sendObjectInfo(int senderId, const std::string &senderPort, vistle::Object::const_ptr container)
{
    // get info
    int sender_port_length = senderPort.length();
    std::string obj_id = container->getName();
    int obj_id_length = obj_id.length();
    int time_step = container->getTimestep();

    //send data
    send(senderId);
    send(sender_port_length);
    send(senderPort.c_str(), sender_port_length);
    send(obj_id_length);
    send(obj_id.c_str(), obj_id.length());
    send(time_step);
}

std::shared_ptr<vistle::RenderObject> BlenderRenderer::addObject(int senderId, const std::string &senderPort,
                                                                 vistle::Object::const_ptr container,
                                                                 vistle::Object::const_ptr geometry,
                                                                 vistle::Object::const_ptr normals,
                                                                 vistle::Object::const_ptr texture)
{
    if (!tryConnect())
        return nullptr;

    if (!container)
        return nullptr;

    if (!geometry)
        return nullptr;


    std::cerr << "++++++Info " << container->getName() << " type " << container->getType() << " creator "
              << container->getCreator() << " generation " << container->getGeneration() << " iter "
              << container->getIteration() << " block " << container->getBlock() << " timestep "
              << container->getTimestep() << std::endl;

    //--- send data to blender
    Object::Type type = geometry->getType();
    std::string operation_type = "ADD";

    std::shared_ptr<BlenderRenderObject> ro;

    switch (type) {
    case vistle::Object::POLYGONS:
    case vistle::Object::TRIANGLES:
    case vistle::Object::QUADS: {
        // prepare data
        vistle::Polygons::const_ptr polygons = vistle::Polygons::as(geometry);
        vistle::Triangles::const_ptr triangles = vistle::Triangles::as(geometry);
        vistle::Quads::const_ptr quads = vistle::Quads::as(geometry);
        vistle::Coords::const_ptr coords = vistle::Coords::as(geometry);
        assert(coords);
        int N = 0;
        Index numElements = 0;
        Index numCorners = 0;
        const Index numVertices = coords->getNumVertices();
        if (polygons) {
            numElements = polygons->getNumElements();
            numCorners = polygons->getNumCorners();
        } else if (triangles) {
            N = 3;
            numElements = triangles->getNumElements();
            numCorners = triangles->getNumCorners();
        } else if (quads) {
            N = 4;
            numElements = quads->getNumElements();
            numCorners = quads->getNumCorners();
        }

        char geom_type = 'p'; // info

        int num_el = numElements; // elements
        int *el_array = new int[num_el];
        for (int i = 0; i < num_el; i++) {
            el_array[i] = polygons ? polygons->el()[i] : i * N;
        }

        int num_corn = numCorners; // corners
        if (num_corn == 0) {
            num_corn = numElements * N;
        }
        int *corn_array = new int[num_corn];
        if (numCorners > 0) {
            if (polygons) {
                for (int i = 0; i < num_corn; i++) {
                    corn_array[i] = polygons->cl()[i];
                }
            } else if (triangles) {
                for (int i = 0; i < num_corn; i++) {
                    corn_array[i] = triangles->cl()[i];
                }
            } else if (quads) {
                for (int i = 0; i < num_corn; i++) {
                    corn_array[i] = quads->cl()[i];
                }
            }
        } else {
            for (int i = 0; i < num_corn; i++) {
                corn_array[i] = i;
            }
        }

        int num_vert = numVertices; // vertices
        int vert_arr_size = num_vert * 3;
        float *vert_array = new float[vert_arr_size];
        for (int i = 0; i < vert_arr_size; i++) {
            vert_array[i] = coords->getVertex(i / 3)[i % 3];
        }

        int num_values = 0; // vert_values
        float *values_array;
        bool is_valid_tex = false;
        if (texture) {
            auto tex1D = vistle::Texture1D::as(texture);
            auto x_tex1d = tex1D->x();
            if (!std::isinf(x_tex1d[0])) {
                num_values = numVertices;
                values_array = new float[num_values];
                for (int i = 0; i < num_values; i++) {
                    values_array[i] = x_tex1d[i];
                }
                is_valid_tex = true;
            }
        }

        // send data
        send(operation_type.c_str(), operation_type.length());
        send(geom_type);
        sendObjectInfo(senderId, senderPort, container);
        send(num_el);
        send(el_array, num_el);
        send(num_corn);
        send(corn_array, num_corn);
        send(num_vert);
        send(vert_array, vert_arr_size);
        send(num_values);
        if (is_valid_tex) {
            send(values_array, num_values);
        }

        // finalize
        delete[] el_array;
        delete[] corn_array;
        delete[] vert_array;
        if (is_valid_tex) {
            delete[] values_array;
        }

        ro.reset(new BlenderRenderObject(senderId, senderPort, container, geometry, normals, texture));

        break;
    }

    case vistle::Object::LINES: {
        vistle::Lines::const_ptr polygons = vistle::Lines::as(geometry);

        const Index numElements = polygons->getNumElements();
        const Index numCorners = polygons->getNumCorners();
        const Index numVertices = polygons->getNumVertices();

        // prepare data
        char geom_type = 'l'; // info

        int num_el = numElements; // elements
        int *el_array = new int[num_el];
        for (int i = 0; i < num_el; i++) {
            el_array[i] = polygons->el()[i];
        }

        int num_corn = numCorners; // corners
        int *corn_array = new int[num_corn];
        for (int i = 0; i < num_corn; i++) {
            corn_array[i] = polygons->cl()[i];
        }

        int num_vert = numVertices; // vertices
        int vert_arr_size = num_vert * 3;
        float *vert_array = new float[vert_arr_size];
        for (int i = 0; i < vert_arr_size; i++) {
            vert_array[i] = polygons->getVertex(i / 3)[i % 3];
        }

        int num_values = 0; // vert_values
        float *values_array = nullptr;
        bool is_valid_tex = false;
        /*if (texture) {
                        auto tex1D = vistle::Texture1D::as(texture);
                        auto x_tex1d = tex1D->x();
                        if(!isinf(x_tex1d[0])){
                            num_values = numVertices;
                            values_array = new float[num_values];
                            for(int i = 0; i < num_values; i++) {
                                values_array[i] = x_tex1d[i];
                            }
                            is_valid_tex = true;
                        }
                    }*/

        // send data
        send(operation_type.c_str(), operation_type.length());
        send(geom_type);
        sendObjectInfo(senderId, senderPort, container);
        send(num_el);
        send(el_array, num_el);
        send(num_corn);
        send(corn_array, num_corn);
        send(num_vert);
        send(vert_array, vert_arr_size);
        send(num_values);
        if (is_valid_tex) {
            send(values_array, num_values);
        }

        // finalize
        delete[] el_array;
        delete[] corn_array;
        delete[] vert_array;
        if (is_valid_tex) {
            delete[] values_array;
        }

        ro.reset(new BlenderRenderObject(senderId, senderPort, container, geometry, normals, texture));

        break;
    }

    default:
        break;


        //send(sock , hello , strlen(hello) , 0 );
        //printf("Client message sent\n");
        /*char buffer[1024] = {0};
            int valread = read( sock , buffer, 1024);
            printf("%s\n",buffer );*/
        //////////////////////
    }

    auto objType = geometry->getType();
    if (objType == vistle::Object::PLACEHOLDER) {
        auto ph = vistle::PlaceHolder::as(geometry);
        assert(ph);
        objType = ph->originalType();
    }
    //    std::shared_ptr<PluginRenderObject> pro(new PluginRenderObject(senderId, senderPort,
    //                                                                   container, geometry, normals, texture));
    //
    //    pro->coverRenderObject.reset(new VistleRenderObject(pro));
    //    auto cro = pro->coverRenderObject;
    //    if (VistleGeometryGenerator::isSupported(objType)) {
    //        auto vgr = VistleGeometryGenerator(pro, geometry, normals, texture);
    //        auto species = vgr.species();
    //        if (!species.empty()) {
    //            m_colormaps[species];
    //        }
    //        vgr.setColorMaps(&m_colormaps);
    //        m_delayedObjects.emplace_back(pro, vgr);
    //    }
    //    osg::ref_ptr<osg::Group> parent = getParent(cro.get());
    //    const int t = pro->timestep;
    //    if (t >= 0) {
    //    }
    //
    //    coVRPluginList::instance()->addObject(cro.get(), parent, cro->getGeometry(), cro->getNormals(), cro->getColors(), cro->getTexture());
    //    return pro;
    return ro;
}

bool BlenderRenderer::addColorMap(const std::string &species, Object::const_ptr texture)
{
    if (!tryConnect())
        return false;

    //    auto &cmap = m_colormaps[species];
    //    cmap.setName(species);
    //    cmap.setRange(texture->getMin(), texture->getMax());
    //
    //    cmap.image->setPixelFormat(GL_RGBA);
    //    cmap.image->setImage(texture->getWidth(), 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, &texture->pixels()[0], osg::Image::NO_DELETE);
    //    cmap.image->dirty();
    //
    //    cmap.setBlendWithMaterial(texture->hasAttribute("_blend_with_material"));
    //
    //    std::string plugin = texture->getAttribute("_plugin");
    //    if (!plugin.empty())
    //        cover->addPlugin(plugin.c_str());
    //
    //    int modId = texture->getCreator();
    //    auto it = m_interactorMap.find(modId);
    //    if (it == m_interactorMap.end()) {
    //        std::cerr << rank() << ": no module with id " << modId << " found for colormap for species " << species << std::endl;
    //        return true;
    //    }
    //
    //    auto inter = it->second;
    //    if (!inter) {
    //        std::cerr << rank() << ": no interactor for id " << modId << " found for colormap for species " << species << std::endl;
    //        return true;
    //    }
    //    auto ro = inter->getObject();
    //    if (!ro) {
    //        std::cerr << rank() << ": no renderobject for id " << modId << " found for colormap for species " << species << std::endl;
    //        return true;
    //    }
    //
    //    auto att = texture->getAttribute("_colormap");
    //    if (att.empty()) {
    //        ro->removeAttribute("COLORMAP");
    //    } else {
    //        ro->addAttribute("COLORMAP", att);
    //        std::cerr << rank() << ": adding COLORMAP attribute for " << species << std::endl;
    //    }
    //
    //    coVRPluginList::instance()->removeObject(inter->getObjName(), true);
    //    coVRPluginList::instance()->newInteractor(inter->getObject(), inter);

    return true;
}

bool BlenderRenderer::removeColorMap(const std::string &species)
{
    if (!tryConnect())
        return false;

    //    auto it = m_colormaps.find(species);
    //    if (it == m_colormaps.end())
    //        return false;
    //
    //    auto &cmap = it->second;
    //    cmap.setRange(0.f, 1.f);
    //
    //    cmap.image->setPixelFormat(GL_RGBA);
    //    unsigned char red_green[] = {1,0,0,1, 0,1,0,1};
    //    cmap.image->setImage(2, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, red_green, osg::Image::NO_DELETE);
    //    cmap.image->dirty();
    //    //m_colormaps.erase(species);
    return true;
}

MODULE_MAIN(BlenderRenderer)
