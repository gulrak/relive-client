#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imguix/applog.h>
#include <imguix/resources.h>

#include <memory>
#include <string>

namespace ImGui {

class Application
{
public:
    const unsigned MIN_WIDTH = 800;
    const unsigned MIN_HEIGHT = 500;
    Application(const std::string& title, const std::string& name = std::string());
    virtual ~Application();

    void run();

    void setWindowSize(unsigned width, unsigned height);
    void quit();
    void onResize(unsigned width, unsigned height);
    void onRefresh();
    void setClearColor(ImU32 col) { _clearColor = col; }
    ImU32 clearColor() const { return _clearColor; }

    uint32_t renderTime() const { return uint32_t(_renderTime_us); }
    ImFont* addFontFromResourceTTF(std::string name, float size_pixels, const ImFontConfig* font_cfg = nullptr, const ImWchar* glyph_ranges = nullptr)
    {
        auto res = ResourceManager::instance().resourceForName(name);
        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig config;
        if(font_cfg) {
            config = *font_cfg;
        }
        else {
            config.PixelSnapH = true;
        }
        config.FontDataOwnedByAtlas = false;
        return io.Fonts->AddFontFromMemoryTTF((void*)res.data(), res.size(), size_pixels, &config, glyph_ranges);
    }

protected:
    virtual void doSetup() {}
    virtual void doTeardown() {}
    virtual void handleInput(ImGuiIO& io) {}
    virtual void renderFrame() = 0;
    virtual void handleResize(float width, float height) {}
private:
    void setup();
    void teardown();
    void render();

    struct Private;
    std::unique_ptr<Private> _impl;
    ImU32 _clearColor = 0;
    float _renderTime_us = 0;
};

}
