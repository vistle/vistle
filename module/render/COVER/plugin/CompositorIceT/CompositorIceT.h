#ifndef COMPOSITOR_ICET_H
#define COMPOSITOR_ICET_H

/**\file
 * \brief CompositorIceT plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013, 2014 HLRS
 *
 * \copyright GPL2+
 */

#include <vector>
#include <IceTMPI.h>

#include <cover/coVRPlugin.h>
#include <PluginUtil/MultiChannelDrawer.h>

namespace vistle {
class ReadBackCuda;
}

struct CompositeCallback;

//! Implement depth compositor for sort-last parallel rendering based on IceT
class CompositorIceT: public opencover::coVRPlugin {
    friend struct CompositeCallback;

public:
    //! constructor
    CompositorIceT();
    //! destructor
    ~CompositorIceT();
    //! called during plug-in initialization
    bool init() override;

    //! called before every frame
    void preFrame() override;
    //! called after all rendering has finished - this is where compositing happens
    void clusterSyncDraw() override;
    //! make common scene bounding sphere available to master
    void expandBoundingSphere(osg::BoundingSphere &bs) override;
    //! MPI size
    int mpiSize() const;
    //! MPI rank
    int mpiRank() const;


private:
    //! state attributes of a view being composited which have to be synchronized across all ranks
    struct ViewData {
        ViewData()
        : model(osg::Matrix::identity()), view(osg::Matrix::identity()), proj(osg::Matrix::identity()), left(false)
        {}

        osg::Matrix model, view, proj;
        bool left;
    };

    //! state attributes of a view being composited which are managed locally
    struct CompositeData {
        CompositeData()
        : cudaColor(NULL)
        , cudaDepth(NULL)
        , buffer(GL_FRONT)
        , width(-1)
        , height(-1)
        , viewport{0, 0, 0, 0}
        , rank(-1)
        , chan(-1)
        , localIdx(-1)
        {}

        IceTContext icetCtx; //!< compositing context
        IceTImage image;
        vistle::ReadBackCuda *cudaColor, *cudaDepth; //!< helpers for CUDA read-back
        GLenum buffer;
        osg::ref_ptr<osg::Camera> camera;
        int width, height;
        int viewport[4]; //!< valid pixel viewport for IceT
        int rank;
        int chan;
        int localIdx;
        std::vector<unsigned char> rgba;
        std::vector<float> depth;
    };
    void screenBounds(int *bounds, const CompositeData &cd, const ViewData &vd, const osg::Vec3 &center, float r);
    void getViewData(ViewData &view, int idx);
    void drawView(int view);
    osg::ref_ptr<osg::GraphicsContext> createGraphicsContext(int view, bool pbuffer,
                                                             osg::ref_ptr<osg::GraphicsContext> sharedGc);

    std::vector<int> m_numViews; //!< no. of views (channels x eyes) to display on each rank
    std::vector<int> m_viewBase; //!< index of first view of each rank
    std::vector<ViewData> m_viewData; //< all views being composited
    std::vector<CompositeData> m_compositeData; //< auxiliary data for all views being composited

    bool m_initialized; //!< initialization completed successfully
    int m_rank, m_size; //!< MPI rank and communicator size
    bool m_useCuda; //!< whether CUDA shall be used for framebuffer read-back
    bool m_sparseReadback; //!< whether local scene bounds shall be used for accelerating read-back and compositing
    bool m_writeDepth; //!< whether depth/z should be taken into account
    IceTCommunicator m_icetComm; //!< IceT communicator
    osg::ref_ptr<opencover::MultiChannelDrawer> m_drawer; //!< component for drawing textures to multiple screens

    //! do the actual composting work for window with index 'win'
    void composite(int view);
    //! check whether the window with index 'win' has been resized since last frame
    void checkResize(int win);
    //! wrapper for pixel read-back (either pure OpenGL or CUDA)
    static bool readpixels(vistle::ReadBackCuda *cuda, GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format,
                           int ps, GLubyte *bits, GLenum buf, GLenum type);
};
#endif
