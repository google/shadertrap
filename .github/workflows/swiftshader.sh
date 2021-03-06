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

df -h
sudo swapoff -a
sudo rm -f /swapfile
sudo apt clean
# shellcheck disable=SC2046
docker rmi $(docker image ls -aq)
df -h

export PATH="${HOME}/bin:$PATH"
mkdir -p "${HOME}/bin"
pushd "${HOME}/bin"
  # Install ninja.
  curl -fsSL -o ninja-build.zip "https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-${NINJA_OS}.zip"
  unzip ninja-build.zip
  ls
popd

mkdir "${HOME}/angle"
pushd "${HOME}/angle"
  # Get pre-built ANGLE
  curl -fsSL -o angle.zip "https://github.com/google/gfbuild-angle/releases/download/github%2Fgoogle%2Fgfbuild-angle%2F96ab6566490495f472cd239997701b201c7a48ac/gfbuild-angle-96ab6566490495f472cd239997701b201c7a48ac-Linux_x64_Debug.zip"
  unzip angle.zip
  # ShaderTrap looks for libEGL.so.1, so make a copy of the EGL .so with that name.
  cp ./lib/libEGL.so ./lib/libEGL.so.1
  ls
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
    env VK_ICD_FILENAMES=${HOME}/angle/lib/vk_swiftshader_icd.json ANGLE_DEFAULT_PLATFORM=vulkan LD_LIBRARY_PATH=${HOME}/angle/lib/ ./temp/build-Debug/src/shadertrap/shadertrap $f --require-vendor-renderer-substring SwiftShader --show-gl-info
done
