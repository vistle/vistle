// OculusVR/SimpleVR_GL.h

#ifndef _OCULUSVR_SIMPLEVR_GL_H_INCLUDED_
#define _OCULUSVR_SIMPLEVR_GL_H_INCLUDED_

#include <OculusVR/SimpleVR.h>

namespace OculusVR
{
	class VR_GL_HelperImplementor;

	struct VR_GL_InitData : public VR_InitData
	{
		unsigned InputEyeFBOs[2];
		unsigned OutputFBO;

		VR_GL_InitData();
	};

	class VR_GL_Helper : public VR_Helper
	{	
	public:

		VR_GL_Helper();
		~VR_GL_Helper();
	};
}

#endif