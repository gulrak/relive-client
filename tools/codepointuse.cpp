//
// Created by Steffen Schümann on 08.08.20.
//

#include <backend/relivedb.hpp>
#include <backend/system.hpp>
#include <ghc/filesystem.hpp>
#include <ghc/options.hpp>
#include <version/version.hpp>
#include "unicode-blocks.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <imgui/imstb_truetype.h>
#include <algorithm>
#include <set>
#include <string>

namespace fs = ghc::filesystem;

static std::set<BlockInfo> usedBlocks;
static std::set<uint32_t> usedCPs;
static std::multimap<int, std::string> ranking;

const BlockInfo& blockForCodepoint(uint32_t cp)
{
    static const BlockInfo dummy{0, 0, ""};
    auto iter = std::lower_bound(blocks.begin(), blocks.end(), BlockInfo{cp, cp, ""}, [](const BlockInfo& b1, const BlockInfo& b2) { return b1.to < b2.to; });
    if (iter != blocks.end()) {
        if (iter->from <= cp) {
            return *iter;
        }
    }
    return dummy;
}

void registerCodeBlocks(const std::string& text, const std::string& comment = std::string())
{
    auto u32str = fs::detail::fromUtf8<std::u32string>(text);
    for (auto cp : u32str) {
        usedCPs.insert(cp);
        if(cp >= 0x2000) {
            std::cout << "[0x" << std::hex << cp << std::dec << " " << (comment.empty() ? text : comment) << "]"  << std::endl;
        }
        auto block = blockForCodepoint(cp);
        if (!block.name.empty()) {
            usedBlocks.insert(block);
        }
    }
}

void dumpUsedBlocks()
{
    std::cout << "Used unicode blocks:" << std::endl;
    for (const auto& block : usedBlocks) {
        std::cout << "    0x" << std::hex << block.from << ", 0x" << block.to << ", // " << block.name << std::endl;
    }
    std::cout << std::dec;
}

int findMissingGlyphs(stbtt_fontinfo& font)
{
    int missing = 0;
    for(auto cp : usedCPs) {
        if(stbtt_FindGlyphIndex(&font, cp) == 0) {
            ++missing;
        }
    }
    return missing;
}

void dumpRanges(stbtt_fontinfo& font)
{
    uint32_t cp = 1;
    while(cp < 0x10ffffu) {
        while(cp < 0x10ffffu && stbtt_FindGlyphIndex(&font, cp) == 0) ++cp;
        uint32_t  start = cp++;
        while(cp < 0x10ffffu && stbtt_FindGlyphIndex(&font, cp) != 0) ++cp;
        std::cout << "    0x" << std::hex << start << ", 0x" << (cp - 1) << "," << std::endl;
    }
}

int main(int argc, char* argv[])
{
    relive::setAppName("codepointuse");
    fs::u8arguments(argc, argv);
    ghc::options parser(argc, argv);
    bool datapathGiven = false;
    std::vector<fs::path> fontDirs;
    parser.onOpt({"-?", "-h", "--help"}, "Output this help text", [&](const std::string&) {
        parser.usage(std::cout);
        exit(0);
    });
    parser.onOpt({"-v", "--version"}, "Show program version and exit.", [&](const std::string&) {
        std::cout << "codepointuse " << RELIVE_VERSION_STRING_LONG << std::endl;
        exit(0);
    });
    parser.onOpt({"-d!", "--data-path"}, "database directory to use", [&](const std::string& file) {
        if (!fs::exists(fs::path(file) / "relive.sqlite")) {
            std::cerr << "Error: there is no 'relive.sqlite' in '" << file << "'" << std::endl;
            std::exit(1);
        }
        relive::dataPath(file);
        datapathGiven = true;
    });
    parser.onOpt({"-f!", "--font-dir"}, "directory to scan for fonts (can be given multiple times)", [&](const std::string& fontDir) {
        if((fs::exists(fontDir) && fs::is_directory(fontDir))) {
            fontDirs.push_back(fontDir);
        }
        else {
            std::cerr << "Error: font dir '" << fontDir << "' does not exist" << std::endl;
            std::exit(1);
        }
    });
    parser.onOpt({"--dump-ranges!"}, "dump list of glyph ranges of given font", [&](const std::string& fontPath) {
      std::vector<char> buffer;
      auto size = fs::file_size(fontPath);
      buffer.resize(size);
      fs::ifstream is(fs::path(fontPath), std::ifstream::in | std::ifstream::binary);
      is.read(&buffer.front(), size);
        auto numFonts = stbtt_GetNumberOfFonts((const unsigned char*)buffer.data());
        if(fs::exists(fontPath) && fs::is_regular_file(fontPath)) {
            for(int idx = 0; idx < numFonts; ++idx) {
                stbtt_fontinfo font;
                stbtt_InitFont(&font, (const unsigned char*)buffer.data(), stbtt_GetFontOffsetForIndex((const unsigned char*)buffer.data(), idx));
                std::cout << "     " << idx << ": " << font.numGlyphs << " glyphs";
                dumpRanges(font);
            }
        }
    });
    parser.parse();
    if(datapathGiven) {
        relive::ReLiveDB rdb;
        auto stations = rdb.fetchStations();
        std::cout << "Scanning..." << std::endl;
        for (auto station : stations) {
            std::cout << "    " << station._name << std::endl;
            registerCodeBlocks(station._name);
            registerCodeBlocks(station._webSiteUrl);
            rdb.deepFetch(station);
            for (auto stream : station._streams) {
                // std::cout << "    " << stream._host << ": " << stream._name << std::endl;
                registerCodeBlocks(stream._host, station._name);
                registerCodeBlocks(stream._name, station._name);
                rdb.deepFetch(stream);
                for (auto& track : stream._tracks) {
                    // std::cout << "        " << track._artist << ": " << track._name << std::endl;
                    registerCodeBlocks(track._artist, stream._name);
                    registerCodeBlocks(track._name, stream._name);
                }
            }
        }
        std::cout << "-------" << std::endl;
        for(auto cp : usedCPs) {
            std::cout << std::hex << "0x" << cp << ", ";
        }
        std::cout << std::endl;
        dumpUsedBlocks();
        std::cout << "In total: " << usedCPs.size() << " codepoints used" << std::endl;
    }
    if(!fontDirs.empty()) {
        for(const auto& dir : fontDirs) {
            for(const  auto& entry : fs::recursive_directory_iterator(dir)) {
                if(entry.is_regular_file() &&
                    (entry.path().extension() == ".ttf" ||
                    entry.path().extension() == ".ttc")) {
                    std::vector<char> buffer;
                    auto size = entry.file_size();
                    buffer.resize(size);
                    fs::ifstream is(entry.path(), std::ifstream::in | std::ifstream::binary);
                    is.read(&buffer.front(), size);
                    auto numFonts = stbtt_GetNumberOfFonts((const unsigned char*)buffer.data());
                    if(numFonts>0) {
                        if(numFonts == 1) {
                            stbtt_fontinfo font;
                            stbtt_InitFont(&font, (const unsigned char*)buffer.data(), stbtt_GetFontOffsetForIndex((const unsigned char*)buffer.data(), 0));
                            std::cout << "Font " << entry.path().filename() << ", " << font.numGlyphs << " glyphs";
                            if(datapathGiven) {
                                auto missing = findMissingGlyphs(font);
                                std::cout << ", missing " << missing << " glyphs";
                                ranking.insert(std::make_pair(missing, entry.path().string()));
                            }
                            std::cout << std::endl;
                        }
                        else {
                            std::cout << "Font " << entry.path().filename() << std::endl;
                            for(int idx = 0; idx < numFonts; ++idx) {
                                stbtt_fontinfo font;
                                stbtt_InitFont(&font, (const unsigned char*)buffer.data(), stbtt_GetFontOffsetForIndex((const unsigned char*)buffer.data(), idx));
                                std::cout << "     " << idx << ": " << font.numGlyphs << " glyphs";
                                if(datapathGiven) {
                                    auto missing = findMissingGlyphs(font);
                                    std::cout << ", missing " << missing << " glyphs";
                                    ranking.insert(std::make_pair(missing, entry.path().string() + ":" + std::to_string(idx)));
                                }
                                std::cout << std::endl;
                            }
                        }
                    }
                }
            }
        }
        int topTen = 0;
        for(const auto& p : ranking) {
            std::cout << p.first << ": " << p.second << std::endl;
            if(++topTen == 10) {
                break;
            }
        }
    }
    return 0;
}
