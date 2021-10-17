cmake_minimum_required(VERSION 3.5)

project(StrawberryShip LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Qt5 COMPONENTS Widgets Gui REQUIRED)

include_directories("reader")

set(READER
    reader/rawdata.h
    reader/rawdata.cpp
    reader/sourceerrordetector.h
    reader/sourceerrordetector.cpp
    reader/sourcefilereader.h
    reader/sourcefilereader.cpp
    )

add_executable(${PROJECT_NAME} WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    ${READER}
  )

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Gui)
