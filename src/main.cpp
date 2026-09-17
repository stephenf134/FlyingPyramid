#include <cmath>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>

#include <windows.h>

#include <bgfx/bgfx.h>
#include <bx/platform.h>
#include <bx/math.h>

#include "MyAnimation/ObjectManager.hpp"

static std::filesystem::path g_exec_parent_path;
static void init_exec_dir_path();

// #define PRJ_DIR    "C:\\Users\\safre\\CLionProjects\\MyAnimation"

// Objects
MyAnimation::ObjectManager* g_objectManager = nullptr;

bx::Vec3 user_position(-8.0f, 0.0f, 0.0f);
static const bx::Vec3 cameraUp(0.0f, 1.0f, 0.0f);

// Preserve the initial heading; movement should not turn the camera toward the cube.
static float cameraYaw = 0.0f;
static float cameraPitch = 0.0f;
static constexpr float turnSpeed = std::numbers::pi / 2.0f;
static constexpr float pitchLimit = 89.0f / 180.0f * std::numbers::pi_v<float>;

static constexpr float cameraSpeed = 5.0f; // World units per second.

// camera rotation
static bool g_kbd_left = false;
static bool g_kbd_right = false;
static bool g_kbd_up = false;
static bool g_kbd_down = false;

// camera movement
static bool g_kbd_w = false;
static bool g_kbd_a = false;
static bool g_kbd_s = false;
static bool g_kbd_d = false;
static bool g_kbd_spc = false;
static bool g_kbd_shft = false;

static uint32_t g_width  = 1280;
static uint32_t g_height = 720;
static HWND g_hwnd = nullptr;

static bool g_bgfxInitialized = false;
static bool g_running = true;

static bgfx::ShaderHandle ldShader(const std::string& filename) {
    std::ifstream fstrm(filename, std::ios::binary | std::ios::ate);

    if (!fstrm.is_open()) {
        (MessageBoxA(nullptr, (std::string("error opening shader file") + filename).c_str(), "Error", MB_OK));
        std::cout << "error opening shader file " << filename << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    // std::ios::ate set our position at the end of the file. so now we can
    // get the file size
    const std::streamsize stream_size = fstrm.tellg();

    fstrm.seekg(0, std::ios::beg);

    const bgfx::Memory* mem = bgfx::alloc(static_cast<uint32_t>(stream_size + 1));

    if (!fstrm.read(reinterpret_cast<char*>(mem->data), stream_size))
        return BGFX_INVALID_HANDLE;
    mem->data[stream_size] = '\0';
    return bgfx::createShader(mem);
}

static bgfx::ProgramHandle ldProgram(const std::string& vs_path, const std::string& fs_path) {
    /* create the bgfx::ShaderHandle for vertex and fragment shader, vsh and fsh respectively */
    auto vsh = ldShader(vs_path);
    auto fsh = ldShader(fs_path);

    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        return BGFX_INVALID_HANDLE;
    }

    return bgfx::createProgram(vsh, fsh, true);


}

static void resizeWinBGFX(void) {
    if (!g_bgfxInitialized)
        return;

    if (!g_width || !g_height)
        return;

    bgfx::SwapChain swapChain;
    swapChain.height = g_height;
    swapChain.width = g_width;
    bgfx::reset(BGFX_RESET_VSYNC, &swapChain);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CLOSE: {
            g_running = false;
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_DESTROY: {
            g_running = false;
            PostQuitMessage(0);
            return 0;
        }

        case WM_SIZE:
        {
            g_width =
                LOWORD(lParam);

            g_height =
                HIWORD(lParam);

            resizeWinBGFX();

            return 0;
        }

        case WM_KILLFOCUS: {
            g_kbd_w = g_kbd_a = g_kbd_s = g_kbd_d = false;
            g_kbd_spc = g_kbd_shft = false;
            g_kbd_left = g_kbd_right = g_kbd_up = g_kbd_down = false;
            return 0;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_LEFT:
                    g_kbd_left = true;
                    return 0;
                case VK_RIGHT:
                    g_kbd_right = true;
                    return 0;
                case VK_UP:
                    g_kbd_up = true;
                    return 0;
                case VK_DOWN:
                    g_kbd_down = true;
                    return 0;

                case 'W':
                    g_kbd_w = true;
                    return 0;
                case 'A':
                    g_kbd_a = true;
                    return 0;
                case 'S':
                    g_kbd_s = true;
                    return 0;
                case 'D':
                    g_kbd_d = true;
                    return 0;
                case ' ':
                    g_kbd_spc = true;
                    return 0;
                case VK_SHIFT:
                    g_kbd_shft = true;
                    return 0;
            }

            break;
        }

        case WM_KEYUP: {
            switch (wParam) {
                case VK_LEFT:
                    g_kbd_left = false;
                    return 0;
                case VK_RIGHT:
                    g_kbd_right = false;
                    return 0;
                case VK_UP:
                    g_kbd_up = false;
                    return 0;
                case VK_DOWN:
                    g_kbd_down = false;
                    return 0;

                case 'W':
                    g_kbd_w = false;
                    return 0;
                case 'A':
                    g_kbd_a = false;
                    return 0;
                case 'S':
                    g_kbd_s = false;
                    return 0;
                case 'D':
                    g_kbd_d = false;
                    return 0;
                case ' ':
                    g_kbd_spc = false;
                    return 0;
                case VK_SHIFT:
                    g_kbd_shft = false;
                    return 0;
            }
            break;
        }
    }

        return DefWindowProcW(hWnd, message, wParam, lParam);
}

static bool createWindow(HINSTANCE hInstance) {
    const auto windowClassName = L"BGFX-3D11";
    WNDCLASSEXW windowClass = {};

    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = windowClassName;

    if (!RegisterClassExW((&windowClass))) {
        return false;
    }

    RECT windowRect = {
        0,
        0,
        static_cast<LONG>(g_width),
        static_cast<LONG>(g_height)
    };

    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowExW(
        0,
        windowClassName,
        windowClassName, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);
    if (!g_hwnd) {
        MessageBoxA(nullptr, "Error: (!g_hwnd evaluated as true statement)", "Error", MB_OK);
        return false;
    }
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    return true;
}

static float tmp_rotAngle = 0.0f;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prev_instance, LPSTR cmd_line, int show) {

    if (!createWindow(hInstance)) {
        MessageBoxA(nullptr, "Error creating window!", "Error", MB_OK);
    }

    init_exec_dir_path();

    bgfx::Init init;
    init.type = bgfx::RendererType::Direct3D11;
    init.vendorId = BGFX_PCI_ID_NONE;
    init.swapChain.nwh = g_hwnd;
    init.swapChain.ndt = nullptr;
    init.swapChain.width = g_width;
    init.swapChain.height = g_height;
    init.reset = BGFX_RESET_VSYNC; // helps with things like modding params
    init.platformData.type = bgfx::NativeWindowHandleType::Default;

    if (!bgfx::init(init)) {
        MessageBoxA(
           nullptr,

           "BGFX initialization failed.",

           "BGFX Error",

           MB_OK
       );
        return 1;
    }
    g_bgfxInitialized = true;

    g_objectManager = new MyAnimation::ObjectManager();
    auto cyan_code = 0xFFFFFF00;
    auto red_code = 0xFF0000FF;
    auto yellow_code = 0xFF00FF00;
    std::array<std::uint32_t, 5> pyr_colors = {cyan_code, red_code, cyan_code, red_code, yellow_code};
    const bx::Vec3 pyramidOrigin(-4.0f, 0.0f, 0.0f);
    constexpr float pyramidWidth = 1.0f;
    constexpr float pyramidHeight = 1.5f;
    // The centroid of a solid pyramid is one-quarter of its height above the base.
    const bx::Vec3 pyramidCenter(
        pyramidOrigin.x + pyramidWidth * 0.5f,
        pyramidOrigin.y + pyramidHeight * 0.25f,
        pyramidOrigin.z + pyramidWidth * 0.5f);
    g_objectManager->addSquarePyramid(
        pyramidOrigin,
        pyr_colors,
        pyramidWidth, pyramidHeight);

    // Take care of Shaders
    auto debugCpy = &g_exec_parent_path;
    std::string parent_as_string = g_exec_parent_path.string();


    auto Program = ldProgram(
        parent_as_string + std::string("\\shaders\\vs.bin"),
        parent_as_string + std::string("\\shaders\\fs.bin")
        );

    if (!bgfx::isValid(Program)) {
        MessageBoxA(nullptr, "Could not load shaders\n",
            "Shader Error", MB_OK);
        delete g_objectManager;
        g_objectManager = nullptr;
        g_bgfxInitialized = false;
        bgfx::shutdown();
        return 1;
    }

    // View Clear
    bgfx::setViewClear(0,
        BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        0x202030ff, 1.0f, 0);
    float delta = 0.0f;
    enum Direction { DECREASE = 0, INCREASE = 1} cubeDirection = INCREASE;
    auto previousFrame = std::chrono::steady_clock::now();
    g_objectManager->bgfxUseBufObjs(0);

    while (g_running) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!g_running) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        // Limit movement after a stall, such as dragging or resizing the window.
        const float deltaTime = std::min(
            std::chrono::duration<float>(now - previousFrame).count(), 0.05f);
        previousFrame = now;

        if (g_width == 0 || g_height == 0) {
            Sleep(10);
            continue;
        }


        const float yawInput = float(g_kbd_right) - float(g_kbd_left);
        const float pitchInput = float(g_kbd_up) - float(g_kbd_down);
        cameraYaw += yawInput * turnSpeed * deltaTime;
        cameraPitch += pitchInput * turnSpeed * deltaTime;
        cameraPitch = std::clamp(cameraPitch, -pitchLimit, pitchLimit);
        const float cosPitch = std::cos(cameraPitch);
        const bx::Vec3 cameraForward(
            cosPitch * std::cos(cameraYaw),
            std::sin(cameraPitch),
            -cosPitch * std::sin(cameraYaw));
        const float forwardInput = float(g_kbd_w) - float(g_kbd_s);
        const float rightInput = float(g_kbd_d) - float(g_kbd_a);
        const float upInput = float(g_kbd_spc) - float(g_kbd_shft);
        // Fixed world axes: W/S = +X/-X, A/D = +Z/-Z.
        const bx::Vec3 movement(forwardInput, 0.0f, -rightInput);
        if (bx::dot(movement, movement) > 0.0f) {
            // Keep diagonal WASD movement at the same speed as a single direction.
            user_position = bx::add(user_position,
                bx::mul(bx::normalize(movement), cameraSpeed * deltaTime));
        }
        // Space/Shift move along world Y independently of the camera orientation.
        user_position.y += upInput * cameraSpeed * deltaTime;

        tmp_rotAngle += 0.02f;
        if (cubeDirection == DECREASE) {
            if (delta > -0.80f) {
                delta -= 0.02f;
            }
            else {
                cubeDirection = INCREASE;
                delta += 0.02f;
            }
        }
        else if (cubeDirection == INCREASE) {
            if (delta < 0.80f) {
                delta += 0.02f;
            }
            else {
                cubeDirection = DECREASE;
                delta -= 0.02f;
            }
        }

        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(g_width), static_cast<uint16_t>(g_height));

        // Camera
        float view[16];
        const bx::Vec3 eye = {
            user_position.x,
            user_position.y,
            user_position.z
        };

        const bx::Vec3 at = bx::add(eye, cameraForward);

        bx::mtxLookAt(view, eye, at, cameraUp);


        // Projection

        float projection[16];
        const float aspect = g_width / static_cast<float>(g_height);
        bx::mtxProj(projection,
            60.0f, aspect,
            0.1f, 100.0f,
            bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, projection);

        // Model Matrix
        float model[16];
        bx::mtxSRT(model,
        /* scale */
        1.0f, 1.0f, 1.0f,
        /* rotation */
        0, 9.2 * tmp_rotAngle, 0,
        /* translation */
        4.0 * sin(3.0 * abs(tmp_rotAngle)), 2.0 * abs(sin(3.0 * abs(tmp_rotAngle) / 1.7)), 0.0f);
        // 0.0, 0.0, 0.0);
        // Rotate about the pyramid's center: p' = R * (p - center) + center.
        const bx::Vec3 rotatedCenter = bx::mul(pyramidCenter, model);
        model[12] = pyramidCenter.x - rotatedCenter.x;
        model[13] = pyramidCenter.y - rotatedCenter.y;
        model[14] = pyramidCenter.z - rotatedCenter.z;

        bgfx::setTransform(model);

        // Geometry
        g_objectManager->bgfxUseBufObjs(0);

        // Rendering State
        bgfx::setState(
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
            BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);

        // Finally, submit the program
        bgfx::submit(0, Program);



        bgfx::frame();
    }
    delete g_objectManager;
    g_objectManager = nullptr;
    bgfx::destroy(Program);
    g_bgfxInitialized = false;
    bgfx::shutdown();
    return 0;
}

static void init_exec_dir_path() {
    std::wstring buf(MAX_PATH, L'\0');
    DWORD path_len = GetModuleFileNameW(nullptr, buf.data(), buf.size());
    if (path_len == buf.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        buf.resize(32767);
        path_len = GetModuleFileNameW(nullptr, buf.data(), buf.size());
    }

    buf.resize(path_len);

    std::filesystem::path exec_path(buf);
    g_exec_parent_path = exec_path.parent_path();
    MessageBoxA(nullptr, exec_path.string().c_str(), "msg", MB_OK | MB_ICONERROR);
}

static std::string wstr_to_str(std::wstring wstr) {
    ;
}