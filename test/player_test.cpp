//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------

#include <backend/logging.hpp>
#include <backend/player.hpp>
#include <backend/system.hpp>
#include <ghc/options.hpp>
#include <ghc/uri.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main(int argc, char* argv[])
{
    try {
        relive::LogManager::instance()->defaultLevel(4);
        Player player;
        ghc::options parser(argc, argv);
        std::vector<ghc::net::uri> uris;
        parser.onOpt({"-?", "-h", "--help"}, "Output this help text", [&](const std::string&){
          parser.usage(std::cout);
          exit(0);
        });
        parser.onPositional("URI to play", [&](const std::string& arg){ uris.emplace_back(arg); });
        parser.parse();

        if(!uris.empty()) {
            player.setSource(Player::eFile, uris.front());
            player.volume(10);
            player.play();
            while (player.state() != eENDOFSTREAM) {
                std::cout << "time: " << relive::formattedDuration(player.playTime()) << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    catch(std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        exit(1);
    }
    return 0;
}
