#pragma once

#include <backend/system.hpp>
namespace fs = ghc::filesystem;

enum class TempOpt { none, change_path };
class TemporaryDirectory
{
public:
    TemporaryDirectory(TempOpt opt = TempOpt::none)
    {
        static auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        static auto rng = std::bind(std::uniform_int_distribution<int>(0, 35), std::mt19937(static_cast<unsigned int>(seed) ^ static_cast<unsigned int>(reinterpret_cast<ptrdiff_t>(&opt))));
        std::string filename;
        do {
            filename = "test_";
            for (int i = 0; i < 8; ++i) {
                filename += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rng()];
            }
            _path = fs::canonical(fs::temp_directory_path()) / filename;
        } while (fs::exists(_path));
        fs::create_directories(_path);
        if (opt == TempOpt::change_path) {
            _orig_dir = fs::current_path();
            fs::current_path(_path);
        }
    }
    
    ~TemporaryDirectory()
    {
        if (!_orig_dir.empty()) {
            fs::current_path(_orig_dir);
        }
        fs::remove_all(_path);
    }
    
    const fs::path& path() const { return _path; }
    
private:
    fs::path _path;
    fs::path _orig_dir;
};


