cmake_minimum_required(VERSION 3.20)
project(lsms LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

add_executable(lsms
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/PlaybackWidget.h
    src/PlaybackWidget.cpp
    src/FilmstripWidget.h
    src/FilmstripWidget.cpp
    src/CropDialog.h
    src/CropDialog.cpp
    src/Project.h
    src/Project.cpp
    src/VideoExporter.h
    src/VideoExporter.cpp
    resources/app.qrc
)

target_link_libraries(lsms PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)

if(WIN32)
    set_target_properties(lsms PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
