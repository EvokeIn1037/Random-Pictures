cmake_minimum_required(VERSION 3.16)

project(setu VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        setu.cpp
        setu.h
        setu.ui
)

# Manually specify curl if find_package can't locate it
set(CURL_INCLUDE_DIR "D:/curl/include")
set(CURL_LIBRARIES "D:/curl/lib/libcurl.dll.a")
include_directories(${CURL_INCLUDE_DIR})
link_libraries(${CURL_LIBRARIES})

# Add the path to nlohmann/json.hpp (you should replace 'path_to_json' with the actual path)
set(JSON_INCLUDE_DIR "D:/msys64/mingw64/include")
include_directories(${JSON_INCLUDE_DIR})

# Link CURL
# target_link_libraries("setu VERSION 0.1 LANGUAGES CXX" ${CURL_LIBRARIES})
# add_executable("setu VERSION 0.1 LANGUAGES CXX" setu.cpp)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(setu
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        iconPic.qrc
        resources.rc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET setu APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(setu SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(setu
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(setu PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.setu)
endif()
set_target_properties(setu PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS setu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(setu)
endif()

