#include <QObject>
#include <QWidget>
#include <QtUiTools>
#include <QFile>
#include <QMetaObject>

#include "QAppWindow.hpp"

QAppWindow::QAppWindow(QWidget *parent)
    : QWidget( parent )
{
    QUiLoader loader;
    QFile file( "forms/qcpview.ui");
    file.open( QFile::ReadOnly );
    QWidget* form_widget = loader.load( &file, this );
    file.close();

    QMetaObject::connectSlotsByName( this );

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget( form_widget );
    setLayout( layout );

}

