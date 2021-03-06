#!/usr/bin/env bash

# Copyright 2021 The ShaderTrap Project Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -x
set -e
set -u

# Old bash versions can't expand empty arrays, so we always include at least this option.
CMAKE_OPTIONS=("-DCMAKE_OSX_ARCHITECTURES=x86_64")

help | head

uname

case "$(uname)" in
"Linux")
  NINJA_OS="linux"
  # Needed to get EGL
  sudo DEBIAN_FRONTEND=noninteractive apt-get -qy install libegl1-mesa-dev
  df -h
  sudo apt clean
  # shellcheck disable=SC2046
  docker rmi -f $(docker image ls -aq)
  sudo rm -rf /usr/share/dotnet /usr/local/lib/android /opt/hostedtoolcache/boost /opt/ghc
  df -h

  # Provided by build.yml.
  export CC="${LINUX_CC}"
  export CXX="${LINUX_CXX}"

  ;;

"Darwin")
  NINJA_OS="mac"
  brew install md5sha1sum
  # Needed to get EGL
  mkdir -p "${HOME}/angle-release"
  pushd "${HOME}/angle-release"
    curl -fsSL -o angle-release.zip https://github.com/paulthomson/build-angle/releases/download/v-592879ad24e66c7c68c3a06d4e2227630520da36/Darwin-x64-Release.zip
    unzip angle-release.zip
    ls
  popd
  export CMAKE_PREFIX_PATH="${HOME}/angle-release"
  ;;

"MINGW"*|"MSYS_NT"*)
  NINJA_OS="win"
  # Needed to get EGL
  mkdir -p "${HOME}/angle-release"
  pushd "${HOME}/angle-release"
    curl -fsSL -o angle-release.zip https://github.com/paulthomson/build-angle/releases/download/v-592879ad24e66c7c68c3a06d4e2227630520da36/MSVC2015-Release-x64.zip
    unzip angle-release.zip
    ls
  popd
  export CMAKE_PREFIX_PATH="${HOME}/angle-release"

  CMAKE_OPTIONS+=("-DCMAKE_C_COMPILER=cl.exe" "-DCMAKE_CXX_COMPILER=cl.exe")
  ;;

*)
  echo "Unknown OS"
  exit 1
  ;;
esac

export PATH="${HOME}/bin:$PATH"
mkdir -p "${HOME}/bin"
pushd "${HOME}/bin"
  # Install ninja.
  curl -fsSL -o ninja-build.zip "https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-${NINJA_OS}.zip"
  unzip ninja-build.zip
  ls
popd



case "$(uname)" in
"Linux")

  # On Linux, source the dev shell to download clang-tidy and other tools.
  # Developers should *run* the dev shell, but we want to continue executing this script.
  export SHADERTRAP_SKIP_COMPILER_SET=1
  export SHADERTRAP_SKIP_ANDROID_NDK=1
  export SHADERTRAP_SKIP_BASH=1

  source ./dev_shell.sh.template

  ;;

"Darwin")
  ;;

"MINGW"*|"MSYS_NT"*)
  ;;

*)
  echo "Unknown OS"
  exit 1
  ;;
esac




mkdir -p build
pushd build
  cmake -G Ninja .. -DCMAKE_BUILD_TYPE="${CONFIG}" "${CMAKE_OPTIONS[@]}"
  cmake --build . --config "${CONFIG}"
  cmake -DCMAKE_INSTALL_PREFIX=./install -DBUILD_TYPE="${CONFIG}" -P cmake_install.cmake
  # Run the unit tests
  ./src/libshadertraptest/libshadertraptest
popd


case "$(uname)" in
"Linux")
  # On Linux, run a few extra analyzes using the compile_commands.json file.
  check_compile_commands.sh build/compile_commands.json
  ;;

"Darwin")
  ;;

"MINGW"*|"MSYS_NT"*)
  ;;

*)
  echo "Unknown OS"
  exit 1
  ;;
esac
