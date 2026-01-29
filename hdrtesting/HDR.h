#ifndef HDR_H
#define HDR_H
#include <vector>
#ifdef HDR_EXPORTS
#define HDR_API __declspec(dllexport)
#else
#define HDR_API __declspec(dllimport)
#endif

extern "C" {
	// Function to initialize the HDR processing library
	void HDR_API UploadToGL(float* linearRGB, int width, int height, unsigned char* outputBGRA);
	// Function to process an image for HDR
	void HDR_API InitFullscreenQuad();
	// Function to release resources used by the HDR processing library
	void HDR_API RenderFullscreenQuad();
}

#endif