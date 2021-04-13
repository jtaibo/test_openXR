#include "glsystem.h"

#include "gfxwrapper_opengl.h"
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <iostream>
#include <algorithm>


#define CHK_GL(cmd) \
{ \
    cmd; \
    GLuint glerr = glGetError(); \
    if (glerr != GL_NO_ERROR) {\
        std::cerr << "GL error " << __FILE__ << ":" << __LINE__ << std::endl; \
    }\
}

// Check result of OpenXR API calls (throws an exception in case of failure)
#define CHK_XR(cmd, instance) \
{ \
    XrResult res = cmd; \
    char err_msg[XR_MAX_RESULT_STRING_SIZE]; \
    if (XR_SUCCEEDED(res)) { \
        if (res != XR_SUCCESS) { \
            xrResultToString(instance, res, err_msg); \
            std::cerr << "WARN: " << err_msg << " (" << res << ")" << std::endl; \
        } \
    } \
    else { \
        xrResultToString(instance, res, err_msg); \
        std::cerr << "ERROR: " << err_msg << " (" << res << ") - " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw res; \
    } \
}


/**
 *
 */
GLSystem::GLSystem() :
    _hDC(0),
    _hGLRC(0)
{

}

// Dirty stuff, I know...
ksGpuWindow window{};

/**
 *
 */
void GLSystem::initializeDevice(XrInstance instance, XrSystemId systemId, int width, int height)
{
    _width = width;
    _height = height;

    // Extension function must be loaded by name
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    CHK_XR(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR)), instance);

    XrGraphicsRequirementsOpenGLKHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
    CHK_XR(pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &graphicsRequirements), instance);

    // Initialize the gl extensions. Note we have to open a window.
    ksDriverInstance driverInstance{};
    ksGpuQueueInfo queueInfo{};
    ksGpuSurfaceColorFormat colorFormat{ KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 };
    ksGpuSurfaceDepthFormat depthFormat{ KS_GPU_SURFACE_DEPTH_FORMAT_D24 };
    ksGpuSampleCount sampleCount{ KS_GPU_SAMPLE_COUNT_1 };
    if (!ksGpuWindow_Create(&window, &driverInstance, &queueInfo, 0, colorFormat, depthFormat, sampleCount, 640, 480, false)) {
        throw("Unable to create GL context");
    }

    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    const XrVersion desiredApiVersion = XR_MAKE_VERSION(major, minor, 0);
    if (graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
        throw("Runtime does not support desired Graphics API and/or version");
    }

#ifdef XR_USE_PLATFORM_WIN32
    _hDC = window.context.hDC;
    _hGLRC = window.context.hGLRC;
#endif

    initGLStuff();
}

std::string GLSystem::textureInternalFormatToString(uint32_t fmt)
{
    switch (fmt) {
    case GL_ALPHA4:
        return "GL_ALPHA4";
        break;
    case GL_ALPHA8:
        return "GL_ALPHA8";
        break;
    case GL_ALPHA12:
        return "GL_ALPHA12";
        break;
    case GL_ALPHA16:
        return "GL_ALPHA16";
        break;
    case GL_LUMINANCE4_ALPHA4:
        return "GL_LUMINANCE4_ALPHA4";
        break;
    case GL_LUMINANCE6_ALPHA2:
        return "GL_LUMINANCE6_ALPHA2";
        break;
    case GL_LUMINANCE8_ALPHA8:
        return "GL_LUMINANCE8_ALPHA8";
        break;
    case GL_LUMINANCE12_ALPHA4:
        return "GL_LUMINANCE12_ALPHA4";
        break;
    case GL_LUMINANCE12_ALPHA12:
        return "GL_LUMINANCE12_ALPHA12";
        break;
    case GL_LUMINANCE16_ALPHA16:
        return "GL_LUMINANCE16_ALPHA16";
        break;
    case GL_INTENSITY:
        return "GL_INTENSITY";
        break;
    case GL_INTENSITY4:
        return "GL_INTENSITY4";
        break;
    case GL_INTENSITY8:
        return "GL_INTENSITY8";
        break;
    case GL_INTENSITY12:
        return "GL_INTENSITY12";
        break;
    case GL_INTENSITY16:
        return "GL_INTENSITY16";
        break;
    case GL_R3_G3_B2:
        return "GL_R3_G3_B2";
        break;
    case GL_RGB4:
        return "GL_RGB4";
        break;
    case GL_RGB5:
        return "GL_RGB5";
        break;
    case GL_RGB8:
        return "GL_RGB8";
        break;
    case GL_RGB10:
        return "GL_RGB10";
        break;
    case GL_RGB12:
        return "GL_RGB12";
        break;
    case GL_RGB16:
        return "GL_RGB16";
        break;
    case GL_RGBA2:
        return "GL_RGBA2";
        break;
    case GL_RGBA4:
        return "GL_RGBA4";
        break;
    case GL_RGB5_A1:
        return "GL_RGB5_A1";
        break;
    case GL_RGBA8:
        return "GL_RGBA8";
        break;
    case GL_RGB10_A2:
        return "GL_RGB10_A2";
        break;
    case GL_RGBA12:
        return "GL_RGBA12";
        break;
    case GL_RGBA16:
        return "GL_RGBA16";
        break;
    case GL_RGBA16F:
        return "GL_RGBA16F";
        break;
    case GL_RGB16F:
        return "GL_RGB16F";
        break;
    case GL_SRGB8:
        return "GL_SRGB8";
        break;
    case GL_SRGB8_ALPHA8:
        return "GL_SRGB8_ALPHA8";
        break;
    case GL_DEPTH_COMPONENT16:
        return "GL_DEPTH_COMPONENT16";
        break;
    case GL_DEPTH_COMPONENT24:
        return "GL_DEPTH_COMPONENT24";
        break;
    case GL_DEPTH_COMPONENT32:
        return "GL_DEPTH_COMPONENT32";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

int64_t GLSystem::getFormat(const std::vector<int64_t>& supported_swapchain_formats)
{
    // Select swapchain format

    // List of supported color swapchain formats.
    constexpr int64_t SupportedColorSwapchainFormats[] = {
        GL_RGB10_A2,
        GL_RGBA16F,
        // The two below should only be used as a fallback, as they are linear color formats without enough bits for color
        // depth, thus leading to banding.
        GL_RGBA8,
        GL_RGBA8_SNORM
    };
    auto swapchainFormatIt =
    std::find_first_of(supported_swapchain_formats.begin(), supported_swapchain_formats.end(), std::begin(SupportedColorSwapchainFormats),
        std::end(SupportedColorSwapchainFormats));

    if (swapchainFormatIt == supported_swapchain_formats.end()) {
        throw("No runtime swapchain format supported for color swapchain");
    }

#if 0
    uint32_t fmt = GL_SRGB8_ALPHA8;
    std::cout << "Selected format 0x" << std::hex << fmt << std::dec
        << " " << textureInternalFormatToString(fmt) << std::endl;
    return fmt;
#else
    std::cout << "Selected format 0x" << std::hex << *swapchainFormatIt << std::dec
        << " " << textureInternalFormatToString(*swapchainFormatIt) << std::endl;
    return *swapchainFormatIt;
#endif
}


void GLSystem::renderToTexture(uint32_t tex)
{
    // TO-DO: implement me!

    std::cout << "Render to texture " << tex << std::endl;

    CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, _swapchainFramebuffer));

    const uint32_t colorTexture = tex;

    CHK_GL(glViewport(0, 0, _width, _height));
#if 0
    glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
        static_cast<GLint>(layerView.subImage.imageRect.offset.y),
        static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
        static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
#endif

//    const uint32_t depthTexture = GetDepthTexture(colorTexture);
    //glBindTexture(GL_TEXTURE_2D, colorTexture);

    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw -1;
    }

    // Clear swapchain and depth buffer.
    CHK_GL(glClearColor(0., 1., 0., 1.));
//    glClearDepth(1.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHK_GL(glClear(GL_COLOR_BUFFER_BIT));

    CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    // Render to PC Window (just for testing...)
    CHK_GL(glClearColor(0., 1., 0., 1.));
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    CHK_GL(glClear(GL_COLOR_BUFFER_BIT));

    // Swap our window every other eye for RenderDoc
    static int everyOther = 0;
    if ((everyOther++ & 1) != 0) {
        ksGpuWindow_SwapBuffers(&window);
    }
}


void GLSystem::initGLStuff()
{
    // Create FBO
    CHK_GL(glGenFramebuffers(1, &_swapchainFramebuffer));

#if 0
    // Create depth texture
    glGenTextures(1, &_depthTexture);
    glBindTexture(GL_TEXTURE_2D, _depthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
#endif

    // ...
}
