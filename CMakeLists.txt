cmake_minimum_required(VERSION 3.16)

project(Sajilo-Bazarr VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
        source/main.cpp
<<<<<<< Updated upstream
       #login
       source/login.cpp
       ui/login.ui
       include/login.h
       #register
       source/register.cpp
       ui/register.ui
       include/register.h
=======
        source/admindashboard.cpp

        source/dashboard.cpp
        source/login.cpp
        source/mainwindow.cpp
        source/product.cpp
        source/register.cpp
        source/staff.cpp
        source/supplier.cpp
        source/category.cpp

        source/staffdashboard.cpp
        source/frontdesk.cpp
        source/inventory.cpp
        source/addcategory.cpp

        include/admindashboard.h

        include/dashboard.h
        include/login.h
        include/mainwindow.h
        include/register.h

        include/staffdashboard.h
        include/product.h
        include/frontdesk.h
        include/inventory.h
        include/addcategory.h
        include/staff.h
        include/supplier.h
        include/category.h





        ui/admindashboard.ui
        ui/staffdashboard.ui
        ui/product.ui
        ui/frontdesk.ui
        ui/login.ui
        ui/inventory.ui
        ui/addcategory.ui
        ui/category.ui
        ui/supplier.ui
        ui/staff.ui
>>>>>>> Stashed changes
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Sajilo-Bazarr
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET admindashboard APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Sajilo-Bazarr SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Sajilo-Bazarr
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Sajilo-Bazarr PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

target_include_directories(Sajilo-Bazarr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.admindashboard)
endif()
set_target_properties(Sajilo-Bazarr PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Sajilo-Bazarr
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Sajilo-Bazarr)
endif()
