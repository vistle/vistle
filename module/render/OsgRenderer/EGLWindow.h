#ifndef EGLWINDOW_H
#define EGLWINDOW_H


#include <osgViewer/GraphicsWindow>

#include <EGL/egl.h>

class EglGraphicsWindow: public osgViewer::GraphicsWindowEmbedded {
public:
    EglGraphicsWindow(int width, int height);
    ~EglGraphicsWindow();

    bool realizeImplementation() override;
    bool makeCurrentImplementation() override;
    bool releaseContextImplementation() override;
    bool setWindowRectangleImplementation(int x, int y, int width, int height) override;
    void swapBuffersImplementation() override;

    bool valid() const override;

protected:
    bool m_valid = false;
    EGLDisplay m_display;
    EGLint m_major, m_minor;
    EGLSurface m_surface;
    EGLContext m_context;
};
#endif
