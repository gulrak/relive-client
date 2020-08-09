# reLive-Clients

_Live show replay clients for experiencing past scene-related shows and podcasts from [reLive](http://relive.nu)._

![Cirrus-CI macOS](https://api.cirrus-ci.com/github/gulrak/relive-client.svg?branch=master&task=macOS)
![Cirrus-CI Linux](https://api.cirrus-ci.com/github/gulrak/relive-client.svg?branch=master&task=Ubuntu)

This repository collects my current work on reLive client software. It is a full rework of the original code I wrote for [reLiveQt](https://relive.gulrak.net/reliveqt).

Originally there was only a curses based client, named **reLiveCUI**, developed and tested on macOS and Ubuntu 18.04,
but starting with the `dev-reliveg-wip` branch, the work on a new graphical client named **reLiveG** has begun.

# reLiveCUI

## Usage:

* Use the function keys to switch screens, cursor up/down to select and/or scroll,
  return to activate something. You need to select a station to get a stream list,
  you need to select a stream to see a track list.
* Page-up/page-down can be used to skip through a list/text.
* Space is used for play/pause
* Volume is currently not shown, but can be changed with v/V.


### Features still missing but planned

* Mouse support
* Configuration screen
* Database search
* Internet radio streaming
* relive: schema urls

### Screenshots

These screenshots where takon on an Ubuntu 18.04 system:

![Stream View](https://github.com/gulrak/relive-client/blob/master/screenshots/relivecui-linux-streams_v0.3.33.png?raw=true)
![Chat View](https://github.com/gulrak/relive-client/blob/master/screenshots/relivecui-linux-chat_v0.3.33.png?raw=true)

## Legal Info

**reLiveCUI v0.5**<br>
_Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com><br>
All rights reserved._

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----

## This program uses the following libraries

https://github.com/yhirose/cpp-httplib

A C++ header-only HTTP/HTTPS server and client library (MIT license)

----

https://github.com/gulrak/filesystem

An implementation of C++17 std::filesystem for C++11 /C++14/C++17 on Windows, macOS and Linux. (MIT license)

----

https://github.com/lieff/minimp3

Minimalistic MP3 decoder single header library (Public Domain)

----

https://github.com/nlohmann/json

JSON for Modern C++ (MIT license)

----

https://github.com/fnc12/sqlite_orm

SQLite ORM light header only library for modern C++ (BSD-3-Clause license)

----

http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/

A threadpool implementation for C++14 (BSD-2-Clause license)

----

# reLiveG

## Legal Info

**reLiveCUI v0.5**<br>
_Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com><br>
All rights reserved._

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----

## This program uses the following libraries

https://github.com/ocornut/imgui

A bloat-free graphical user interface library for C++ (MIT license)

----

https://github.com/yhirose/cpp-httplib

A C++ header-only HTTP/HTTPS server and client library (MIT license)

----

https://github.com/gulrak/filesystem

An implementation of C++17 std::filesystem for C++11 /C++14/C++17 on Windows, macOS and Linux. (MIT license)

----

https://github.com/lieff/minimp3

Minimalistic MP3 decoder single header library (Public Domain)

----

https://github.com/nlohmann/json

JSON for Modern C++ (MIT license)

----

https://github.com/fnc12/sqlite_orm

SQLite ORM light header only library for modern C++ (BSD-3-Clause license)

----

http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/

A threadpool implementation for C++14 (BSD-2-Clause license)

----
