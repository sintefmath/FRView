#include <siut/dsrv/DSRViewer.hpp>
#include <siut/gl_utils/ContextResourceCache.hpp>
#include <siut/gl_utils/RenderCoordSys.hpp>
#include <GL/glew.h>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSizePolicy>
#include "Logger.hpp"
#include "QViewerWidget.hpp"


QViewerWidget::QViewerWidget(QWidget *parent)
    : QGLWidget( QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer), parent ),
      m_viewer( NULL ),
      m_rescache( NULL )
{
    setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
}


QSize
QViewerWidget::minimumSizeHint() const
{
    return QSize( 320, 180 );
}

QSize
QViewerWidget::sizeHint() const
{
    return QSize( 640, 360 );
}

QSizePolicy
QViewerWidget::sizePolicy() const
{
  return QSizePolicy( QSizePolicy::MinimumExpanding,
		      QSizePolicy::MinimumExpanding );
}


void
QViewerWidget::initializeGL()
{
    GLenum error = glewInit();
    if( error != GLEW_OK ) {
        QMessageBox::critical( this,
                               "GLEW failed to initialize",
                               reinterpret_cast<const char*>( glewGetErrorString( error) ),
                               QMessageBox::Abort,
                               QMessageBox::NoButton,
                               QMessageBox::NoButton );
    }


    siut::simd::BBox3f bb( siut::simd::Vec3f(0.f, 0.f, 0.f),
                           siut::simd::Vec3f(1.f, 1.f, 1.f) );
    m_viewer = new siut::dsrv::DSRViewer( siut::simd::BSphere3f( bb ) );
    m_rescache = new siut::gl_utils::ContextResourceCache;

}

void
QViewerWidget::paintGL()
{
  Logger log = getLogger( "QViewerWidget.paintGL" );
  LOGGER_DEBUG( log, "Invoked" );
    m_viewer->update();

    siut::simd::Vec2f ws = m_viewer->getWindowSize();
    glViewport(0, 0, ws[0], ws[1] );


    glClearColor( 0.f, 0.f, 0.f, 0.f );
    glClear( GL_COLOR_BUFFER_BIT );

    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( m_viewer->getModelviewMatrix().c_ptr() );
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( m_viewer->getProjectionMatrix().c_ptr() );
    glMatrixMode( GL_MODELVIEW );

    glColor3f( 1.f, 0.f, 0.f );
    glBegin( GL_TRIANGLES );
    glVertex2f( -0.5, -0.5 );
    glVertex2f( -0.5,  0.5 );
    glVertex2f(  0.5,  0.5 );
    glEnd();

}

void
QViewerWidget::resizeGL( int width, int height )
{
  Logger log = getLogger( "QViewerWidget.paintGL" );
  LOGGER_DEBUG( log, "Resized to ["<<width<<"x"<<height<<"]" );
    glViewport( 0, 0, (GLsizei)width, (GLsizei)height );
    m_viewer->setWindowSize( width, height );
}

void
QViewerWidget::mousePressEvent( QMouseEvent* e )
{
    if( e->button() == Qt::LeftButton ) {
        m_viewer->newState( siut::dsrv::DSRViewer::ROTATE, e->x(), e->y() );
    }
    else if( e->button() == Qt::MidButton ) {
        m_viewer->newState( siut::dsrv::DSRViewer::PAN, e->x(), e->y() );
    }
    else if( e->button() == Qt::RightButton ) {
        m_viewer->newState( siut::dsrv::DSRViewer::ZOOM, e->x(), e->y() );
    }
    updateGL();
}

void
QViewerWidget::mouseMoveEvent( QMouseEvent* e)
{
    m_viewer->motion( e->x(), e->y() );
    updateGL();
}

void
QViewerWidget::mouseReleaseEvent( QMouseEvent* e )
{
    m_viewer->newState( siut::dsrv::DSRViewer::NONE, e->x(), e->y() );
}
