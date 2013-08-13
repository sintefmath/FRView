PROJECT( cpview )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
OPTION( EXTEND_CMAKE_MODULE_PATH
  "Extend the CMAKE_MODULE_PATH variable with user directories?"
  ON )
IF( EXTEND_CMAKE_MODULE_PATH )
  SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "./cmake/Modules"
    "~/cmake/Modules"
    "$ENV{HOMEPATH}/cmake/Modules"
    "../siut/cmakemacros"
    "../siut"
    "."
    )
ENDIF( EXTEND_CMAKE_MODULE_PATH )
OPTION( PROFILE "Enable profiling" OFF )
OPTION( USE_SSE2 "Use SSE2 intrinsics" ON )
OPTION( USE_SSSE3 "Use SSSE3 intrinsics" ON )
OPTION( USE_SSE4 "Use SSE4.1 and SSE4.2 intrinsics" ON )
OPTION( CHECK_INVARIANTS "Check invariants throughout the code" OFF )
OPTION( DEBUG_GRAPHICS "Enable debug graphics" OFF )

# Versioning
SET( svn_revision $ENV{SVN_REVISION} )
IF ( NOT DEFINED svn_revision )
    SET( svn_revision 0 )
ENDIF( NOT  DEFINED svn_revision )

SET( build_number $ENV{BUILD_NUMBER} )
IF ( NOT DEFINED build_number )
    SET( build_number 0 )
ENDIF( NOT  DEFINED build_number )

SET( version_number "0.${svn_revision}.${build_number}" )

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
set( CMAKE_CXX_FLAGS "-std=c++0x -Wall" )


FIND_LIBRARY( GLEW
              NAMES GLEW glew32
              PATHS ${LIB_PATHS} )
FIND_PACKAGE(Freeglut)
ADD_DEFINITIONS(-DHAVE_FREEGLUT)
FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(PolicyLib REQUIRED)
FIND_PACKAGE(JobObserver REQUIRED)
#FIND_PACKAGE(Boost REQUIRED COMPONENTS thread unit_test_framework prg_exec_monitor test_exec_monitor)
FIND_PACKAGE( GLM REQUIRED )
FIND_PACKAGE(SIUT2 REQUIRED)
FIND_LIBRARY(LOG4CXX_LOG4CXX_LIBRARY log4cxx
    /usr/lib
    /usr/local/lib
)
FIND_LIBRARY(QTOBSERVER QtObserver
    /work/projects/qtobserver
)

FIND_PATH(LOG4CXX_INCLUDE_DIR log4cxx/logger.h)
SET(LOG4CXX_LIBRARIES ${LOG4CXX_LOG4CXX_LIBRARY})

ADD_DEFINITIONS( -DUSE_GLEW )

#ADD_DEFINITIONS( -DHAVE_FREEGLUT )
#FIND_PACKAGE( Freeglut REQUIRED )
#FIND_PACKAGE( siut REQUIRED )

IF( UNIX )
#        SET( CMAKE_CXX_FLAGS "-Wall -fprofile-arcs -ftest-coverage -pg -g2 -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
#        SET( CMAKE_CXX_FLAGS "-Wall -O3 -msse2 -mfpmath=sse -g2 -DCHECK_INVARIANTS -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
    SET( CMAKE_CXX_FLAGS "-Wall -O3 -g2 -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
    IF( PROFILE )
        SET( CMAKE_CXX_FLAGS " -fprofile-arcs -ftest-coverage -pg -g2 ${CMAKE_CXX_FLAGS}" )
    ENDIF( PROFILE )
    IF( CHECK_INVARIANTS )
        SET( CMAKE_CXX_FLAGS "-DCHECK_INVARIANTS ${CMAKE_CXX_FLAGS}" )
    ENDIF( CHECK_INVARIANTS )
    IF( DEBUG_GRAPHICS )
        SET( CMAKE_CXX_FLAGS "-DDEBUG_GRAPHICS ${CMAKE_CXX_FLAGS}" )
    ENDIF( DEBUG_GRAPHICS )
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


#OPTION( USE_SSE2 "Use SSE2 intrinsics" ON )
#OPTION( USE_SSSE3 "Use SSSE3 intrinsics" ON )


INCLUDE(${QT_USE_FILE})
SET(QT_USE_QTOPENGL TRUE)

INCLUDE_DIRECTORIES(
    "./src"
    "/work/projects/qtobserver/include"
    "/work/projects/PolicyLib2/include"
    "/work/projects/jobobserver/include"
    "/work/tmp/glm-0.9.1.1"
    ${SIUT2_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PolicyLib_INCLUDE_DIR}
    ${QT_QTOPENGL_INCLUDE_DIR}
    ${JobObserver_INCLUDE_DIR}
    ${Freeglut_INCLUDE_DIR}
)


ADD_LIBRARY( eclipseio
     "src/Eclipse.cpp"
     "src/EclipseParser.cpp"
     "src/EclipseReader.cpp"
     "src/Logger.cpp"
     "src/PerfTimer.cpp"
)
TARGET_LINK_LIBRARIES( eclipseio ${LOG4CXX_LIBRARIES} )

ADD_LIBRARY( cpview
     "src/Project.cpp"
     "src/ClipPlane.cpp"
     "src/CellSelector.cpp"
     "src/GridTess.cpp"
     "src/GridTessBridge.cpp"
     "src/GridTessSubset.cpp"
     "src/GridTessSurf.cpp"
     "src/GridTessSurfBuilder.cpp"
     "src/GridTessSurfRenderer.cpp"
     "src/PolygonTessellator.cpp"
     "src/GridField.cpp"
     "src/CornerPointTessellatorSimple.cpp"
     "src/CornerPointTessellator.cpp"
     "src/FooBarParser.cpp"
     "src/CellSanityChecker.cpp"
     "src/TextRenderer.cpp"
     "src/TextRendererFont.cpp"
)
TARGET_LINK_LIBRARIES( cpview )

ADD_EXECUTABLE( cpview_job "src/main_job.cpp" "src/CPViewJob.cpp" )
TARGET_LINK_LIBRARIES( cpview_job
                       cpview
                       eclipseio
                       ${QTOBSERVER}
                       ${GLEW}
                       ${OPENGL_LIBRARIES}
                       ${QT_LIBRARIES}
                       ${QT_QTOPENGL_LIBRARIES}
                       ${PolicyLib_LIBRARIES}
                       ${JobObserver_LIBRARIES}
                       ${Boost_LIBRARIES}
                       ${LOG4CXX_LIBRARIES}
)

ADD_EXECUTABLE( cpview_glut "src/main_glut.cpp" )
TARGET_LINK_LIBRARIES( cpview_glut
                       cpview
                       eclipseio
                       ${SIUT2_LIBRARY}
                       ${Freeglut_LIBRARY}
                       ${GLEW}
                       ${OPENGL_LIBRARIES}
                       ${QT_LIBRARIES}
                       ${QT_QTOPENGL_LIBRARIES}
                       ${PolicyLib_LIBRARIES}
                       ${JobObserver_LIBRARIES}
                       ${Boost_LIBRARIES}
                       ${LOG4CXX_LIBRARIES}
)

ADD_EXECUTABLE( sseplayground "src/SSEPlayGround.cpp" )
TARGET_LINK_LIBRARIES( sseplayground rt )

ADD_EXECUTABLE( eclipsescan "src/eclipsescan.cpp" )
TARGET_LINK_LIBRARIES( eclipsescan
                       eclipseio
                       ${LOG4CXX_LIBRARIES}
)
