cmake_minimum_required(VERSION 3.16)
project(panon VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PULSEAUDIO REQUIRED libpulse libpulse-simple)
pkg_check_modules(DDE_DOCK REQUIRED dde-dock)

# dtkgui may be named differently across Deepin versions
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
if(NOT DTK_FOUND)
    find_path(DGuiApplicationHelper_INCLUDE_DIR
        NAMES DGuiApplicationHelper
        PATHS /usr/include /usr/include/libdtk-6.0.0 /usr/include/libdtk-5.6.0
    )
    if(DGuiApplicationHelper_INCLUDE_DIR)
        set(DTK_INCLUDE_DIRS ${DGuiApplicationHelper_INCLUDE_DIR})
    endif()
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

target_link_libraries(panon PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PULSEAUDIO_LIBRARIES}
    ${DTK_LIBRARIES}
)

install(TARGETS panon LIBRARY DESTINATION /usr/lib/dde-dock/plugins/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/panon.json DESTINATION /usr/lib/dde-dock/plugins/)
