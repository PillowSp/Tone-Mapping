#ifndef HDR_H
#define HDR_H

#ifdef HDR_EXPORTS
#define HDR_API __declspec(dllexport)
#else
#define HDR_API __declspec(dllimport)
#endif

extern "C" {

	void HDR_API UploadToGL(float* linearRGB, int width, int height, unsigned char* outputBGRA, float exposure, float whitePoint);

	void HDR_API CleanupGLFW();

	bool HDR_API InitGLFW();
}

#endif