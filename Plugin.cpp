// Install DebugView to view the OutputDebugString calls
#include <sstream>
#include <mutex>

#include <Windows.h>

#include <reframework/API.hpp>

//Attenuation includes
#include <winrt/windows.foundation.h>
#include <winrt/windows.media.control.h>


#include "Plugin.hpp"

using namespace reframework;
using namespace winrt;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Foundation;

static void SimpleError(const char* str) {
    API::get()->log_error("%s", str);
}

static GlobalSystemMediaTransportControlsSessionManager sessionManager = nullptr;

static GlobalSystemMediaTransportControlsSessionManager GetManager() {
    if (sessionManager) return sessionManager;

    try {
        init_apartment();
        return sessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    }
    catch (...) {
        SimpleError("Could not retrieve manager.");
        return nullptr;
    }

}

static GlobalSystemMediaTransportControlsSession GetSession() {
    auto session_manager = GetManager();
    if (!session_manager) {
        return nullptr;
    }

    try {
        return session_manager.GetCurrentSession();
    }
    catch (...) {
        SimpleError("Failed to retrieve current session.");
        return nullptr;
    }

}

static GlobalSystemMediaTransportControlsSessionPlaybackInfo GetPlaybackInfo() {
    auto session = GetSession();
    if (!session) {
        return nullptr;
    }

    try {
        return session.GetPlaybackInfo();
    }
    catch (...) {
        SimpleError("Failed to retrieve current session's playback info.");
        return nullptr;
    }

}

static bool IsExternalMediaPlaying() {
    auto playback_info = GetPlaybackInfo();
    if (!playback_info) return false;

    try {
        API::get()->log_warn("Getting playback info");
        return playback_info.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
    }
    catch (...) {
        return false;
    }
}

static reframework::API::Method* deltaFunc = nullptr;
thread_local static int optionType = -1;

int pre_start(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
    optionType = (int)argv[1];
    return REFRAMEWORK_HOOK_CALL_ORIGINAL;
}

static int current_volume = 80;

void post_start(void** ret_val, REFrameworkTypeDefinitionHandle ret_ty, unsigned long long ret_addr) {
    if (optionType != 141) return;

    auto app = API::get()->get_native_singleton("via.Application");
    auto type = API::get()->tdb()->find_type("via.Application");
    if (!deltaFunc) deltaFunc = type->find_method("get_DeltaTime");
    auto deltaTime = deltaFunc->call<float>(API::get()->sdk()->vm_context, app);
    if (!deltaTime) return;

    auto time = 1.0f - expf(1.5f * -deltaTime);
    //API::get()->log_warn("%f", deltaTime);
    int* retVal = (int *)ret_val;
    API::get()->log_warn("%i", *retVal);

    if (IsExternalMediaPlaying()) {
        *retVal = (int)round(std::lerp(current_volume, 0, time));
    }
    else {
        *retVal = (int)round(std::lerp(current_volume, *retVal, time));
    }
    current_volume = *retVal;
}

extern "C" __declspec(dllexport) void reframework_plugin_required_version(REFrameworkPluginVersion* version) {
    version->major = REFRAMEWORK_PLUGIN_VERSION_MAJOR;
    version->minor = REFRAMEWORK_PLUGIN_VERSION_MINOR;
    version->patch = REFRAMEWORK_PLUGIN_VERSION_PATCH;
    version->game_name = "MHWILDS";

    // Optionally, specify a specific game name that this plugin is compatible with.
    //version->game_name = "MHRISE";
}

extern "C" __declspec(dllexport) bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam* param) {
    API::initialize(param);

    const auto functions = param->functions;
    //functions->on_lua_state_created(on_lua_state_created);
    //functions->on_lua_state_destroyed(on_lua_state_destroyed);
    //functions->on_present(on_present);
    //functions->on_pre_application_entry("UpdateWwise", on_update_wwise);
    //functions->on_post_application_entry("EndRendering", on_post_end_rendering);
    //functions->on_device_reset(on_device_reset);
    //functions->on_message((REFOnMessageCb)on_message);
    functions->log_error("%s %s", "Hello", "error");
    functions->log_warn("%s %s", "Hello", "warning");
    functions->log_info("%s %s", "Hello", "info");
    typedef LONG RtlGetVersion_1(PRTL_OSVERSIONINFOW);
    RtlGetVersion_1* RtlGetVersion = (RtlGetVersion_1*)GetProcAddress(LoadLibrary(TEXT("ntdll.dll")), "RtlGetVersion");

    OSVERSIONINFOEXW osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    if (RtlGetVersion((PRTL_OSVERSIONINFOW)&osvi) != 0)
        return false;

    if (osvi.dwMajorVersion < 10 || (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber < 17763)) {
        SimpleError("This plugin only supports Windows 10 build 17763 or greater.");
        return false;
    }
    

    API::get()->tdb()->find_type("app.OptionUtil")->find_method("getOptionValue")->add_hook(pre_start, post_start, false);

    return true;
}

// you DO NOT need to have a DllMain, this is only necessary
// if your DLL needs to load immediately, like in a raw plugin.
// or if you want to do some pre-initialization in the DllMain
// or if you want to do some cleanup in the DllMain
BOOL APIENTRY DllMain(HANDLE handle, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        //OutputDebugString("Plugin early load test.");
    }

    return TRUE;
}
