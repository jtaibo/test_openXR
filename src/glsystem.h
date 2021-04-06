#pragma once

#include <windows.h>
#include <GL/gl.h>
#include <openxr/openxr.h>
#include <vector>
#include <string>

class GLSystem {

public:
    
    /// Constructor
    GLSystem();

    void initializeDevice(XrInstance instance, XrSystemId systemId, int width, int height);

    inline HDC getHDC() { return _hDC; }
    inline HGLRC getHGLRC() { return _hGLRC; }

    void initGLStuff();

    std::string textureInternalFormatToString(uint32_t fmt);
    int64_t getFormat(const std::vector<int64_t> &supported_swapchain_formats);

    void renderToTexture(uint32_t tex);

private:

    HDC _hDC;
    HGLRC _hGLRC;

    GLsizei _width;
    GLsizei _height;
    GLuint _swapchainFramebuffer;
    uint32_t _depthTexture;

};
