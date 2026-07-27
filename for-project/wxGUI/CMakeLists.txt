cmake_minimum_required(VERSION 3.10)
project(picklebot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Manually add wx include directory (this is the key fix)
include_directories(/usr/include/wx-3.2)

# Find wxWidgets for libs only
find_package(wxWidgets 3.0 COMPONENTS core base REQUIRED)

set(SRC_FILES
    main.cpp
    MainFrame.cpp
    BoardPanel.cpp
)

add_executable(picklebot ${SRC_FILES})

target_link_libraries(picklebot ${wxWidgets_LIBRARIES})