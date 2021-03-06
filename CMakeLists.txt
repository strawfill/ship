cmake_minimum_required(VERSION 3.5)

project(StrawberryShip LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(Qt5 COMPONENTS Widgets Gui Concurrent REQUIRED)

include_directories("reader" "other" "viewer" "pathwalk" "viewer/graphicsitems")

set(VIEWER
    viewer/mainwindow.h
    viewer/mainwindow.cpp
    viewer/mainwindow.ui
    viewer/simulationscene.h
    viewer/simulationscene.cpp
    viewer/ignoredndgraphicsview.h
    viewer/ignoredndgraphicsview.cpp
    viewer/graphicsitemzoomer.h
    viewer/graphicsitemzoomer.cpp
    viewer/graphicsviewzoomer.h
    viewer/graphicsviewzoomer.cpp
    viewer/graphicsitems/graphicsiteminterface.h
    viewer/graphicsitems/tracgraphicsitem.h
    viewer/graphicsitems/tracgraphicsitem.cpp
    viewer/graphicsitems/shipgraphicsitem.h
    viewer/graphicsitems/shipgraphicsitem.cpp
    viewer/placeholderframe.h
    viewer/placeholderframe.cpp
    viewer/waitingframe.h
    viewer/waitingframe.cpp
    )

set(READER
    reader/rawdata.h
    reader/rawdata.cpp
    reader/sourceerrordetector.h
    reader/sourceerrordetector.cpp
    reader/sourcefilereader.h
    reader/sourcefilereader.cpp
    )

set(PATHWALK
    pathwalk/prepareddata.h
    pathwalk/prepareddata.cpp
    pathwalk/patherrordetector.h
    pathwalk/patherrordetector.cpp
    pathwalk/movestopathconverter.h
    pathwalk/movestopathconverter.cpp
    pathwalk/algoannealing.h
    pathwalk/algoannealing.cpp
    pathwalk/algobruteforce.h
    pathwalk/algobruteforce.cpp
    pathwalk/algodummy.h
    pathwalk/algodummy.cpp
    pathwalk/mtsfinder.h
    pathwalk/mtsfinder.cpp
    pathwalk/sortviacrowding.h
    pathwalk/sortviacrowding.cpp
    pathwalk/worker.h
    pathwalk/worker.cpp
    )

set(OTHER
    other/debugcatcher.h
    other/debugcatcher.cpp
    )

set(RESOURCES
    resources/data.qrc
    resources/rcIco.rc
    )

add_executable(${PROJECT_NAME} WIN32
    main.cpp

    ${VIEWER}
    ${READER}
    ${PATHWALK}
    ${OTHER}
    ${RESOURCES}
    )

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Gui Qt5::Concurrent)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}
    )


#target_compile_options(${PROJECT_NAME} PRIVATE -O3)
#target_compile_options(${PROJECT_NAME} PRIVATE -O2 -ftree-vectorize -fopt-info-vec-optimized )
