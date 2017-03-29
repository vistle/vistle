// CubemapReprojector.h

#ifndef _CUBEMAPREPROJECTOR_H_INCLUDED_
#define _CUBEMAPREPROJECTOR_H_INCLUDED_

#include "../export.h"

#include <memory>

class CubemapReprojectorImplementor;

class V_PLUGINUTILEXPORT CubemapReprojector
{
	std::unique_ptr<CubemapReprojectorImplementor> m_Implementor;

public:

	CubemapReprojector();
	~CubemapReprojector();

	void Destroy();
	void Render(unsigned openGLContextID);

	void AdjustDimensionsAndMatrices(unsigned viewIndex,
		unsigned short clientWidth, unsigned short clientHeight,
		unsigned short* serverWidth, unsigned short* serverHeight,
		const double* leftViewMatrix, const double* rightViewMatrix,
		const double* leftProjMatrix, const double* rightProjMatrix,
		double* viewMatrix, double* projMatrix);

	void SetNewServerCamera(int viewIndex,
		const double* model, const double* view, const double* proj);

	void SetClientCamera(const double* model, const double* view, const double* proj);

	void ResizeView(int viewIndex, int w, int h, unsigned depthFormat);

	unsigned char* GetColorBuffer(unsigned viewIndex);
	unsigned char* GetDepthBuffer(unsigned viewIndex);
	void SwapFrame();

	void GetFirstModelMatrix(double* model);
	void GetModelMatrix(double* model, bool* pIsValid);

	void SetClientCameraMovementAllowed(bool isAllowed);

	unsigned GetCountSourceViews() const;
};

#endif