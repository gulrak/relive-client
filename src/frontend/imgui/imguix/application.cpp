
#include "application.h"

#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>  // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>  // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

#include <GLFW/glfw3.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

using namespace ImGui;


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void glfw_resize_callback(GLFWwindow* window, int width, int height)
{
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onResize(width, height);
}

void glfw_refresh_callback(GLFWwindow* window)
{
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onRefresh();
}

struct Application::Private
{
    std::string title;
    std::string name;
    GLFWwindow* window = nullptr;
    unsigned width = 0;
    unsigned height = 0;
};

Application::Application(const std::string& title, const std::string& name)
: _impl(new Application::Private)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;

    _impl->title = title;
    _impl->name = name.empty() ? title : name;
    _impl->width = MIN_WIDTH;
    _impl->height = MIN_HEIGHT;
}

Application::~Application()
{
    // todo
}

void Application::setWindowSize(unsigned width, unsigned height)
{
    _impl->width = width;
    _impl->height = height;
}

void Application::onResize(unsigned width, unsigned height)
{
    _impl->width = width;
    _impl->height = height;
    handleResize(float(width), float(height));
}

void Application::onRefresh()
{
    render();
}

void Application::setup()
{
    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    _impl->window = glfwCreateWindow(int(_impl->width), int(_impl->height), _impl->name.c_str(), nullptr, nullptr);
    if (_impl->window == nullptr)
        return;
    glfwSetWindowTitle(_impl->window, _impl->title.c_str());
    glfwSetWindowSizeLimits(_impl->window, MIN_WIDTH, MIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(_impl->window);
    glfwSwapInterval(1);  // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
    bool err = false;  // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(_impl->window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glfwSetWindowUserPointer(_impl->window, this);
    glfwSetWindowSizeCallback(_impl->window, glfw_resize_callback);
    handleResize(_impl->width, _impl->height);
    glfwSetWindowRefreshCallback(_impl->window, glfw_refresh_callback);

    _clearColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.45f, 0.55f, 0.60f, 1.00f));
    doSetup();
}

void Application::teardown()
{
    doTeardown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(_impl->window);
    glfwTerminate();
}

void Application::render()
{
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    auto start = std::chrono::steady_clock::now();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderFrame();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(_impl->window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    auto renderTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);
    _renderTime_us = _renderTime_us * 0.95f + renderTime.count() * 0.05f;

    glfwSwapBuffers(_impl->window);
}

void Application::run()
{
    using namespace std::chrono_literals;
    setup();

    std::chrono::system_clock::time_point lastTime = std::chrono::system_clock::now();
    int64_t dt = 0;
    // Main loop
    while (!glfwWindowShouldClose(_impl->window)) {

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureKeyboard) {
            handleInput(io);
        }

        render();

        //std::this_thread::sleep_for(14ms);

        /*
        std::cout << dt << std::endl;
        auto now = std::chrono::system_clock::now();
        dt = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count();
        if(dt < 16500) {
            std::this_thread::sleep_for(std::chrono::microseconds(16500 - dt));
        }
        lastTime = now;
         */
    }

    teardown();
}

