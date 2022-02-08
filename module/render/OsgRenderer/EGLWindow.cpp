#include "EGLWindow.h"
#include <EGL/egl.h>
#include <iostream>

EglGraphicsWindow::EglGraphicsWindow(int width, int height): osgViewer::GraphicsWindowEmbedded(0, 0, width, height)
{
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY) {
        return;
    }
    if (!eglInitialize(m_display, &m_major, &m_minor)) {
        return;
    }

    const EGLint config[] = {EGL_RENDERABLE_TYPE,
                             EGL_OPENGL_BIT,
                             EGL_SURFACE_TYPE,
                             EGL_PBUFFER_BIT,
                             EGL_RED_SIZE,
                             8,
                             EGL_GREEN_SIZE,
                             8,
                             EGL_BLUE_SIZE,
                             8,
                             EGL_DEPTH_SIZE,
                             24,
                             EGL_NONE};

    std::vector<EGLConfig> m_configs;

    EGLint numConfigs = 0;
    if (eglChooseConfig(m_display, config, nullptr, 1, &numConfigs)) {
        m_configs.resize(numConfigs);
        if (!eglChooseConfig(m_display, config, &m_configs[0], numConfigs, &numConfigs)) {
            std::cerr << "EglGraphicsWindow: finding config failed" << std::endl;
            m_configs.clear();
            return;
        }
    } else {
        std::cerr << "EglGraphicsWindow: no matching config" << std::endl;
        return;
    }

    m_context = eglCreateContext(m_display, m_configs.front(), EGL_NO_CONTEXT, nullptr);
    if (m_context == EGL_NO_CONTEXT) {
        return;
    }

    const EGLint attribs[] = {
        EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE,
    };
    m_surface = eglCreatePbufferSurface(m_display, m_configs.front(), attribs);
    if (m_surface == EGL_NO_SURFACE) {
        return;
    }

    if (!eglBindAPI(EGL_OPENGL_API)) {
        return;
    }

    eglMakeCurrent(m_display, m_surface, m_surface, m_context);

    m_valid = true;

    setState(new osg::State);
    getState()->setGraphicsContext(this);

    getState()->setContextID(osg::GraphicsContext::createNewContextID());
}

EglGraphicsWindow::~EglGraphicsWindow()
{
    eglTerminate(m_display);
}

bool EglGraphicsWindow::valid() const
{
    return m_valid;
}

bool EglGraphicsWindow::realizeImplementation()
{
    if (!valid())
        return false;
    eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    return true;
}

bool EglGraphicsWindow::makeCurrentImplementation()
{
    eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    return true;
}

bool EglGraphicsWindow::releaseContextImplementation()
{
    return true;
}

bool EglGraphicsWindow::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    return true;
}

void EglGraphicsWindow::swapBuffersImplementation()
{}
