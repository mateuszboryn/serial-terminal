cmake_minimum_required(VERSION 3.5)

project(serial-terminal VERSION 0.1 LANGUAGES CXX)

set(QT_CMAKE_DIR "D:/Qt/5.15.2/msvc2019_64/lib/cmake")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets SerialPort)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(serial-terminal
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(serial-terminal
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(serial-terminal PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::SerialPort)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.serial-terminal)
endif()
set_target_properties(serial-terminal PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS serial-terminal
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_CMAKE_DIR}/../../../bin" "${_qt5Core_install_prefix}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                        --no-compiler-runtime
                        --no-translations
                        \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/serial-terminal.exe\"
            )
        ")
    endif()

    message("VCToolsRedistDir = $ENV{VCToolsRedistDir}")
    # Determine Visual C++ Redistributable executable name based on target architecture (64-bit or 32-bit)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(VCREDIST_NAME "vc_redist.x64.exe")
    else()
        set(VCREDIST_NAME "vc_redist.x86.exe")
    endif()

    cmake_path(APPEND VCREDIST_EXE $ENV{VCToolsRedistDir} ${VCREDIST_NAME})
    message("VCREDIST_EXE = ${VCREDIST_EXE}")

    if(VCREDIST_EXE AND EXISTS "${VCREDIST_EXE}" AND NOT IS_DIRECTORY "${VCREDIST_EXE}")
        message(STATUS "Including VCRedist (${VCREDIST_NAME}): ${VCREDIST_EXE}")
        install(FILES "${VCREDIST_EXE}" DESTINATION vcredist RENAME "${VCREDIST_NAME}")
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  ExecWait '\\\"$INSTDIR\\\\vcredist\\\\${VCREDIST_NAME}\\\" /install /passive /norestart'
")
    else()
        message(WARNING "${VCREDIST_NAME} not found in MSVC redist path. VCRedist will not be included in the installer.")
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(serial-terminal)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Maintainer <maintainer@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_EXECUTABLES "serial-terminal" "Serial Terminal")
    set(CPACK_NSIS_PACKAGE_NAME "Serial Terminal")
    set(CPACK_NSIS_DISPLAY_NAME "Serial Terminal")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
endif()

include(CPack)
