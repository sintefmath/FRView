PROJECT( FRView )
CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
SET( version_number "0.7.0" )

OPTION( EXTEND_CMAKE_MODULE_PATH "Extend the CMAKE_MODULE_PATH variable with user directories?"  ON )
OPTION( DESKTOP_JOB "Build desktop job" ON )
OPTION( CLOUD_JOB "Build cloud job" ON )
OPTION( CHECK_TOPOLOGY "Check topology of tessellated cells (slow!)" OFF )
OPTION( ECLIPSESCAN_APP "Build app to scan eclipse files" OFF )
OPTION( PROFILE "Enable profiling" OFF )
OPTION( USE_SSE2 "Use SSE2 intrinsics" ON )
OPTION( USE_SSSE3 "Use SSSE3 intrinsics" ON )
OPTION( USE_SSE4 "Use SSE4.1 and SSE4.2 intrinsics" ON )
OPTION( CHECK_INVARIANTS "Check invariants throughout the code" OFF )
OPTION( EXTRACT_EDGE_GEOMETRY "Extract edge geometry" ON )

IF( EXTEND_CMAKE_MODULE_PATH )
  SET( CMAKE_MODULE_PATH
       "${CMAKE_MODULE_PATH}"
       "${CMAKE_SOURCE_DIR}/cmake/Modules"
       "~/cmake/Modules"
       "$ENV{HOMEPATH}/cmake/Modules" )
ENDIF( EXTEND_CMAKE_MODULE_PATH )

INCLUDE( WrapResources )


# Debian package
SET( CPACK_GENERATOR "DEB" )
SET( CPACK_DEBIAN_PACKAGE_NAME "qtobserver" )
SET( CPACK_PACKAGE_VERSION ${version_number} )
SET( CPACK_DEBIAN_PACKAGE_MAINTAINER "kjetil.olsen.lye@sintef.no" )
SET( CPACK_COMPONENTS_ALL libraries headers )
SET( CPACK_COMPONENT_HEADERS_DEPENDS libraries )
SET( CPACK_PACKAGE_INSTALL_DIRECTORY "/" )
include ( CPack )

#set( CMAKE_INSTALL_PREFIX "/usr/local" )

set( BUILDDIR "build" )
set( CMAKE_CXX_FLAGS "-std=c++0x -Wall ${CMAKE_CXX_FLAGS}" )

FIND_PACKAGE( Tinia 0.2.5 REQUIRED COMPONENTS Server Desktop )
FIND_PACKAGE( OpenGL REQUIRED)
FIND_PACKAGE( GLEW )

# We require at least version 0.9.4, since we use radians and not degrees.
FIND_PACKAGE( GLM REQUIRED )
ADD_DEFINITIONS( -DGLM_FORCE_RADIANS )

# For VTK reading
FIND_PACKAGE( LibXml2 REQUIRED)
FIND_LIBRARY( LOG4CXX_LOG4CXX_LIBRARY log4cxx
    /usr/lib
    /usr/local/lib
)

# Wrap this in a find module
FIND_PATH( HPMC_INCLUDE_DIR hpmc.h PATHS ../hpmc/hpmc/include ../hpmc_trunk/hpmc/include )
FIND_LIBRARY( HPMC_LIBRARY hpmc ../hpmc ../hpmc_trunk )

FIND_PATH(LOG4CXX_INCLUDE_DIR log4cxx/logger.h)
SET(LOG4CXX_LIBRARIES ${LOG4CXX_LOG4CXX_LIBRARY})

IF(LOG4CXX_LIBRARIES AND LOG4CXX_INCLUDE_DIR)
       SET(LOG4CXX_FOUND TRUE)
       ADD_DEFINITIONS(-DFRVIEW_HAS_LOG4CXX)
ELSE()
       MESSAGE("Could not find LOG4CXX, defaulting to std::cout-logging")
       SET(LOG4CXX_FOUND FALSE)
       SET(LOG4CXX_INCLUDE_DIR "")
       SET(LOG4CXX_LIBRARIES "")
ENDIF()

IF( UNIX )
#        SET( CMAKE_CXX_FLAGS "-Wall -fprofile-arcs -ftest-coverage -pg -g2 -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
#        SET( CMAKE_CXX_FLAGS "-Wall -O3 -msse2 -mfpmath=sse -g2 -DCHECK_INVARIANTS -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
    SET( CMAKE_CXX_FLAGS "-Wall -O3 -g2 -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
    IF( CHECK_TOPOLOGY )
        SET( CMAKE_CXX_FLAGS "-DCHECK_TOPOLOGY ${CMAKE_CXX_FLAGS}" )
    ENDIF( CHECK_TOPOLOGY )
    IF( PROFILE )
        SET( CMAKE_CXX_FLAGS " -fprofile-arcs -ftest-coverage -pg -g2 ${CMAKE_CXX_FLAGS}" )
    ENDIF( PROFILE )
    IF( CHECK_INVARIANTS )
        SET( CMAKE_CXX_FLAGS "-DCHECK_INVARIANTS ${CMAKE_CXX_FLAGS}" )
    ENDIF( CHECK_INVARIANTS )
    IF( EXTRACT_EDGE_GEOMETRY )
        SET( CMAKE_CXX_FLAGS "-DEXTRACT_EDGE_GEOMETRY ${CMAKE_CXX_FLAGS}" )
    ENDIF( EXTRACT_EDGE_GEOMETRY )
    IF( USE_SSE4 )
        SET( CMAKE_CXX_FLAGS "-msse4 ${CMAKE_CXX_FLAGS}" )
    ELSE( USE_SSE4 )
        IF( USE_SSSE3 )
            SET( CMAKE_CXX_FLAGS "-mssse3 -mfpmath=sse ${CMAKE_CXX_FLAGS}" )
        ELSE( USE_SSSE3 )
            IF( USE_SSE2 )
                SET( CMAKE_CXX_FLAGS "-msse2 -mfpmath=sse ${CMAKE_CXX_FLAGS}" )
            ELSE( USE_SSE2 )
                MESSAGE( "Sure that the CPU doesn't have SSE capabilites?" )
            ENDIF( USE_SSE2 )
        ENDIF( USE_SSSE3 )
    ENDIF( USE_SSE4 )
ENDIF( UNIX )

INCLUDE_DIRECTORIES(
    "./src"
    ${TINIA_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIR}
    ${HPMC_INCLUDE_DIR}
)


# --- small data files ---------------------------------------------------------
FILE( GLOB frview_EXAMPLE_DATA "data/*.gtxt" )
SOURCE_GROUP( "Example data" FILES ${frview_EXAMPLE_DATA} )
SET_SOURCE_FILES_PROPERTIES( ${frview_EXAMPLE_DATA} PROPERTIES HEADER_FILE_ONLY TRUE )

# --- javscript scriptlets -----------------------------------------------------
FILE( GLOB frview_core_JS "js/*.js" )
SOURCE_GROUP( "JS" FILES ${frview_core_JS} )
SET_SOURCE_FILES_PROPERTIES( ${frview_core_JS} PROPERTIES HEADER_FILE_ONLY TRUE )
FOREACH( file ${frview_core_JS} )
    GET_FILENAME_COMPONENT( symbol ${file} NAME_WE)
    ADD_TEXT_FILE( ${symbol} "resources::${symbol}" ${file} )
    LIST( APPEND JS_LIBS ${symbol} )
ENDFOREACH()

# --- shader files -------------------------------------------------------------

# --- Compile and link code that is common for all eclipse viz apps ------------

MACRO( WRAP_SHADERS objects sources namespace path )
    FILE( GLOB files ${path} )
    FOREACH( file ${files} )
        GET_FILENAME_COMPONENT( symbol ${file} NAME_WE)
        SET( full "${namespace}::${symbol}" )
        STRING( REGEX REPLACE "::" "_" foo ${full} )
        ADD_TEXT_FILE( ${foo} ${full} ${file} )
        LIST( APPEND ${sources} ${file} )
        LIST( APPEND ${objects} ${foo} )
    ENDFOREACH()
ENDMACRO( WRAP_SHADERS )


FILE( GLOB frview_core_SRC "src/models/*.cpp"
                           "src/bridge/*.cpp"
                           "src/render/*.cpp"
                           "src/render/mesh/*.cpp"
                           "src/render/wells/*.cpp"
                           "src/render/rlgen/*.cpp"
                           "src/render/subset/*.cpp"
                           "src/render/surface/*.cpp"
                           "src/render/manager/*.cpp"
                           "src/cornerpoint/*.cpp"
                           "src/eclipse/*.cpp"
                           "src/job/*.cpp"
                           "src/utils/*.cpp"
                           "src/dataset/*.cpp" )
FILE( GLOB frview_core_HDR "src/models/*.hpp"
                           "src/bridge/*.hpp"
                           "src/render/*.hpp"
                           "src/render/mesh/*.hpp"
                           "src/render/wells/*.hpp"
                           "src/render/rlgen/*.hpp"
                           "src/render/subset/*.hpp"
                           "src/render/surface/*.hpp"
                           "src/render/manager/*.hpp"
                           "src/cornerpoint/*.hpp"
                           "src/eclipse/*.hpp"
                           "src/job/*.hpp"
                           "src/utils/*.hpp"
                           "src/dataset/*.hpp" )
SET_SOURCE_FILES_PROPERTIES( ${frview_HDR} PROPERTIES HEADER_FILE_ONLY TRUE )

WRAP_SHADERS( SHADER_LIBS frview_core_SHADERS "resources" "shaders/*.glsl" )
WRAP_SHADERS( SHADER_LIBS frview_core_SHADERS "render::wells::glsl" "src/render/wells/*.glsl" )
WRAP_SHADERS( SHADER_LIBS frview_core_SHADERS "render::rlgen::glsl" "src/render/rlgen/*.glsl" )
WRAP_SHADERS( SHADER_LIBS frview_core_SHADERS "render::subset::glsl" "src/render/subset/*.glsl" )
WRAP_SHADERS( SHADER_LIBS frview_core_SHADERS "render::surface::glsl" "src/render/surface/*.glsl" )
WRAP_SHADERS( SHADER_LIBS frview_core_SHADERS "render::manager::glsl" "src/render/manager/*.glsl" )

SOURCE_GROUP( "GLSL" FILES ${frview_core_SHADERS} )
SET_SOURCE_FILES_PROPERTIES( ${frview_core_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE )




ADD_LIBRARY( frview_core
             ${frview_core_SHADERS}         # Add deps to make them appear in solution
             ${frview_core_JS}              # Add deps to make them appear in solution
             ${frview_EXAMPLE_DATA}         # Add deps to make them appear in solution
             ${frview_core_SRC}
             ${frview_core_HDR} )

TARGET_LINK_LIBRARIES( frview_core
                       ${SHADER_LIBS}
                       ${JS_LIBS}
                       ${LIBXML2_LIBRARIES}
)


# --- Compile and link desktop job ---------------------------------------------
IF( DESKTOP_JOB )
    FIND_PACKAGE( Qt4 REQUIRED)
    INCLUDE( ${QT_USE_FILE} )
    SET( QT_USE_QTOPENGL TRUE )
    INCLUDE_DIRECTORIES(
        "/work/projects/qtobserver/include"
        ${QT_INCLUDE_DIR}
        ${QT_QTOPENGL_INCLUDE_DIR}
    )
    ADD_EXECUTABLE( frview_desktop_job "src/main_job.cpp" )
    TARGET_LINK_LIBRARIES( frview_desktop_job
                           frview_core
                           ${TINIA_LIBRARIES}
                           ${HPMC_LIBRARY}
                           ${GLEW}
                           ${OPENGL_LIBRARIES}
                           ${QT_LIBRARIES}
                           ${QT_QTOPENGL_LIBRARIES}
                           ${Boost_LIBRARIES}
                           ${LOG4CXX_LIBRARIES}
    )
  INSTALL(TARGETS frview_desktop_job RUNTIME DESTINATION bin/)
ENDIF( DESKTOP_JOB )

# --- Compile and linke cloud job ----------------------------------------------
IF( CLOUD_JOB )
    FIND_PACKAGE(LibXml2 REQUIRED)
    INCLUDE_DIRECTORIES( ${LIBXML2_INCLUDE_DIR} )
    ADD_EXECUTABLE( frview_cloud_job "src/main_cloud_job.cpp" )
    TARGET_LINK_LIBRARIES( frview_cloud_job
                           frview_core
                           ${TINIA_LIBRARIES}
                           ${HPMC_LIBRARY}
                           ${GLEW}
                           ${OPENGL_LIBRARIES}
                           ${Boost_LIBRARIES}
                           ${LOG4CXX_LIBRARIES}
                           ${LIBXML2_LIBRARIES} )
    INSTALL(TARGETS 
      frview_cloud_job 
      RUNTIME 
      DESTINATION ${TINIA_APPS_DIR})

ENDIF( CLOUD_JOB )




# --- Compile and link simple app that scans through eclipse files -------------
IF( ECLIPSESCAN_APP )
    ADD_EXECUTABLE( eclipsescan "src/eclipsescan.cpp" 
                                "src/eclipse/Eclipse.cpp"
                                "src/eclipse/EclipseParser.cpp"
                                "src/eclipse/EclipseReader.cpp"
                                "src/utils/Logger.cpp"
                                "src/utils/PerfTimer.cpp"
    )
    TARGET_LINK_LIBRARIES( eclipsescan
                           ${LOG4CXX_LIBRARIES}
    )
ENDIF( ECLIPSESCAN_APP )

#ADD_EXECUTABLE( sseplayground "src/SSEPlayGround.cpp" )
#TARGET_LINK_LIBRARIES( sseplayground rt )
