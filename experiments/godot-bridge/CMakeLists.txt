include(FetchContent)

set(GODOTCPP_API_VERSION "4.7" CACHE STRING "" FORCE)
set(GODOTCPP_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    godot-cpp
    GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
    GIT_TAG 507ed9d840c01a3c5b2a39af8bb4000bfac30bf5
    GIT_SHALLOW FALSE
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

add_custom_command(
    TARGET remaster_core_bridge
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:remaster_core_bridge>"
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:remaster_core_bridge>"
)
