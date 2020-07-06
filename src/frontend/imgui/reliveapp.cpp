#include <backend/logging.hpp>
#include <backend/relivedb.hpp>
#include <backend/player.hpp>
#include <backend/system.hpp>
#include <version/version.hpp>
#include <ghc/options.hpp>
#include <RtAudio.h>
#include <GLFW/glfw3.h>
#include <imguix/application.h>
//#include <imguix/logging.h>
#include <imguix/imguix.h>
#include <resources/feather_icons.h>

#include <iostream>

using namespace std::literals;

#define RELIVE_APP_NAME "reLiveG"

namespace relive {


class ReLiveApp : public ImGui::Application
{
    const float FONT_SIZE = 16;
    const float DISPLAY_FONT_SIZE = 18;
    enum CurrentPage { pSTATIONS, pSTREAMS, pTRACKS, pCHAT };

public:
    ReLiveApp()
    : ImGui::Application("reLiveG v1.0 - \u00a9 2020 by Gulrak", "reLiveG")
    {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        setWindowSize(static_cast<unsigned>(mode->width * 0.8), static_cast<unsigned>(mode->height * 0.8));
        //LogManager::instance()->defaultLevel(1);

    }

    ~ReLiveApp()
    {

    }

    void doSetup() override
    {
        auto& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(2, 2);
        // styleColorsCustom();
        // ImGuiIO& io = ImGui::GetIO();
        _propFont = addFontFromResourceTTF("Roboto-Medium.ttf", FONT_SIZE);
        static const ImWchar icons_ranges[] = {ICON_MIN_FTH, ICON_MAX_FTH, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        addFontFromResourceTTF("Feather.ttf", FONT_SIZE, &icons_config, icons_ranges);
        ImGuiIO& io = ImGui::GetIO();
        _monoFont = io.Fonts->AddFontDefault();
    }
    void doTeardown() override {}
    void handleInput(ImGuiIO& io) override {}

    void handleResize(float width, float height) override
    {
        _width = width;
        _height = height;
    }

    void renderFrame() override
    {
        renderMainWindow();
        if (_show_demo_window) {
            ImGui::ShowDemoWindow(&_show_demo_window);
        }
    }

    void renderStations(ImVec2 pageSize)
    {
        ImGui::BeginTable("StationsTable", 3, 0, pageSize);
        ImGui::TableSetupColumn("Station Name##22",200);
        ImGui::TableSetupColumn("Streams##22", 200);
        ImGui::TableSetupColumn("URL##22", 200);
        ImGui::TableAutoHeaders();
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Teststation");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("42");
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("https://example.com");
        /*
        for(const auto& station : _stations) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", station._name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%lu", station._streams.size());
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", station._webSiteUrl.c_str());
        }*/
        ImGui::EndTable();
    }

    void renderMainWindow()
    {
        auto style = ImGui::GetStyle();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(_width, _height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4);
        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::Button("Stations");
        ImGui::SameLine();
        ImGui::Button("Streams");
        ImGui::SameLine();
        ImGui::Button("Tracks");
        ImGui::SameLine();
        ImGui::Button("Chat");

        ImGui::Separator();

        auto pageSize = ImVec2(_width + 8, _height - 70);
        switch(_currentPage) {
            case pSTATIONS:
                renderStations(pageSize);
                break;
            default:
                break;
        }
        ImGui::Separator();

        if(_progress) {
            ImGui::ProgressBar(_progress / 100.0f, ImVec2(-1, 0));
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

private:
    float _width = 0;
    float _height = 0;
    bool _show_demo_window = false;
    std::mutex _mutex;
    ReLiveDB _rdb;
    int64_t _lastFetch;
    Player _player;
    ImFont* _propFont = nullptr;
    ImFont* _displayFont = nullptr;
    ImFont* _monoFont = nullptr;
    std::atomic_bool _needsRefresh;
    CurrentPage _currentPage = pSTATIONS;
    std::vector<Station> _stations;
    int64_t _activeStation = 0;
    int _progress = 45;
};

}


int main(int argc, char* argv[])
{
    using namespace relive;

    try {
        setAppName(RELIVE_APP_NAME);
        LogManager::setOutputFile(relive::dataPath() + "/" + appName() + ".log");
        LogManager::instance()->defaultLevel(3);
        if(isInstanceRunning()) {
            throw std::runtime_error("Instance already running.");
        }
        ghc::options parser(argc, argv);
        parser.onOpt({"-?", "-h", "--help"}, "Output this help text", [&](const std::string&){
          parser.usage(std::cout);
          exit(0);
        });
        parser.onOpt({"-v", "--version"}, "Show program version and exit.", [&](const std::string&){
          std::cout << "reLiveCUI " << RELIVE_VERSION_STRING_LONG << std::endl;
          exit(0);
        });
        parser.onOpt({"-l", "--list-devices"}, "Dump a list of found and supported output devices and exit.", [&](const std::string&){
          RtAudio audio;
          audio.showWarnings(false);
          unsigned int devices = audio.getDeviceCount();
          RtAudio::DeviceInfo info;
          for(unsigned int i=0; i<devices; ++i) {
              info = audio.getDeviceInfo( i );
              if(info.probed && info.outputChannels >= 2) {
                  std::cout << "'" << info.name << "', ";
                  std::cout << " maximum output channels = " << info.outputChannels << ", [";
                  std::string rates;
                  for(auto rate : info.sampleRates) {
                      if(!rates.empty()) {
                          rates += ", ";
                      }
                      rates += std::to_string(rate) + "Hz";
                  }
                  std::cout << rates << "]" << std::endl;
              }
          }
          exit(0);
        });
        parser.onOpt({"-s?", "--default-station?"}, "[<name>]\tSet the default station to switch to on startup, only significant part of the name is needed. Without a parameter, this resets to starting on station screen.", [&](std::string str){
          ReLiveDB db;
          if(str.empty()) {
              db.setConfigValue(Keys::default_station, "");
              std::cout << "Selected starting with stations screen." << std::endl;
          }
          else {
              auto stations = db.findStations("%"+str+"%");
              if(stations.empty()) {
                  std::cerr << "Sorry, no station matches the given name: '" << str << "'" << std::endl;
              }
              else if (stations.size() > 1) {
                  std::cerr << "Sorry, more than one station matches the given name: '" << str << "'" << std::endl;
                  for(const auto station : stations) {
                      std::cerr << "    '" << station._name << "'" << std::endl;
                  }
              }
              else {
                  db.setConfigValue(Keys::default_station, stations.front()._name);
                  std::cout << "Set '" << stations.front()._name << "' as the new default station." << std::endl;
              }
          }
          exit(0);
        });
        parser.parse();

        ReLiveApp app;
        app.run();
    }
    catch(std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        exit(1);
    }        return 0;
}

