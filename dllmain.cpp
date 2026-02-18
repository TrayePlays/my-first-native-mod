#define NOMINMAX
#include <Windows.h>
#include <print>
#include <cstdlib>
#include <libhat/scanner.hpp>
#include <libhat/signature.hpp>
// std::println("a");
// std::println << "a"
#include <MinHook.h>
// #pragma pack(push, 1)
class LevelRendererPlayer {
    float getFov(LevelRendererPlayer* a1, bool enableVariableFOV, char a3);
    uint8_t padding_x[0xF80];
    float fov_x;
    uint8_t padding_y[0xF94 - 0xF80 - 4];
    float fov_y;
};

// struct LevelRenderer {
//     uint8_t padding[0x3F0];
//     LevelRendererPlayer* player;
// };
// #pragma pack(pop)

static std::atomic<uintptr_t> g_originalRenderLevel{ 0 };
static float g_zoomModifier = 1.0f;
constexpr float TARGET_ZOOM = 10.0f;

// float currentFOV = 30.0;
constexpr float baseFOV = 70.0;

float multiplier = 1.0f;
bool initZoom = true;


void HandleMouseWheelZoom() {
    SHORT wheel = GET_WHEEL_DELTA_WPARAM(GetAsyncKeyState(VK_MBUTTON));

    // Windows doesn't give wheel delta through GetAsyncKeyState,
    // so we use the raw wheel messages instead.
    // Instead, we poll the wheel using the message queue:

    MSG msg;
    while (PeekMessage(&msg, nullptr, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE)) {
        const int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
        constexpr float scrollZoomStep = 0.01;
        if (delta > 0) {
            multiplier -= scrollZoomStep;
        }
        else if (delta < 0) {
            multiplier += scrollZoomStep;
        }

        if (multiplier > 2.0f) {
            multiplier = 2.0f;
        } else if (multiplier < scrollZoomStep) {
            multiplier = 0.01f;
        }
    }
}

float __fastcall hk_getFov(uintptr_t* self, float a, bool enableVariableFOV) {
    // if (const bool quitting = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0) {
    // FreeLibraryAndExitThread();
    //     return baseFOV;
    // }
    if (const bool zooming = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
        if (initZoom) {
            multiplier = 0.2f;
            initZoom = false;
        }
        HandleMouseWheelZoom();
    } else {
        initZoom = true;
        multiplier = 1.0f;
    }
    return baseFOV * multiplier;
}

// void __fastcall hk_RenderLevel(LevelRenderer* levelRenderer, void* screenContext, void* unk) {
//     uintptr_t originalAddr = g_originalRenderLevel.load(std::memory_order_relaxed);
//     if (originalAddr != 0) {
//         auto original = reinterpret_cast<void(__fastcall*)(LevelRenderer*, void*, void*)>(originalAddr);
//         original(levelRenderer, screenContext, unk);
//
//         if (levelRenderer && levelRenderer->player) {
//             bool isCPressed = (GetAsyncKeyState('C') & 0x8000) != 0;
//             float target = isCPressed ? TARGET_ZOOM : 1.0f;
//
//             g_zoomModifier = g_zoomModifier + (target - g_zoomModifier) * 0.1f;
//
//             levelRenderer->player->fov_x *= g_zoomModifier;
//             levelRenderer->player->fov_y *= g_zoomModifier;
//         }
//     }
// }



// MinHook will redirect Dimension::getTimeOfDay to this function, replacing its behavior with our own code.
static bool specialTimeSet = false;

static float hk_getTimeOfDay(void* _this, int32_t time, float alpha) {
    const bool toggleActive = (GetAsyncKeyState('=') & 0x1) != 0;
    static float value = 0.0f;
    if (toggleActive) {
        // println("Hello World");
        specialTimeSet = !specialTimeSet;
    }
    if (specialTimeSet) {
        const bool leftPressed = (GetAsyncKeyState('[') & 0x8000) != 0;
        const bool rightPressed = (GetAsyncKeyState(']') & 0x8000) != 0;
        if (leftPressed) {
            value += 0.00001f;
            value = std::fmodf(value, 1.f);
        }
        if (rightPressed) {
            value -= 0.00001f;
            value = std::fmodf(value, 1.f);
        }
    }
        //             float target = isCPressed ? TARGET_ZOOM : 1.0f;
        //
        //             g_zoomModifier = g_zoomModifier + (target - g_zoomModifier) * 0.1f;
        //
        //             levelRenderer->player->fov_x *= g_zoomModifier;
        //             levelRenderer->player->fov_y *= g_zoomModifier;
    return value;
}

static bool g_Running = true;

static DWORD WINAPI startup(LPVOID dll) {
    constexpr auto signatureTimeOfDay = hat::compile_signature<"44 8B C2 B8 F1 19 76 05 F7 EA">();

    const hat::scan_result scanResult = hat::find_pattern(signatureTimeOfDay, ".text");
    std::byte* getTimeOfDay = scanResult.get();

    // constexpr auto signatureRenderLevel = hat::compile_signature<"48 8B C4 48 89 58 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 ? 0F 29 78 ? 44 0F 29 40 ? 44 0F 29 48 ? 44 0F 29 90 ? ? ? ? 44 0F 29 98 ? ? ? ? 44 0F 29 A0 ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4D 8B F0">();
    //
    // const hat::scan_result scanResultRenderLevel = hat::find_pattern(signatureRenderLevel, ".text");
    // std::byte* renderLevel = scanResultRenderLevel.get();

    constexpr auto signatureGetFov = hat::compile_signature<"4C 8B DC 53 48 83 EC ? 0F 29 74 24">();

    const hat::scan_result scanResultGetFOV = hat::find_pattern(signatureGetFov, ".text");
    std::byte* getFOV = scanResultGetFOV.get();

    // Initialize MinHook.
    MH_Initialize();

    // Hook getTimeOfDay.
    LPVOID original;
    // LPVOID originalLevel;
    LPVOID originalFOV;

    MH_CreateHook(getTimeOfDay, &hk_getTimeOfDay, &original);
    MH_EnableHook(getTimeOfDay);

    MH_CreateHook(getFOV, &hk_getFov, &original);
    MH_EnableHook(getFOV);

    while (g_Running) {
        if ((GetAsyncKeyState(VK_INSERT) & 0x1) != 0) {
            g_Running = false;
        }
    };
    MH_DisableHook(getTimeOfDay);
    MH_DisableHook(getFOV);

    MH_Uninitialize();

    Sleep(50);

    const auto hModule = static_cast<HMODULE>(dll);
    FreeLibraryAndExitThread(hModule, 0);
    // MH_CreateHook(renderLevel, &hk_RenderLevel, &originalLevel);
    // MH_EnableHook(renderLevel);
    // Exiting this thread without calling FreeLibraryAndExitThread on `dll` will cause the DLL to remain injected forever.
    return 0;
}

// `DllMain` is the function that will be called by Windows when your mod is injected into the game's process.
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // DLL_PROCESS_ATTACH means the DLL is being initialized.

            // We create a new thread to call `startup`.
            // This is necessary since we can't use most Windows APIs inside DllMain.
            CreateThread(nullptr, 0, &startup, hinstDLL, 0, nullptr);
            break;

        case DLL_PROCESS_DETACH:
            // DLL_PROCESS_DETACH means that the DLL is being ejected,
            // or that the game is shutting down.

        default:
            break;
    }

    return TRUE;
}

// #define NOMINMAX
// #include <Windows.h>
// #include <cstdint>
// #include <atomic>
// #include <thread>
// #include <cmath>
//
// #include <libhat/scanner.hpp>
// #include <libhat/signature.hpp>
//
// #include <MinHook.h>
//
// #pragma pack(push, 1)
// struct LevelRendererPlayer {
//     uint8_t padding_x[0xF80];
//     float fov_x;
//     uint8_t padding_y[0xF94 - 0xF80 - 4];
//     float fov_y;
// };
//
// struct LevelRenderer {
//     uint8_t padding[0x3F0];
//     LevelRendererPlayer* player;
// };
// #pragma pack(pop)
//
// // Global storage for original function pointer
// static std::atomic<uintptr_t> g_originalRenderLevel{ 0 };
// static float g_zoomModifier = 1.0f;
// constexpr float TARGET_ZOOM = 10.0f;
//
// // Our detour
// void __fastcall hk_RenderLevel(LevelRenderer* levelRenderer, void* screenContext, void* unk) {
//     uintptr_t originalAddr = g_originalRenderLevel.load(std::memory_order_relaxed);
//     if (originalAddr != 0) {
//         auto original = reinterpret_cast<void(__fastcall*)(LevelRenderer*, void*, void*)>(originalAddr);
//         original(levelRenderer, screenContext, unk);
//
//         if (levelRenderer && levelRenderer->player) {
//             bool isCPressed = (GetAsyncKeyState('C') & 0x8000) != 0;
//             float target = isCPressed ? TARGET_ZOOM : 1.0f;
//
//             g_zoomModifier = g_zoomModifier + (target - g_zoomModifier) * 0.1f;
//
//             levelRenderer->player->fov_x *= g_zoomModifier;
//             levelRenderer->player->fov_y *= g_zoomModifier;
//         }
//     }
// }
//
// static float hk_getTimeOfDay(void* _this, int32_t time, float alpha) {
//     static float value = 0.f;
//     value += 0.00001f;
//     value = std::fmodf(value, 1.f);
//
//     return value;
// }
//
// static DWORD WINAPI startup(LPVOID dll) {
//     // New signature with wildcards, in libhat format
//     // ? bytes stay as "?" just like your pattern/mask combo
//     constexpr auto signature =
//         hat::compile_signature<"48 8B C4 48 89 58 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 ? 0F 29 78 ? 44 0F 29 40 ? 44 0F 29 48 ? 44 0F 29 90 ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B DA 4C 8B F1">();
//
//     // Scan .text for the renderLevel function
//     hat::scan_result scanResult = hat::find_pattern(signature, ".text");
//     void* renderLevelAddr = scanResult.get();
//     // if (!renderLevelAddr) {
//     //     // Failed to find pattern; bail out
//     //     return 0;
//     // }
//
//     // if (MH_Initialize() != MH_OK) {
//     //     return 0;
//     // }
//
//     constexpr auto signature2 = hat::compile_signature<"44 8B C2 B8 F1 19 76 05 F7 EA">();
//
//     hat::scan_result scanResult2 = hat::find_pattern(signature, ".text");
//     std::byte* getTimeOfDay = scanResult2.get();
//
//     // Initialize MinHook.
//     MH_Initialize();
//
//     // Hook getTimeOfDay.
//     LPVOID original2;
//     MH_CreateHook(getTimeOfDay, &hk_getTimeOfDay, &original2);
//     MH_EnableHook(getTimeOfDay);
//
//     void* original = nullptr;
//     if (MH_CreateHook(renderLevelAddr, &hk_RenderLevel, &original) == MH_OK) {
//         g_originalRenderLevel.store(reinterpret_cast<uintptr_t>(original), std::memory_order_relaxed);
//         MH_EnableHook(renderLevelAddr);
//     }
//
//     // If you want to cleanly unload later, you’d call FreeLibraryAndExitThread here.
//     return 0;
// }
//
// BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
//     switch (fdwReason) {
//         case DLL_PROCESS_ATTACH:
//             // Avoid DLL_THREAD_ATTACH/DETACH notifications
//             DisableThreadLibraryCalls(hinstDLL);
//             // Start our startup routine on a new thread
//             CreateThread(nullptr, 0, &startup, hinstDLL, 0, nullptr);
//             break;
//
//         case DLL_PROCESS_DETACH:
//             // Optional: MH_DisableHook(MH_ALL_HOOKS); MH_Uninitialize();
//             break;
//
//         default:
//             break;
//     }
//
//     return TRUE;
// }
