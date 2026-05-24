cmake_minimum_required(VERSION 3.16)
project(panon VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PULSEAUDIO REQUIRED libpulse libpulse-simple)
pkg_check_modules(DDE_DOCK REQUIRED dde-dock)

# Find DGuiApplicationHelper header from dtk (needed by dde-dock pluginsiteminterface.h)
# Try multiple methods: pkg-config, CMake config, direct search
set(NEEDED_HEADER DGuiApplicationHelper)

# Method 1: pkg-config
pkg_check_modules(DTK dtkgui QUIET)
if(NOT DTK_FOUND)
    pkg_check_modules(DTK dtk6gui QUIET)
endif()
if(NOT DTK_FOUND)
    pkg_check_modules(DTK DtkGui QUIET)
endif()
if(NOT DTK_FOUND)
    pkg_check_modules(DTK Dtk6Gui QUIET)
endif()

# Method 2: CMake config
if(NOT DTK_FOUND)
    find_package(DtkGui QUIET)
    if(DtkGui_FOUND)
        # Use the CMake target instead of raw directories
        set(DTK_FOUND TRUE)
        set(DTK_USING_CMAKE_TARGET TRUE)
    endif()
endif()
if(NOT DTK_FOUND)
    find_package(Dtk6Gui QUIET)
    if(Dtk6Gui_FOUND)
        set(DTK_FOUND TRUE)
        set(DTK_USING_CMAKE_TARGET TRUE)
    endif()
endif()

# Method 3: Direct header search (last resort)
# DTK5 uses .h extension; DTK6 uses no extension like Qt6
if(NOT DTK_FOUND OR NOT DTK_INCLUDE_DIRS)
    find_path(DTK_HEADER_DIR
        NAMES ${NEEDED_HEADER} ${NEEDED_HEADER}.h
        PATHS
            /usr/include
            /usr/include/libdtk-6.0.0
            /usr/include/libdtk-5.6.0
            /usr/include/DtkGui
            /usr/include/Dtk6Gui
            /usr/include/dtk6
            /usr/include/Deepin
            /usr/lib/x86_64-linux-gnu
    )
    if(DTK_HEADER_DIR)
        set(DTK_INCLUDE_DIRS ${DTK_HEADER_DIR})
        set(DTK_FOUND TRUE)
    endif()
endif()

if(NOT DTK_FOUND)
    message(WARNING "DGuiApplicationHelper header not found; plugin may not compile")
endif()

add_library(panon MODULE
    src/panonplugin.cpp
    src/spectrumwidget.cpp
    src/audiosource.cpp
    src/fftprocessor.cpp
    src/effects/bareffect.cpp
    src/effects/waveeffect.cpp
    src/effects/solideffect.cpp
    src/effects/beameffect.cpp
    src/effects/hilleffect.cpp
    src/effects/spectrogrameffect.cpp
)

target_include_directories(panon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PULSEAUDIO_INCLUDE_DIRS}
    ${DDE_DOCK_INCLUDE_DIRS}
    ${DTK_INCLUDE_DIRS}
)

if(DTK_USING_CMAKE_TARGET)
    target_link_libraries(panon PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${PULSEAUDIO_LIBRARIES}
        DtkGui::DtkGui
    )
else()
    target_link_libraries(panon PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${PULSEAUDIO_LIBRARIES}
        ${DTK_LIBRARIES}
    )
endif()

install(TARGETS panon LIBRARY DESTINATION /usr/lib/dde-dock/plugins/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/panon.json DESTINATION /usr/lib/dde-dock/plugins/)
