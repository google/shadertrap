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

# Needed to get Mesa build dependencies
sudo DEBIAN_FRONTENT=noninteractive apt-get -qy build-dep mesa

# Mesa requires Mako
python -m pip install Mako

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

mkdir -p "${HOME}/mesa"
pushd "${HOME}/mesa"
  # Get and build mesa
  curl -fsSL -o mesa-src.tar.xf "https://archive.mesa3d.org/mesa-21.1.0.tar.xz"
  tar xf mesa-src.tar.xf
  ls
  pushd mesa-21.1.0
    mkdir build
    pushd build
      meson -D prefix=/usr -D egl=enabled -D gles1=enabled -D gles2=enabled -D dri-drivers=auto -D vulkan-drivers="" -D gallium-drivers=swrast -D glx=dri
      ninja
    popd
    mkdir dynamiclibs
    pushd dynamiclibs
      for suffix in "" ".1" ".2"
      do
	  for f in `find ../build -name "*.so${suffix}"`
	  do
	      cp -s $f .
	  done
      done
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

env LD_LIBRARY_PATH=${HOME}/mesa/mesa-21.1.0/dynamiclibs ./temp/build-Debug/src/shadertrap/shadertrap ./examples/cube.shadertrap --show_gl_info
