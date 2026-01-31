cmake_minimum_required(VERSION 3.16)
project(Raefile VERSION 2.0.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/core/FileSystemEngine.cpp
    src/ui/MainWindow.cpp
    src/ui/FileListWidget.cpp
    include/core/FileSystemEngine.h
    include/ui/MainWindow.h
    include/ui/FileListWidget.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets to binary directory"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)
