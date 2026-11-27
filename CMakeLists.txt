cmake_minimum_required(VERSION 3.20)

# Build tested on MSVC 2016 x64

project(streamdeck-discordmixer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

option(BUILD_IN_STREAMDECK_PLUGINS_DIR "Whether to build to the 'bin' directory or to the stream deck plugins directory." OFF)

# ===========================================
# Target setup
# ===========================================
set(target streamdeck-discordmixer)

# Qt setup
find_package(Qt6 COMPONENTS Core WebSockets Network Gui REQUIRED CONFIG)
set(CMAKE_AUTOMOC ON)

if (BUILD_IN_STREAMDECK_PLUGINS_DIR)
    file(TO_CMAKE_PATH "$ENV{APPDATA}" appdata_path)
    set(target_directory "${appdata_path}/Elgato/StreamDeck/Plugins/com.farissoze.discordvoicemixerfix.sdPlugin")
else ()
    set(target_directory "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/com.farissoze.discordvoicemixerfix.sdPlugin")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${target_directory}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${target_directory}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${target_directory}/bin")
qt_add_executable(${target})
target_link_libraries(${target} PRIVATE Qt6::WebSockets Qt6::Network Qt6::Gui)
target_compile_definitions(${target} PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x060200)

# ===========================================
# Sources
# ===========================================
file(GLOB_RECURSE source_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
target_sources(${target} PRIVATE ${source_files})
target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src")

# ===========================================
# QtStreamDeck
# ===========================================
file(GLOB_RECURSE streamdeckplugin_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/deps/qtstreamdeck2-local/*.cpp")
target_sources(${target} PRIVATE ${streamdeckplugin_files})
target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/deps/qtstreamdeck2-local")

# ===========================================
# QtDiscord
# ===========================================
file(GLOB_RECURSE discordipc_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/deps/qtdiscordipc-local/*.cpp")
target_sources(${target} PRIVATE ${discordipc_files})
target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/deps/qtdiscordipc-local")

option(BUILD_RECOVERY_TESTS "Build the isolated cached-authentication probe" OFF)
if(BUILD_RECOVERY_TESTS)
    qt_add_executable(qdiscord-auth-probe tests/native-auth-client.cpp ${discordipc_files})
    target_include_directories(qdiscord-auth-probe PRIVATE "${PROJECT_SOURCE_DIR}/deps/qtdiscordipc-local")
    target_link_libraries(qdiscord-auth-probe PRIVATE Qt6::Network Qt6::Gui)
    set_target_properties(qdiscord-auth-probe PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests")
endif()

# ===========================================
# Deployment
# ===========================================

# Copy files from dist
install(DIRECTORY "${PROJECT_SOURCE_DIR}/dist/" DESTINATION ${target_directory})
install(DIRECTORY "${PROJECT_SOURCE_DIR}/deps/qtstreamdeck2-local/dist/" DESTINATION ${target_directory})

# Run windeployqt
get_target_property(QMAKE_FILEPATH Qt6::qmake IMPORTED_LOCATION)

install(CODE "set(target_file \"$<TARGET_FILE:${target}>\")")
install(CODE "set(QMAKE_FILEPATH \"${QMAKE_FILEPATH}\")")
install(CODE "set(PROJECT_SOURCE_DIR \"${PROJECT_BASE_DIR}\")")
install(CODE "set(CMAKE_BINARY_DIR \"${CMAKE_BASE_DIR}\")")
install(SCRIPT "${PROJECT_SOURCE_DIR}/deploy.cmake")
