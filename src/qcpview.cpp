#include <iostream>
#include <string>
#include <list>
#include <QtGui>
#include <QApplication>
#include <QDesktopWidget>
#include <QFile>
#include <QtUiTools>
#include <QFile>
#include <QMetaObject>
#include <QVBoxLayout>
#include "Logger.hpp"
#include "QAppWindow.hpp"
#include "QViewerWidget.hpp"
#include "Project.hpp"

int
main( int argc, char** argv )
{
  initializeLoggingFramework( &argc, argv );

    QApplication app( argc, argv );


    std::list<std::string> files;
    for(int i=1; i<argc; i++) {
        files.push_back( argv[i] );
    }


    Project<float> project( files );


    QUiLoader loader;
    QFile file( "forms/qcpview.ui" );
    QWidget* app_widget = loader.load( &file, NULL );
    file.close();


    QVBoxLayout* viewer_layout = qFindChild<QVBoxLayout*>( app_widget, "viewer_layout" );
    if( viewer_layout == NULL ) {
        QMessageBox::critical( app_widget,
                               "Failed to locate child",
                               "viewer_layout",
                               QMessageBox::Abort,
                               QMessageBox::NoButton,
                               QMessageBox::NoButton );
        return -1;
    }
    QViewerWidget* viewer = new QViewerWidget( app_widget );
    viewer_layout->insertWidget( 1, viewer );

    app_widget->show();
    return app.exec();
}
