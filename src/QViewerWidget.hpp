#pragma once

#include <QSize>
#include <QSizePolicy>
#include <QGLWidget>


namespace siut {
    namespace dsrv {
        class DSRViewer;
    } // of namespace dsrv

    namespace gl_utils {
        class ContextResourceCache;
    } // of namespace gl_utils

} // of namespace siut


class QViewerWidget : public QGLWidget
{
    Q_OBJECT;
public:
    QViewerWidget( QWidget* parent = 0 );

    QSize
    minimumSizeHint() const;

    QSize
    sizeHint() const;

    QSizePolicy
    sizePolicy() const;

protected:

    void
    initializeGL();

    void
    paintGL();

    void
    resizeGL( int width, int height );

    void
    mousePressEvent( QMouseEvent* e );

    void
    mouseMoveEvent( QMouseEvent* e);

    void
    mouseReleaseEvent( QMouseEvent* e );

    siut::dsrv::DSRViewer*                 m_viewer;
    siut::gl_utils::ContextResourceCache*  m_rescache;

};
