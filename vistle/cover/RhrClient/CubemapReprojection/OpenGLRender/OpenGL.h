// OpenGLRender/OpenGL.h

#ifndef OPENGLRENDER_OPENGL_H_INCLUDED_
#define OPENGLRENDER_OPENGL_H_INCLUDED_

// GLEW - Extension loading library.
#include <External/glew-1.12.0/include/GL/glew.h>

namespace OpenGLRender
{
	// Initializes GLEW.
	void InitializeGLEW();

	// Creates debug callbacks. InitializeGLEW() has to be called first.
	void InitializeGLDebugging();

	// Raises exception on OpenGL error.
	void CheckGLError();
}

#endif