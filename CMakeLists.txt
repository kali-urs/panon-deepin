cmake_minimum_required(VERSION 3.16)
project(panon VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PULSEAUDIO QUIET libpulse libpulse-simple)
pkg_check_modules(DDE_DOCK REQUIRED dde-dock)

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
    ${DDE_DOCK_INCLUDE_DIRS}
)

target_link_libraries(panon PRIVATE
    Qt6::Core
    Qt6::Widgets
)

install(TARGETS panon LIBRARY DESTINATION /usr/lib/dde-dock/plugins/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/panon.json DESTINATION /usr/lib/dde-dock/plugins/)
