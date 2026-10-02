include(FetchContent)

set(GODOTCPP_API_VERSION "4.7" CACHE STRING "" FORCE)
set(GODOTCPP_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json" CACHE PATH "" FORCE)

FetchContent_Declare(
    godot-cpp
    GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
    GIT_TAG 10.0.0-stable
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(godot-cpp)

add_library(remaster_core_bridge SHARED
    src/register_types.cpp
    src/remaster_core_bridge.cpp
)

target_include_directories(remaster_core_bridge
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(remaster_core_bridge
    PRIVATE
        godot::cpp
        remaster_core
)

target_compile_features(remaster_core_bridge PRIVATE cxx_std_17)

set_target_properties(remaster_core_bridge PROPERTIES
    CXX_EXTENSIONS OFF
)

if(ANDROID)
    if(NOT CMAKE_ANDROID_ARCH_ABI)
        set(CMAKE_ANDROID_ARCH_ABI "arm64-v8a")
    endif()
    set(REMASTER_BRIDGE_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/android/${CMAKE_ANDROID_ARCH_ABI}")
else()
    set(REMASTER_BRIDGE_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif()

add_custom_command(
    TARGET remaster_core_bridge
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${REMASTER_BRIDGE_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:remaster_core_bridge>"
        "${REMASTER_BRIDGE_RUNTIME_DIR}/$<TARGET_FILE_NAME:remaster_core_bridge>"
)
