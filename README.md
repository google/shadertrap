# ShaderTrap

An OpenGL shader runner that runs self-contained `.shadertrap` files.

This is not an officially supported Google product.

## Build

```sh
# Clone the repo.
git clone git@github.com:google/shadertrap.git
# Or:
git clone https://github.com/google/shadertrap.git

# Navigate to the root of the repo.
cd shadertrap

# Update and init submodules.
git submodule update --init

# Build using a recent version of CMake. Ensure `ninja` is on your PATH.
mkdir build
cd build

cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
cmake -DCMAKE_INSTALL_PREFIX=./install -P cmake_install.cmake --config Debug
```
