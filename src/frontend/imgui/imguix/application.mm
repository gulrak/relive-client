#include "application.h"

#include "../thirdparty/imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <cstdio>

using namespace ImGui;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void glfw_resize_callback(GLFWwindow* window, int width, int height)
{
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onResize(width, height);
    std::cout << "Window-Size: " << width << "x" << height << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    //glViewport(0, 0, width, height);
    std::cout << "Framebuffer-Size: " << width << "x" << height << std::endl;
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
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    NSWindow* nswin = nullptr;
    CAMetalLayer* layer = nullptr;
    MTLRenderPassDescriptor* renderPassDescriptor = nullptr;
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
    //ImGuiIO& io = ImGui::GetIO();
    handleResize(float(width), float(height));
}

void Application::onRefresh()
{
    render();
}

void Application::setup()
{
    // Create window with graphics context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _impl->window = glfwCreateWindow(_impl->width, _impl->height, _impl->name.c_str(), NULL, NULL);
    if (_impl->window == NULL)
        return;

    glfwSetWindowTitle(_impl->window, _impl->title.c_str());
    glfwSetWindowSizeLimits(_impl->window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
    _impl->device = MTLCreateSystemDefaultDevice();
    _impl->commandQueue = [_impl->device newCommandQueue];

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    doSetup();

    ImGui_ImplGlfw_InitForOpenGL(_impl->window, true);
    ImGui_ImplMetal_Init(_impl->device);

    _impl->nswin = glfwGetCocoaWindow(_impl->window);
    _impl->layer = [CAMetalLayer layer];
    _impl->layer.device = _impl->device;
    _impl->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _impl->nswin.contentView.layer = _impl->layer;
    _impl->nswin.contentView.wantsLayer = YES;

    _impl->renderPassDescriptor = [MTLRenderPassDescriptor new];

    glfwSwapInterval(1);

    glfwSetWindowUserPointer(_impl->window, this);
    glfwSetWindowSizeCallback(_impl->window, glfw_resize_callback);
    glfwSetFramebufferSizeCallback(_impl->window, framebuffer_size_callback);
    handleResize(_impl->width, _impl->height);
    glfwSetWindowRefreshCallback(_impl->window, glfw_refresh_callback);

}

void Application::teardown()
{
    doTeardown();

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(_impl->window);
    glfwTerminate();
}

void Application::render()
{
    ImVec4 clear_color = ImGui::ColorConvertU32ToFloat4(_clearColor);
    int width, height;
    glfwGetFramebufferSize(_impl->window, &width, &height);
    _impl->layer.drawableSize = CGSizeMake(width, height);
    id<CAMetalDrawable> drawable = [_impl->layer nextDrawable];

    id<MTLCommandBuffer> commandBuffer = [_impl->commandQueue commandBuffer];
    _impl->renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    _impl->renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    _impl->renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    _impl->renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:_impl->renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"ImGui demo"];

    // Start the Dear ImGui frame
    ImGui_ImplMetal_NewFrame(_impl->renderPassDescriptor);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderFrame();

    // Rendering
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

void Application::quit()
{
    glfwSetWindowShouldClose(_impl->window, GLFW_TRUE);
}

void Application::run()
{
    using namespace std::chrono_literals;
    setup();

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;

    // Main loop
    while (!glfwWindowShouldClose(_impl->window)) {
        @autoreleasepool {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            render();

            std::this_thread::sleep_for(10ms);
        }
    }

    teardown();
}
