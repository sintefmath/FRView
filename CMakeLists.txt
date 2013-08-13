PROJECT( cpview )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

OPTION( EXTEND_CMAKE_MODULE_PATH "Extend the CMAKE_MODULE_PATH variable with user directories?"  ON )
OPTION( DESKTOP_JOB "Build desktop job" ON )
OPTION( CLOUD_JOB "Build cloud job" ON )
OPTION( GLUT_APP "Build glut app" OFF )
OPTION( CHECK_TOPOLOGY "Check topology of tessellated cells (slow!)" OFF )
OPTION( ECLIPSESCAN_APP "Build app to scan eclipse files" OFF )
OPTION( PROFILE "Enable profiling" OFF )
OPTION( USE_SSE2 "Use SSE2 intrinsics" ON )
OPTION( USE_SSSE3 "Use SSSE3 intrinsics" ON )
OPTION( USE_SSE4 "Use SSE4.1 and SSE4.2 intrinsics" ON )
OPTION( CHECK_INVARIANTS "Check invariants throughout the code" OFF )
OPTION( DEBUG_GRAPHICS "Enable debug graphics" OFF )
OPTION( OMIT_GEOMETRIC_EDGES "Omit extraction of geometric edges" OFF )

IF( EXTEND_CMAKE_MODULE_PATH )
  SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "../tinia/cmake/Modules"
    "./cmake/Modules"
    "~/cmake/Modules"
    "$ENV{HOMEPATH}/cmake/Modules"
    "../siut/cmakemacros"
    "../siut"
    "."
    )
ENDIF( EXTEND_CMAKE_MODULE_PATH )

INCLUDE( txt2cpp )



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
set( CMAKE_CXX_FLAGS "-std=c++0x -Wall ${CMAKE_CXX_FLAGS}" )

FIND_PACKAGE( Tinia REQUIRED COMPONENTS Server Desktop )
FIND_PACKAGE( OpenGL REQUIRED)
FIND_LIBRARY( GLEW  NAMES GLEW glew32 PATHS ${LIB_PATHS} )
FIND_PACKAGE( GLM REQUIRED )
FIND_PACKAGE( SIUT2 REQUIRED )
FIND_LIBRARY( LOG4CXX_LOG4CXX_LIBRARY log4cxx
    /usr/lib
    /usr/local/lib
)

# Wrap this in a find module
FIND_PATH( HPMC_INCLUDE_DIR hpmc.h PATHS ../hpmc/hpmc/include ../hpmc_trunk/hpmc/include )
FIND_LIBRARY( HPMC_LIBRARY hpmc ../hpmc ../hpmc_trunk )

FIND_PATH(LOG4CXX_INCLUDE_DIR log4cxx/logger.h)
SET(LOG4CXX_LIBRARIES ${LOG4CXX_LOG4CXX_LIBRARY})


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
    IF( OMIT_GEOMETRIC_EDGES )
        SET( CMAKE_CXX_FLAGS "-DOMIT_GEOMETRIC_EDGES ${CMAKE_CXX_FLAGS}" )
    ENDIF( OMIT_GEOMETRIC_EDGES )
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

INCLUDE_DIRECTORIES(
    "./src"
    ${TINIA_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${SIUT2_INCLUDE_DIR}
    ${HPMC_INCLUDE_DIR}
)

# --- Compile and link eclipse io library --------------------------------------
ADD_LIBRARY( eclipseio
     "src/eclipse/Eclipse.cpp"
     "src/eclipse/EclipseParser.cpp"
     "src/eclipse/EclipseReader.cpp"
     "src/Logger.cpp"
     "src/PerfTimer.cpp"
)
TARGET_LINK_LIBRARIES( eclipseio ${LOG4CXX_LIBRARIES} )

# --- shader files -------------------------------------------------------------
FILE( GLOB cpview_core_SHADERS "shaders/*glsl" )
FOREACH( file ${cpview_core_SHADERS} )
    GET_FILENAME_COMPONENT( symbol ${file} NAME_WE)
    ADD_TEXT_FILE( ${symbol} "resources::${symbol}" ${file} )
    LIST( APPEND SHADER_LIBS ${symbol} )
ENDFOREACH()

# --- javscript scriptlets -----------------------------------------------------
FILE( GLOB cpview_core_JS "js/*.js" )
FOREACH( file ${cpview_core_JS} )
    GET_FILENAME_COMPONENT( symbol ${file} NAME_WE)
    ADD_TEXT_FILE( ${symbol} "resources::${symbol}" ${file} )
    LIST( APPEND JS_LIBS ${symbol} )
ENDFOREACH()

# --- Compile and link code that is common for all eclipse viz apps ------------
ADD_LIBRARY( cpview_core
             "src/models/Appearance.cpp"
             "src/models/GridStats.cpp"
             "src/models/File.cpp"
             "src/models/UnderTheHood.cpp"
             "src/Project.cpp"
             "src/render/ClipPlane.cpp"
             "src/render/CellSelector.cpp"
             "src/render/GridTess.cpp"
             "src/render/GridTessBridge.cpp"
             "src/render/GridTessSubset.cpp"
             "src/render/GridTessSurf.cpp"
             "src/render/GridTessSurfBuilder.cpp"
             "src/render/GridTessSurfRenderer.cpp"
             "src/render/GridFieldBridge.cpp"
             "src/render/GridCubeRenderer.cpp"
             "src/render/GridField.cpp"
             "src/cornerpoint/Tessellator.cpp"
             "src/cornerpoint/PillarFloorSampler.cpp"
             "src/cornerpoint/PillarWallSampler.cpp"
             "src/FooBarParser.cpp"
             "src/CellSanityChecker.cpp"
             "src/render/TextRenderer.cpp"
             "src/render/TextRendererFont.cpp"
             "src/render/WellRenderer.cpp"
             "src/render/CoordSysRenderer.cpp"
             "src/render/GridVoxelization.cpp"
             "src/render/VoxelSurface.cpp"
             "src/render/TimerQuery.cpp"
)

TARGET_LINK_LIBRARIES( cpview_core ${SHADER_LIBS} ${JS_LIBS} )

# --- Compile and link job (common to both desktop and cloud jobs) -------------
ADD_LIBRARY( cpview_job "src/CPViewJob.cpp"
                        "src/ASyncReader.cpp"
                        "src/CPViewJob_Fetch.cpp"
                        "src/CPViewJob_Compute.cpp"
                        "src/CPViewJob_Render.cpp"
                        "src/CPViewJob_RenderList.cpp"
)
TARGET_LINK_LIBRARIES( cpview_job
                       cpview_core
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
    ADD_EXECUTABLE( cpview_desktop_job "src/main_job.cpp" )
    TARGET_LINK_LIBRARIES( cpview_desktop_job
      ${TINIA_LIBRARIES}
                           cpview_job
                           cpview_core
                           eclipseio
                           ${tinia_RenderListGL_LIBRARY}
                           ${tinia_RenderList_LIBRARY}
                           ${tinia_jobcontroller_LIBRARY}
                           ${tinia_model_LIBRARY}
                           ${tinia_qtcontroller_LIBRARY}
                           ${HPMC_LIBRARY}
                           ${GLEW}
                           ${OPENGL_LIBRARIES}
                           ${QT_LIBRARIES}
                           ${QT_QTOPENGL_LIBRARIES}
                           ${Boost_LIBRARIES}
                           ${LOG4CXX_LIBRARIES}
    )
ENDIF( DESKTOP_JOB )

# --- Compile and linke cloud job ----------------------------------------------
IF( CLOUD_JOB )
    FIND_PACKAGE(LibXml2 REQUIRED)
    INCLUDE_DIRECTORIES( ${LIBXML2_INCLUDE_DIR} )
    ADD_EXECUTABLE( cpview_cloud_job "src/main_cloud_job.cpp" )
    TARGET_LINK_LIBRARIES( cpview_cloud_job
      ${TINIA_LIBRARIES}
                           cpview_job
                           cpview_core
                           eclipseio
                           ${tinia_RenderListGL_LIBRARY}
                           ${tinia_RenderList_LIBRARY}
                           ${tinia_jobcontroller_LIBRARY}
                           ${tinia_model_LIBRARY}
                           ${tinia_modelxml_LIBRARY}
                           ${tinia_trell_LIBRARY}
                           ${HPMC_LIBRARY}
                           ${GLEW}
                           ${OPENGL_LIBRARIES}
                           ${Boost_LIBRARIES}
                           ${LOG4CXX_LIBRARIES}
                           ${LIBXML2_LIBRARIES} )
ENDIF( CLOUD_JOB )


# ${tinia_RenderListGL_LIBRARY}
# ${tinia_RenderList_LIBRARY}
# ${tinia_jobcontroller_LIBRARY}
# ${tinia_model_LIBRARY}
# ${tinia_modelxml_LIBRARY}
# ${tinia_qtcontroller_LIBRARY}
# ${tinia_trell_LIBRARY}

# --- Compile and link simple glut app (doesn't use policylib at all) ----------
IF( GLUT_APP )
    ADD_EXECUTABLE( cpview_glut_app "src/main_glut.cpp" )
    TARGET_LINK_LIBRARIES( cpview_glut_app
                           cpview_core
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
ENDIF( GLUT_APP )

# --- Compile and link simple app that scans through eclipse files -------------
IF( ECLIPSESCAN_APP )
    ADD_EXECUTABLE( eclipsescan "src/eclipsescan.cpp" )
    TARGET_LINK_LIBRARIES( eclipsescan
                           eclipseio
                           ${LOG4CXX_LIBRARIES}
    )
ENDIF( ECLIPSESCAN_APP )

#ADD_EXECUTABLE( sseplayground "src/SSEPlayGround.cpp" )
#TARGET_LINK_LIBRARIES( sseplayground rt )
