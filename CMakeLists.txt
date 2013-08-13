PROJECT( cpview )
cmake_minimum_required(VERSION 2.4)

OPTION(EXTEND_CMAKE_MODULE_PATH
       "Extend the CMAKE_MODULE_PATH variable with user directories?"
        ON)
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

INCLUDE_DIRECTORIES(
        "./src"
)

IF( UNIX )
#        SET( CMAKE_CXX_FLAGS "-Wall -fprofile-arcs -ftest-coverage -pg -g2 -O2 -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
        SET( CMAKE_CXX_FLAGS "-Wall -O2 -DDEBUG -DUNIX -std=gnu++0x ${CMAKE_CXX_FLAGS}" )
ENDIF( UNIX )


FIND_PACKAGE(Qt4 REQUIRED)

ADD_DEFINITIONS( -DUSE_GLEW )
ADD_DEFINITIONS( -DHAVE_FREEGLUT )


FIND_LIBRARY( GLEW
              NAMES GLEW glew32
              PATHS ${LIB_PATHS} )
FIND_PACKAGE( OpenGL REQUIRED )
FIND_PACKAGE( Freeglut REQUIRED )
FIND_PACKAGE( siut REQUIRED )
FIND_PACKAGE( GLM REQUIRED )
FIND_LIBRARY(LOG4CXX_LOG4CXX_LIBRARY log4cxx
    /usr/lib
    /usr/local/lib
)
FIND_PATH(LOG4CXX_INCLUDE_DIR log4cxx/logger.h)
SET(LOG4CXX_LIBRARIES ${LOG4CXX_LOG4CXX_LIBRARY})


INCLUDE_DIRECTORIES(
        "./include"
        "../include"
        ${GLEW_INCLUDE_DIR}
        ${siut_INCLUDE_DIR}
        ${Freeglut_INCLUDE_DIR}
        ${GLM_INCLUDE_DIRS}
        ${LOG4CXX_INCLUDE_DIR}
)

SET( SRC
     "src/main.cpp"
     "src/Logger.cpp"
     "src/Project.cpp"
     "src/Eclipse.cpp"
     "src/EclipseParser.cpp"
     "src/EclipseReader.cpp"
     "src/ClipPlane.cpp"
     "src/CellSelector.cpp"
     "src/GridTess.cpp"
     "src/GridTessRenderer.cpp"
     "src/GridField.cpp"
     "src/CornerPointTessellatorSimple.cpp"
     "src/CornerPointTessellator.cpp"
     "src/FooBarParser.cpp"
)

SET( ECLIPSESCAN_SRC
  "src/eclipsescan.cpp"
  "src/Eclipse.cpp"
  "src/EclipseReader.cpp"
  "src/Logger.cpp"
)
ADD_EXECUTABLE( eclipsescan ${ECLIPSESCAN_SRC} )
TARGET_LINK_LIBRARIES( eclipsescan ${LOG4CXX_LIBRARIES} )


SET(QT_USE_QTOPENGL TRUE)
SET(QT_USE_QTUITOOLS TRUE)
SET( qcpview_SOURCES
    "src/qcpview.cpp"
    "src/QAppWindow.cpp"
    "src/QViewerWidget.cpp"
    "src/Logger.cpp"
    "src/Project.cpp"
    "src/EclipseReader.cpp"
    "src/EclipseParser.cpp"
    "src/Eclipse.cpp"
)
SET( qcpview_HEADERS
    "src/QAppWindow.hpp"
    "src/QViewerWidget.hpp"
)
SET( qcpview_FORMS )
SET( qcpview_RESOURCES )

QT4_WRAP_CPP( qcpview_HEADERS_MOC ${qcpview_HEADERS} )
QT4_WRAP_UI( qcpview_FORMS_HEADERS ${qcpview_FORMS} )
QT4_ADD_RESOURCES( qcpview_RESOURCES_RCC ${qcpview_RESOURCES} )
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ADD_DEFINITIONS(${QT_DEFINITIONS})

#ADD_EXECUTABLE( qcpview
#                ${qcpview_SOURCES}
#                ${qcpview_HEADERS_MOC}
#                ${qcpview_FORMS_HEADERS}
#                ${qcpview_RESOURCES_RCC}
#)
#TARGET_LINK_LIBRARIES( qcpview
#                       ${GLEW}
#                       ${QT_LIBRARIES}
#                       ${LOG4CXX_LIBRARIES}
#)

ADD_EXECUTABLE( cpview_job ${SRC} )

TARGET_LINK_LIBRARIES( cpview_job
                       ${GLEW}
                       ${Freeglut_LIBRARY}
                       ${OPENGL_LIBRARIES}
                       ${GLUT_LIBRARIES}
                       ${LOG4CXX_LIBRARIES}
)

