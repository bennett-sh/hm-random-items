cmake_minimum_required(VERSION 3.15)

project(RandomItems CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
# Set ZHMMODSDK_DIR variable to a local directory to use a local copy of the ZHMModSDK.
set(ZHMMODSDK_VER "v3.0.0")
include(cmake/setup-zhmmodsdk.cmake)

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

# Create the RandomItems mod library.
add_library(RandomItems SHARED
    src/RandomItems.cpp
    src/RandomItems.h
)

# Set UTF-8 flag.
set_target_properties(RandomItems PROPERTIES COMPILE_FLAGS "/utf-8")

# Add SDK and DX headers dependencies.
find_package(directx-headers CONFIG REQUIRED)

target_link_libraries(RandomItems
    ZHMModSDK
    Microsoft::DirectX-Guids
    Microsoft::DirectX-Headers
)

install(TARGETS RandomItems
    RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
# zhmmodsdk_install(RandomItems)
