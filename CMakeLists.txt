#----------------------------------------------------------------------------
# CMakeLists.txt for cttwin
#
# Adapted from the standard Geant4 examples build template.
# Builds a single executable `cttwin` from sources in src/ and headers in
# include/, linked against the Geant4 libraries discovered on the system.
#
# Usage:
#     mkdir build && cd build
#     cmake ..
#     make -jN
#     ./cttwin              # interactive (uses macros/vis.mac)
#     ./cttwin run.mac      # batch mode with a macro file
#----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(cttwin LANGUAGES CXX)

#----------------------------------------------------------------------------
# Find Geant4. By default include all UI and visualization drivers; users
# can disable visualization with -DWITH_GEANT4_UIVIS=OFF on the cmake line.
#----------------------------------------------------------------------------
option(WITH_GEANT4_UIVIS "Build with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
    find_package(Geant4 REQUIRED ui_all vis_all)
else()
    find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# Project-wide settings
#----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

#----------------------------------------------------------------------------
# Sources and headers
#----------------------------------------------------------------------------
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Executable
#----------------------------------------------------------------------------
add_executable(cttwin cttwin.cc ${sources} ${headers})
target_link_libraries(cttwin ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copy macro files into the build directory so the executable can find them
# at runtime without a fully qualified path.
#----------------------------------------------------------------------------
set(CTTWIN_MACROS
    vis.mac
    init_physics.mac
    single_projection.mac
    full_scan_template.mac
)
foreach(_macro ${CTTWIN_MACROS})
    configure_file(
        ${PROJECT_SOURCE_DIR}/macros/${_macro}
        ${PROJECT_BINARY_DIR}/${_macro}
        COPYONLY
    )
endforeach()

#----------------------------------------------------------------------------
# Install
#----------------------------------------------------------------------------
install(TARGETS cttwin DESTINATION bin)
