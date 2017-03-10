// OpenGLRender/OpenGL.cpp

#include <OpenGLRender/OpenGL.h>

#include <sstream>

#include <EngineBuildingBlocks/ErrorHandling.h>

namespace OpenGLRender
{
	void CheckGLError()
	{
		auto errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			std::stringstream ss;
			ss << "An OpenGL error has been occured: " << glewGetErrorString(errorCode) << "\n";
			EngineBuildingBlocks::RaiseException(ss);
		}
	}

	void InitializeGLEW()
	{
		glewExperimental = GL_TRUE;
		GLenum initResult = glewInit();
		if (initResult != GLEW_OK)
		{
			throw initResult;
		}

		// GLEW initialization casuses error: flushing it.
		glGetError();
	}

	inline void GetSourceString(GLenum source, std::string& str)
	{
		switch (source)
		{
		case GL_DEBUG_CATEGORY_API_ERROR_AMD:
		case GL_DEBUG_SOURCE_API: str = "API"; break;
		case GL_DEBUG_CATEGORY_APPLICATION_AMD:
		case GL_DEBUG_SOURCE_APPLICATION: str = "Application"; break;
		case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: str = "Window System"; break;
		case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
		case GL_DEBUG_SOURCE_SHADER_COMPILER: str = "Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: str = "Third Party"; break;
		case GL_DEBUG_CATEGORY_OTHER_AMD:
		case GL_DEBUG_SOURCE_OTHER: str = "Other"; break;
		default: str = "Unknown"; break;
		}
	}

	inline void GetTypeString(GLenum type, std::string& str)
	{
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR: str = "Error"; break;
		case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: str = "Deprecated Behavior"; break;
		case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: str = "Undefined Behavior"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB: str = "Portability"; break;
		case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
		case GL_DEBUG_TYPE_PERFORMANCE: str = "Performance"; break;
		case GL_DEBUG_CATEGORY_OTHER_AMD:
		case GL_DEBUG_TYPE_OTHER: str = "Other"; break;
		default: str = "Unknown"; break;
		}
	}

	inline void GetSeverityString(GLenum severity, std::string& str)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH: str = "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: str = "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW: str = "Low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: str = "Notification"; break;
		default: str = "Unknown"; break;
		}
	}

	inline void GetErrorIdString(GLuint id, std::string& str)
	{
		switch (id)
		{
		case GL_INVALID_VALUE: str = "Invalid value"; break;
		case GL_INVALID_ENUM: str = "Invalid enum"; break;
		case GL_INVALID_OPERATION: str = "Invalid operation"; break;
		case GL_STACK_OVERFLOW: str = "Stack overflow"; break;
		case GL_STACK_UNDERFLOW: str = "Stack underflow"; break;
		case GL_OUT_OF_MEMORY: str = "Out of memory"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: str = "Invalid framebuffer operation"; break;
		case GL_CONTEXT_LOST: str = "Context lost"; break;
		case GL_TABLE_TOO_LARGE: str = "Table too large"; break;
		default: std::stringstream ss; ss << "Unknow: " << id; str = ss.str(); break;
		}
	}

	void GLAPIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
		const GLchar* message, void* userParam)
	{
		std::string sourceString, typeString, severityString, errorIdString;

		GetSourceString(source, sourceString);
		GetTypeString(type, typeString);
		GetSeverityString(severity, severityString);
		GetErrorIdString(id, errorIdString);

		std::stringstream ss;
		if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		{
			ss << "An OpenGL notification has been received.";
		}
		else
		{
			ss << "An OpenGL error has been occured.";
		}
		ss << std::endl << std::endl;
		ss << "Source:   " << sourceString << std::endl;
		ss << "Type:     " << typeString << std::endl;
		ss << "Severity: " << severityString << std::endl;
		ss << "ID:       " << errorIdString << std::endl;
		ss << "Message:  " << message << std::endl << std::endl;
		ss << "*********************************************" << std::endl << std::endl;

		if (severity == GL_DEBUG_SEVERITY_HIGH) EngineBuildingBlocks::RaiseException(ss);
		else EngineBuildingBlocks::RaiseWarning(ss);
	}

	void InitializeGLDebugging()
	{
		glDebugMessageCallback((GLDEBUGPROC)DebugCallback, nullptr);
#ifdef GL_ARB_debug_output
		if (glewIsExtensionSupported("GL_ARB_debug_output")) glDebugMessageCallbackARB((GLDEBUGPROCARB)DebugCallback, nullptr);
#endif

		glEnable(GL_DEBUG_OUTPUT);

		// Enable synchronous callback. This ensures that the callback function is called
		// right after an error has occurred. This capability is not defined in the AMD
		// version.
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}
}