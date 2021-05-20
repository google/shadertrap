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


help | head

uname

if [ "$(uname)" != "Linux" ]
then
    echo "Unsupported OS"
    exit 1
fi

echo "deb-src http://gb.archive.ubuntu.com/ubuntu/ focal main restricted" | sudo tee -a /etc/apt/sources.list
sudo DEBIAN_FRONTEND=noninteractive apt-get -qy update

NINJA_OS="linux"
# Needed to get EGL
sudo DEBIAN_FRONTEND=noninteractive apt-get -qy install libegl1-mesa-dev

# Remove this to avoid clashes with the dependencies that ANGLE installs
sudo DEBIAN_FRONTEND=noninteractive apt-get -qy remove php7.4-common

df -h
sudo swapoff -a
sudo rm -f /swapfile
sudo apt clean
# shellcheck disable=SC2046
docker rmi $(docker image ls -aq)
df -h

pushd "${HOME}"
  export PATH="${HOME}/depot_tools":${PATH}
  git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools

  git clone https://chromium.googlesource.com/angle/angle angle
  pushd angle
    python --version
    python scripts/bootstrap.py
    gclient sync
    git checkout master
    sudo ./build/install-build-deps.sh
    gn gen out/Debug --args="is_debug=true angle_enable_vulkan=true angle_enable_swiftshader=true angle_enable_hlsl=false angle_enable_d3d9=false angle_enable_d3d11=false angle_enable_gl=false angle_enable_null=false angle_enable_metal=false"
    autoninja -C out/Debug libEGL libGLESv2
    pushd out/Debug
      cp libEGL.so libEGL.so.1
    popd
  popd
popd

export SHADERTRAP_SKIP_BASH=1

source ./dev_shell.sh.template

cd ${SHADERTRAP_REPO_ROOT}

pushd temp
  mkdir -p "build-Debug/"
  pushd "build-Debug/"
    cmake \
      -G Ninja \
      ../.. \
      -DCMAKE_BUILD_TYPE=Debug \
      -DSHADERTRAP_USE_LLVM_LIBCPP=1
    cmake --build . --config Debug
  popd
popd

for f in `find ./examples/GLES31 -name "*.shadertrap"`
do
    env VK_ICD_FILENAMES=${HOME}/angle/out/Debug/vk_swiftshader_icd.json ANGLE_DEFAULT_PLATFORM=vulkan LD_LIBRARY_PATH=${HOME}/angle/out/Debug/ ./temp/build-Debug/src/shadertrap/shadertrap $f --require-vendor-renderer-substring SwiftShader --show-gl-info
done
