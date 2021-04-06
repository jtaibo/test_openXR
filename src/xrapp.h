#pragma once

#include <openxr/openxr.h>
#include <vector>
#include <map>
#include <string>

#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_WIN32
#include <windows.h>
#include <openxr/openxr_platform.h>

#include "glsystem.h"


class XRApp {

public:

    XRApp();
    ~XRApp();

    void mainLoop();

private:

    void showPropertiesAndExtensions();

    XrResult createInstance();
    void createSystem();
    void enumEnvironmentBlendModes();
    void enumViewConfigurations();
    void enumViewConfigProps();
    void enumViewConfigViews();
    void configureInteraction();
    void createSession();
    void attachActionSets();
    void enumerateReferenceSpaces();
    void createReferenceSpace(XrReferenceSpaceType ref_space_type, XrSpace* space, XrExtent2Df* bounds);
    void createActionSpace();
    void enumerateSwapChainFormats();
    void createSwapchains();
    void enumerateSwapchainImages(XrSwapchain& swapchain);

    std::string resultString(XrResult res);

    void beginSession();
    std::vector<XrView> getViews(XrTime display_time, XrViewState &view_state);
    void frame();
    void processActions();

    bool _done;

    std::vector<XrExtensionProperties> _instanceExtensionProperties;
    std::vector<XrEnvironmentBlendMode> _envBlendModes;
    std::vector<XrViewConfigurationType> _viewConfigs;
    std::vector<XrViewConfigurationView> _viewConfigViews;
    std::vector<XrReferenceSpaceType> _referenceSpaces;
    std::vector<int64_t> _swapchainFormats;

    std::string _appName;
    XrInstance _instance;
    XrInstanceProperties _instanceProps;
    XrSystemId _systemID;
    XrSystemProperties _systemProps;
    GLSystem *_gfxStuff;
    XrGraphicsBindingOpenGLWin32KHR _gfxBinding;
    XrSession _session;
    XrSessionState _sstate;
    XrViewConfigurationType _viewConfType;
    XrEnvironmentBlendMode _envBlendMode;

    XrViewConfigurationProperties _viewConfProps;

    XrActionSetCreateInfo _mainActionSetInfo;
    XrActionSet _mainActionSet;

    XrSpace _viewSpace;
    XrExtent2Df _viewSpaceBounds;
    XrSpace _localSpace;
    XrExtent2Df _localSpaceBounds;
    XrSpace _stageSpace;
    XrExtent2Df _stageSpaceBounds;

    std::vector<XrSwapchain> _swapChains;
    std::map<XrSwapchain, std::vector<XrSwapchainImageOpenGLKHR> > _swapchainImages;

};
