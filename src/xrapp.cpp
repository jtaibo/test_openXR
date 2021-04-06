#include "xrapp.h"
#include <iostream>


// Check result of OpenXR API calls (throws an exception in case of failure)
#define CHK_XR(cmd) \
{ \
    XrResult res = cmd; \
    char err_msg[XR_MAX_RESULT_STRING_SIZE]; \
    if (XR_SUCCEEDED(res)) { \
        if (res != XR_SUCCESS) { \
            xrResultToString(_instance, res, err_msg); \
            std::cerr << "WARN: " << err_msg << " (" << res << ")" << std::endl; \
        } \
    } \
    else { \
        xrResultToString(_instance, res, err_msg); \
        std::cerr << "ERROR: " << err_msg << " (" << res << ") - " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw res; \
    } \
}

std::string XRApp::resultString(XrResult res)
{
    char err_msg[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(_instance, res, err_msg);
    return err_msg;
}

/**
 *  Constructor
 */
XRApp::XRApp() :
    _done(false),
    _appName("XRApp"),
    _viewConfType(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
{
    showPropertiesAndExtensions();
    createInstance();
    createSystem();
    enumEnvironmentBlendModes();
    enumViewConfigurations();
    enumViewConfigProps();
    enumViewConfigViews();
    configureInteraction(); // ActionSets, Actions, InteractionProfileBindings ...
    createSession();
    enumerateReferenceSpaces();
    createReferenceSpace(XR_REFERENCE_SPACE_TYPE_VIEW, &_viewSpace, &_viewSpaceBounds);
    createReferenceSpace(XR_REFERENCE_SPACE_TYPE_LOCAL, &_localSpace, &_localSpaceBounds);
    createReferenceSpace(XR_REFERENCE_SPACE_TYPE_STAGE, &_stageSpace, &_stageSpaceBounds);

    createActionSpace();
    attachActionSets();

    enumerateSwapChainFormats();
    createSwapchains();
}

/**
 *  Destructor
 */
XRApp::~XRApp()
{
}


/**
 *
 */
void XRApp::showPropertiesAndExtensions()
{
    XrResult res;

    uint32_t api_layer_props_count = 0;
    xrEnumerateApiLayerProperties(0, &api_layer_props_count, nullptr);
    std::cout << "API_Layer_Properties: " << api_layer_props_count << std::endl;

    // Non-layer extensions(layerName == nullptr).
    uint32_t instanceExtensionCount;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &instanceExtensionCount, nullptr);
    std::cout << instanceExtensionCount << " instance extensions" << std::endl;
    _instanceExtensionProperties.resize(instanceExtensionCount, XrExtensionProperties{ XR_TYPE_EXTENSION_PROPERTIES });
    if (XR_SUCCEEDED(res = xrEnumerateInstanceExtensionProperties(nullptr, (uint32_t)_instanceExtensionProperties.size(), &instanceExtensionCount, _instanceExtensionProperties.data()))) {

        for (const XrExtensionProperties& extension : _instanceExtensionProperties) {
            std::cout << extension.extensionName << " version " << extension.extensionVersion << std::endl;
            //extensions.push_back(extension.extensionName);
        }
    }
    else {
        std::cerr << "ERROR calling xrEnumerateInstanceExtensionProperties : " << res << std::endl;
    }
}

/**
 *
 */
XrResult XRApp::createInstance()
{
    std::vector<const char*> extensions;
    extensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);  // "XR_KHR_opengl_enable"
    
    XrInstanceCreateInfo create_info;
    memset(&create_info, 0, sizeof(create_info));
    create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
    create_info.next = nullptr;
    create_info.enabledExtensionCount = (uint32_t)extensions.size();
    create_info.enabledExtensionNames = extensions.data();
    strcpy(create_info.applicationInfo.applicationName, _appName.c_str());
    create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrResult res = xrCreateInstance(&create_info, &_instance);
    if (!XR_SUCCEEDED(res)) {
        std::cerr << "Cannot create OpenXR instance! Aborting execution." << std::endl;
        throw res;
    }

    memset(&_instanceProps, 0, sizeof(_instanceProps));
    _instanceProps.type = XR_TYPE_INSTANCE_PROPERTIES;
    CHK_XR(xrGetInstanceProperties(_instance, &_instanceProps));

    std::cout << "Runtime: " << _instanceProps.runtimeName << " "
        << XR_VERSION_MAJOR(_instanceProps.runtimeVersion) << "."
        << XR_VERSION_MINOR(_instanceProps.runtimeVersion) << "."
        << XR_VERSION_PATCH(_instanceProps.runtimeVersion)
        << std::endl;

    return res;
}

/**
 *
 */
void XRApp::createSystem()
{
    XrSystemGetInfo system_get_info;
    system_get_info.type = XR_TYPE_SYSTEM_GET_INFO;
    system_get_info.next = nullptr;
    system_get_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    CHK_XR(xrGetSystem(_instance, &system_get_info, &_systemID));

    memset(&_systemProps, 0, sizeof(_systemProps));
    _systemProps.type = XR_TYPE_SYSTEM_PROPERTIES;
    CHK_XR( xrGetSystemProperties(_instance, _systemID, &_systemProps) );
    std::cout << "SYSTEM PROPERTIES: " << std::endl;
    std::cout << "System name: " << _systemProps.systemName << std::endl;
    std::cout << "Vendor ID: " << _systemProps.vendorId << std::endl;
    std::cout << "Max layer count: " << _systemProps.graphicsProperties.maxLayerCount << std::endl;
    std::cout << "Max swapchain image width: " << _systemProps.graphicsProperties.maxSwapchainImageWidth << std::endl;
    std::cout << "Max swapchain image height: " << _systemProps.graphicsProperties.maxSwapchainImageHeight << std::endl;
    std::cout << "Position tracking: " << _systemProps.trackingProperties.positionTracking << std::endl;
    std::cout << "Orientation tracking: " << _systemProps.trackingProperties.orientationTracking << std::endl;
}

/**
 *
 */
void XRApp::enumEnvironmentBlendModes()
{
    uint32_t cap_input = 0;
    uint32_t count_output = 0;
    CHK_XR( xrEnumerateEnvironmentBlendModes(_instance, _systemID, _viewConfType, cap_input, &count_output, nullptr) );
    std::cout << count_output << " environment blend mode(s)" << std::endl;
    cap_input = count_output;
    _envBlendModes.resize(count_output);
    CHK_XR( xrEnumerateEnvironmentBlendModes(_instance, _systemID, _viewConfType, cap_input, &count_output, _envBlendModes.data()) );
    for (const XrEnvironmentBlendMode& env_blend_mode : _envBlendModes) {
        std::cout << "Environment blend mode: " << env_blend_mode << " ";
        switch (env_blend_mode) {
        case XR_ENVIRONMENT_BLEND_MODE_OPAQUE:
            std::cout << "XR_ENVIRONMENT_BLEND_MODE_OPAQUE" << std::endl;
            break;
        case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE:
            std::cout << "XR_ENVIRONMENT_BLEND_MODE_ADDITIVE" << std::endl;
            break;
        case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND:
            std::cout << "XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND" << std::endl;
            break;
        }
        std::cout << std::endl;
    }

    // Set the first supported one (probably the only supported one) as the selected blend mode
    _envBlendMode = _envBlendModes[0];
}

/**
 *
 */
void XRApp::enumViewConfigurations()
{
    uint32_t cap_input = 0;
    uint32_t count_output = 0;
    CHK_XR( xrEnumerateViewConfigurations(_instance, _systemID, cap_input, &count_output, NULL) );
    std::cout << count_output << " view configuration(s)" << std::endl;
    cap_input = count_output;
    _viewConfigs.resize(count_output);
    CHK_XR( xrEnumerateViewConfigurations(_instance, _systemID, cap_input, &count_output, _viewConfigs.data()) );
    for (const XrViewConfigurationType& view_conf : _viewConfigs) {
        std::cout << "View configuration: " << view_conf << " ";
        switch (view_conf) {
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
            std::cout << "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO" << std::endl;
            break;
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
            std::cout << "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO" << std::endl;
            break;
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:
            std::cout << "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO" << std::endl;
            break;
        case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT:
            std::cout << "XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT" << std::endl;
            break;
        }
        std::cout << std::endl;
    }
}

/**
 *
 */
void XRApp::enumViewConfigProps()
{
    _viewConfProps = {XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
    CHK_XR( xrGetViewConfigurationProperties(_instance, _systemID, _viewConfType, &_viewConfProps) );
    std::cout << "FOV mutable: " << _viewConfProps.fovMutable << std::endl;
}

/**
 *
 */
void XRApp::enumViewConfigViews()
{
    uint32_t cap_input = 0;
    uint32_t count_output = 0;
    CHK_XR(xrEnumerateViewConfigurationViews(_instance, _systemID, _viewConfType, cap_input, &count_output, NULL));
    std::cout << count_output << " view configuration views(s)" << std::endl;
    cap_input = count_output;
    _viewConfigViews.resize(count_output, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    CHK_XR(xrEnumerateViewConfigurationViews(_instance, _systemID, _viewConfType, cap_input, &count_output, _viewConfigViews.data()));
    int cfg_idx = 0;
    for (const XrViewConfigurationView& view_conf_view : _viewConfigViews) {
        std::cout << "View configuration view #" << cfg_idx++ << ": " << std::endl;
        std::cout << "  Recommended image rect width " << view_conf_view.recommendedImageRectWidth << std::endl;
        std::cout << "  Max image rect width " << view_conf_view.maxImageRectWidth << std::endl;
        std::cout << "  Recommended image rect height " << view_conf_view.recommendedImageRectHeight << std::endl;
        std::cout << "  Max image rect height " << view_conf_view.maxImageRectHeight << std::endl;
        std::cout << "  Recommended swapchain sample count " << view_conf_view.recommendedSwapchainSampleCount << std::endl;
        std::cout << "  Max swapchain sample count " << view_conf_view.maxSwapchainSampleCount << std::endl;
        std::cout << "  next: " << view_conf_view.next << std::endl;
    }
}

/**
 *
 */
void XRApp::createSession()
{
    _gfxBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
    _gfxBinding.next = nullptr;
    _gfxStuff = new GLSystem();
    // to-do: get wxh in systemProps and viewConfigViews
    // Let's suppose that all views (both eyes) have the same dimensions
    _gfxStuff->initializeDevice(_instance, _systemID, _viewConfigViews[0].recommendedImageRectWidth, _viewConfigViews[0].recommendedImageRectHeight);
    _gfxBinding.hDC = _gfxStuff->getHDC();
    _gfxBinding.hGLRC = _gfxStuff->getHGLRC();

    XrSessionCreateInfo session_create_info;
    memset(&session_create_info, 0, sizeof(session_create_info));
    session_create_info.type = XR_TYPE_SESSION_CREATE_INFO;
    session_create_info.systemId = _systemID;
    session_create_info.createFlags = 0;
    session_create_info.next = &_gfxBinding;    // Pointer to Graphics API binding structure
    CHK_XR(xrCreateSession(_instance, &session_create_info, &_session));
}

/**
 *
 */
void XRApp::configureInteraction()
{
    _mainActionSetInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    strcpy(_mainActionSetInfo.actionSetName, "gameplay");
    strcpy(_mainActionSetInfo.localizedActionSetName, "Gameplay");
    _mainActionSetInfo.priority = 0;
    CHK_XR(xrCreateActionSet(_instance, &_mainActionSetInfo, &_mainActionSet));

    // ...some example actions (TO-DO: fixme!)

    // create a "teleport" input action
    XrActionCreateInfo actioninfo{ XR_TYPE_ACTION_CREATE_INFO };
    strcpy(actioninfo.actionName, "teleport");
    actioninfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy(actioninfo.localizedActionName, "Teleport");
    XrAction teleportAction;
    CHK_XR(xrCreateAction(_mainActionSet, &actioninfo, &teleportAction));

    // create a "player_hit" output action
    XrActionCreateInfo hapticsactioninfo{ XR_TYPE_ACTION_CREATE_INFO };
    strcpy(hapticsactioninfo.actionName, "player_hit");
    hapticsactioninfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
    strcpy(hapticsactioninfo.localizedActionName, "Player hit");
    XrAction hapticsAction;
    CHK_XR(xrCreateAction(_mainActionSet, &hapticsactioninfo, &hapticsAction));

    XrPath triggerClickPath, hapticPath;
    CHK_XR(xrStringToPath(_instance, "/user/hand/right/input/trigger/click", &triggerClickPath));   // <-- This path is UNSUPPORTED! WTF!!??
    CHK_XR(xrStringToPath(_instance, "/user/hand/right/output/haptic", &hapticPath));

    XrPath interactionProfilePath;

    //    CHK_XR(xrStringToPath(_instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath));
    CHK_XR(xrStringToPath(_instance, "/interaction_profiles/khr/simple_controller", &interactionProfilePath));

#if 0
    XrActionSuggestedBinding bindings[2];
    bindings[0].action = teleportAction;
    bindings[0].binding = triggerClickPath;
    bindings[1].action = hapticsAction;
    bindings[1].binding = hapticPath;

    XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    suggestedBindings.interactionProfile = interactionProfilePath;
    suggestedBindings.suggestedBindings = bindings;
    suggestedBindings.countSuggestedBindings = 2;
    CHK_XR(xrSuggestInteractionProfileBindings(_instance, &suggestedBindings));
#else
    std::vector<XrActionSuggestedBinding> bindings;
//    bindings.push_back({teleportAction, triggerClickPath});   // <-- This path is UNSUPPORTED! WTF!!??
    bindings.push_back({ hapticsAction, hapticPath });

    XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    suggestedBindings.interactionProfile = interactionProfilePath;
    suggestedBindings.suggestedBindings = bindings.data();
    suggestedBindings.countSuggestedBindings = bindings.size();
    CHK_XR(xrSuggestInteractionProfileBindings(_instance, &suggestedBindings));
#endif
}

/**
 *
 */
void XRApp::attachActionSets()
{
    // After session has been created, attach action sets
    XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &_mainActionSet;
    CHK_XR(xrAttachSessionActionSets(_session, &attachInfo));
}

/**
 *
 */
void XRApp::enumerateReferenceSpaces()
{
    uint32_t cap_input = 0;
    uint32_t count_output = 0;
    CHK_XR(xrEnumerateReferenceSpaces(_session, cap_input, &count_output, nullptr));
    std::cout << count_output << " reference space(s)" << std::endl;
    cap_input = count_output;
    _referenceSpaces.resize(count_output);
    CHK_XR(xrEnumerateReferenceSpaces(_session,cap_input, &count_output, _referenceSpaces.data()));
    for (const XrReferenceSpaceType& ref_space : _referenceSpaces) {
        std::cout << "Reference space: " << ref_space << " ";
        switch (ref_space) {
        case XR_REFERENCE_SPACE_TYPE_VIEW:
            std::cout << "XR_REFERENCE_SPACE_TYPE_VIEW" << std::endl;
            break;
        case XR_REFERENCE_SPACE_TYPE_LOCAL:
            std::cout << "XR_REFERENCE_SPACE_TYPE_LOCAL" << std::endl;
            break;
        case XR_REFERENCE_SPACE_TYPE_STAGE:
            std::cout << "XR_REFERENCE_SPACE_TYPE_STAGE" << std::endl;
            break;
        case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT:
            std::cout << "XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT" << std::endl;
            break;
        }
    }
}

/**
 *
 */
void XRApp::createReferenceSpace(XrReferenceSpaceType ref_space_type, XrSpace *space, XrExtent2Df *bounds)
{
    XrReferenceSpaceCreateInfo create_info{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    create_info.referenceSpaceType = ref_space_type;
    create_info.poseInReferenceSpace.orientation.w = 1.;
    CHK_XR(xrCreateReferenceSpace(_session, &create_info, space));

    CHK_XR(xrGetReferenceSpaceBoundsRect(_session, ref_space_type, bounds));
    std::cout << "Bounds for reference space: " << bounds->width << "x" << bounds->height << std::endl;
}

void XRApp::createActionSpace()
{
    XrActionCreateInfo actioninfo{ XR_TYPE_ACTION_CREATE_INFO };
    strcpy(actioninfo.actionName, "right_aim");
    actioninfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy(actioninfo.localizedActionName, "Right Aim");
    XrAction poseAction;
    CHK_XR(xrCreateAction(_mainActionSet, &actioninfo, &poseAction));

    XrPath rightHandAimInteractionProfilePath;
    CHK_XR(xrStringToPath(_instance, "/user/hand/right/input/aim", &rightHandAimInteractionProfilePath));

    XrSpace action_space;
    XrActionSpaceCreateInfo create_info{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
    create_info.next = nullptr;
    create_info.action = poseAction;
    create_info.poseInActionSpace.orientation.w = 1.;
    CHK_XR(xrCreateActionSpace(_session, &create_info, &action_space));


}

void XRApp::enumerateSwapChainFormats()
{
    uint32_t cap_input = 0;
    uint32_t count_output = 0;
    CHK_XR(xrEnumerateSwapchainFormats(_session, cap_input, &count_output, nullptr));
    std::cout << count_output << " swapchain format(s)" << std::endl;
    cap_input = count_output;
    _swapchainFormats.resize(count_output);
    CHK_XR(xrEnumerateSwapchainFormats(_session, cap_input, &count_output, _swapchainFormats.data()));
    for (const int64_t& swapchain_format : _swapchainFormats) {
        std::cout << "Swapchain format: 0x" << std::hex << swapchain_format << std::dec << " " << _gfxStuff->textureInternalFormatToString(swapchain_format) << std::endl;
    }
}

/**
 *  @todo *************** STORE SWAPCHAINS INFORMATION AND OPENGL TEXTURES IN ATTRIBUTES
 */
void XRApp::createSwapchains()
{
    for ( const XrViewConfigurationView &view : _viewConfigViews) {

        XrSwapchainCreateInfo create_info{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        /*
            XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT
            XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT
        */
        create_info.createFlags = 0;
        /*
            XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT
            XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT
            XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT
            XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT
            XR_SWAPCHAIN_USAGE_SAMPLED_BIT
            XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT
            XR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT_MND
        */
        create_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        create_info.format = _gfxStuff->getFormat(_swapchainFormats);

        create_info.sampleCount = view.recommendedSwapchainSampleCount;
        create_info.width = view.recommendedImageRectWidth;
        create_info.height = view.recommendedImageRectHeight;
        create_info.faceCount = 1;
        create_info.arraySize = 1;
        create_info.mipCount = 1;
        XrSwapchain swapchain;
        CHK_XR(xrCreateSwapchain(_session, &create_info, &swapchain));
        std::cout << "Created swapchain: " << create_info.width << "x" << create_info.height << std::endl;
        _swapChains.push_back(swapchain);
        enumerateSwapchainImages(swapchain);
    }
}

void XRApp::enumerateSwapchainImages(XrSwapchain& swapchain)
{
    uint32_t cap_input = 0;
    uint32_t count_output = 0;
    CHK_XR(xrEnumerateSwapchainImages(swapchain, cap_input, &count_output, nullptr));
    std::cout << count_output << " swapchain image(s)" << std::endl;
    cap_input = count_output;
    // Allocate SwapchainImage structures
    std::vector<XrSwapchainImageOpenGLKHR> swapchain_images(count_output);
    for (XrSwapchainImageOpenGLKHR& swi : swapchain_images) {
        swi.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
    }
    CHK_XR(xrEnumerateSwapchainImages(swapchain, cap_input, &count_output, (XrSwapchainImageBaseHeader*)swapchain_images.data()));
    for (const XrSwapchainImageOpenGLKHR& img : swapchain_images) {
        std::cout << "OpenGL texture handle: " << img.image << std::endl;
    }
    _swapchainImages[swapchain] = swapchain_images;
}


void XRApp::mainLoop()
{
    while (!_done) {

        // 1. Event processing

        XrEventDataBuffer event{ XR_TYPE_EVENT_DATA_BUFFER };
        XrResult res = xrPollEvent(_instance, &event);
        if (XR_FAILED(res)) {
            std::cerr << "Error polling event : " << resultString(res) << std::endl;
            _done = true;
            break;
        }
        else {
            switch (res) {
            case XR_SUCCESS:
                std::cout << "Event type " << event.type << std::endl;
                switch (event.type) {
                case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB:
                {
                    XrEventDataDisplayRefreshRateChangedFB* ev = (XrEventDataDisplayRefreshRateChangedFB*)&event;
                    break;
                }
                break;
                case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                {
                    XrEventDataEventsLost* ev = (XrEventDataEventsLost*)&event;
                    break;
                }
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                {
                    XrEventDataInstanceLossPending* ev = (XrEventDataInstanceLossPending*)&event;
                    break;
                }
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                {
                    XrEventDataInteractionProfileChanged* ev = (XrEventDataInteractionProfileChanged*) &event;
                    std::cout << "INTERACTION_PROFILE_CHANGED" << std::endl;
                    break;
                }
                case XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX:
                {
                    XrEventDataMainSessionVisibilityChangedEXTX* ev = (XrEventDataMainSessionVisibilityChangedEXTX*)&event;
                    break;
                }
                case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
                {
                    XrEventDataPerfSettingsEXT* ev = (XrEventDataPerfSettingsEXT*)&event;
                    break;
                }
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                {
                    XrEventDataReferenceSpaceChangePending* ev = (XrEventDataReferenceSpaceChangePending*)&event;
                    break;
                }
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
                {
                    XrEventDataSessionStateChanged* ev = (XrEventDataSessionStateChanged*)&event;
                    _sstate = ev->state;
                    std::cout << "Session state " << ev->state << std::endl;
                    switch (ev->state) {
                    case XR_SESSION_STATE_IDLE:
                        std::cout << "IDLE. Waiting to be ready..." << std::endl;
                        break;
                    case XR_SESSION_STATE_READY:
                        std::cout << "READY. Starting session..." << std::endl;
                        beginSession();
                        break;
                    case XR_SESSION_STATE_SYNCHRONIZED:
                        std::cout << "SYNCHRONIZED" << std::endl;
                        break;
                    case XR_SESSION_STATE_VISIBLE:
                        std::cout << "VISIBLE" << std::endl;
                        break;
                    case XR_SESSION_STATE_FOCUSED:
                        std::cout << "FOCUSED" << std::endl;
                        break;
                    }
                    break;
                }
                case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
                {
                    XrEventDataVisibilityMaskChangedKHR* ev = (XrEventDataVisibilityMaskChangedKHR*)&event;
                    break;
                }
                }
                break;
            case XR_EVENT_UNAVAILABLE:
                // No event yet
                break;
            }
        }

        // 2. Frame processing
        switch (_sstate) {
        case XR_SESSION_STATE_READY:
        case XR_SESSION_STATE_SYNCHRONIZED:
        case XR_SESSION_STATE_VISIBLE:
        case XR_SESSION_STATE_FOCUSED:
            frame();
            break;
        }
    }
}


void XRApp::beginSession()
{
    XrSessionBeginInfo begin_info{XR_TYPE_SESSION_BEGIN_INFO};
    begin_info.primaryViewConfigurationType = _viewConfType;
    CHK_XR(xrBeginSession(_session, &begin_info));
}

void XRApp::frame()
{
    XrFrameState frame_state{XR_TYPE_FRAME_STATE};
    XrFrameWaitInfo frame_wait_info{ XR_TYPE_FRAME_WAIT_INFO };
    CHK_XR(xrWaitFrame(_session, &frame_wait_info, &frame_state));
#if 0
    std::cout << "Frame state - pred. disp. period: " << frame_state.predictedDisplayPeriod
        << " pred. disp. time: " << frame_state.predictedDisplayTime
        << " should render: " << frame_state.shouldRender
        << std::endl;
#endif

    XrFrameBeginInfo frame_begin_info{XR_TYPE_FRAME_BEGIN_INFO};
    CHK_XR(xrBeginFrame(_session, &frame_begin_info));
#if 0
    std::cout << ((_sstate == XR_SESSION_STATE_VISIBLE) ? "VISIBLE " : "")
        << ((_sstate == XR_SESSION_STATE_FOCUSED) ? "FOCUSED " : "") 
        << ((frame_state.shouldRender) ? "SHOULD_RENDER" : "")
        << std::endl;
#endif

    // Input events processing (may be done in another thread)
    if (_sstate == XR_SESSION_STATE_FOCUSED) {
        processActions();
    }

    std::vector<XrCompositionLayerBaseHeader*> layers;
    std::vector<XrCompositionLayerProjectionView> projViews;
    XrCompositionLayerProjection layerProj{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };

    if ( _sstate == XR_SESSION_STATE_VISIBLE || _sstate == XR_SESSION_STATE_FOCUSED ) {
//    if ( frame_state.shouldRender ) {

        // Get camera information (for both eyes) - calls xrLocateViews
        XrViewState view_state{ XR_TYPE_VIEW_STATE };
        std::vector<XrView> views = getViews( frame_state.predictedDisplayTime, view_state);

        // Check valid states to abort rendering when necessary
        if ( (view_state.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) &&
             (view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT))
        {
            // Swapchains and views must match in quantity
            if (_swapChains.size() != views.size()) {
                std::cerr << views.size() << " views and " << _swapChains.size() << " swapchains. This is bad. Really really BAD" << std::endl;
                throw -1;
            }

            // For each swapchain AND view (one per eye)
            for (int i = 0; i < _swapChains.size(); i++) {

                XrSwapchain& swapchain = _swapChains[i];
                const XrView& view = views[i];
                const XrViewConfigurationView& view_cfg = _viewConfigViews[i];

                XrSwapchainImageAcquireInfo acquire_info{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
                uint32_t idx;
                CHK_XR(xrAcquireSwapchainImage(swapchain, &acquire_info, &idx));
//                std::cout << "idx: " << idx << std::endl;

                XrSwapchainImageWaitInfo wait_info{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
                wait_info.timeout = 0;
                CHK_XR(xrWaitSwapchainImage(swapchain, &wait_info));

                // Render to texture #idx (GL stuff)
                uint32_t tex_gl_id = _swapchainImages[swapchain][idx].image;
//                std::cout << "Rendering to texture ID " << tex_gl_id << std::endl;
                _gfxStuff->renderToTexture(tex_gl_id);

                XrSwapchainImageReleaseInfo release_info{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
                CHK_XR(xrReleaseSwapchainImage(swapchain, &release_info));

                // Assemble composition layers structure
                XrCompositionLayerProjectionView proj_view{ XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
                proj_view.pose = view.pose;
                proj_view.fov = view.fov;
                proj_view.subImage.imageArrayIndex = 0;
                proj_view.subImage.imageRect.offset = {0,0};
                proj_view.subImage.imageRect.extent = { (int32_t)view_cfg.recommendedImageRectWidth, (int32_t)view_cfg.recommendedImageRectHeight };
                proj_view.subImage.swapchain = swapchain;
                projViews.push_back(proj_view);
            }
            layerProj.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
            layerProj.space = _stageSpace;
            layerProj.viewCount = projViews.size();
            layerProj.views = projViews.data();
            layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layerProj));
        }
    }

    XrFrameEndInfo frame_end_info{XR_TYPE_FRAME_END_INFO};
    frame_end_info.environmentBlendMode = _envBlendMode;
    frame_end_info.displayTime = frame_state.predictedDisplayTime;
    frame_end_info.layerCount = layers.size();
    frame_end_info.layers = layers.data();
    std::cout << "### END FRAME ###" << std::endl;
    CHK_XR(xrEndFrame(_session, &frame_end_info));
}

std::vector<XrView> XRApp::getViews(XrTime display_time, XrViewState &view_state)
{
    uint32_t cap_input = 0;
    uint32_t count_output = 0;
    XrViewLocateInfo view_locate_info{ XR_TYPE_VIEW_LOCATE_INFO };
    view_locate_info.viewConfigurationType = _viewConfType;
    view_locate_info.displayTime = display_time;
    view_locate_info.space = _stageSpace;
    CHK_XR(xrLocateViews(_session, &view_locate_info, &view_state, cap_input, &count_output, nullptr));
    //std::cout << count_output << " view(s)" << std::endl;
    cap_input = count_output;
    std::vector<XrView> views(count_output, {XR_TYPE_VIEW});
    CHK_XR(xrLocateViews(_session, &view_locate_info, &view_state, cap_input, &count_output, views.data()));
#if 0
    for (XrView& view : views) {
        std::cout << "View state: "
            << ((view_state.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) ? "position_valid " : "")
            << ((view_state.viewStateFlags & XR_VIEW_STATE_POSITION_TRACKED_BIT) ? "position_tracked " : "")
            << ((view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) ? "orientation_valid " : "")
            << ((view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_TRACKED_BIT) ? "orientation_tracked " : "")
            << std::endl;
        std::cout << "Position: " << view.pose.position.x << ", " << view.pose.position.y << ", " << view.pose.position.z << std::endl;
        std::cout << "Orientation: " << view.pose.orientation.x << ", " << view.pose.orientation.y << ", " << view.pose.orientation.z << ", " << view.pose.orientation.w << std::endl;
        std::cout << "FOV: " << view.fov.angleDown << " " << view.fov.angleUp << " " << view.fov.angleLeft << " " << view.fov.angleRight << std::endl;
    }
#endif
    return views;
}


void XRApp::processActions()
{
    XrActionsSyncInfo sync_info{ XR_TYPE_ACTIONS_SYNC_INFO };
    sync_info.countActiveActionSets = 1;
    XrActiveActionSet activeActionSet{ _mainActionSet, XR_NULL_PATH };
    sync_info.activeActionSets = &activeActionSet;
    CHK_XR(xrSyncActions(_session, &sync_info));
}
