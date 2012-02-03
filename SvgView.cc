/* This file is part of dotviz.
 * Copyright (C) 2007 Florian Pigorsch <pigorsch@informatik.uni-freiburg.de>
 *
 *  dotviz is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation, version 2.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
*/

#include "SvgView.h"

#include <QtSvg/QSvgRenderer>

#include <QApplication>
#include <QPainter>
#include <QWheelEvent>
#include <QtDebug>
#include <iostream>
#include <ostream>

SvgView::SvgView( QWidget* parent )
    : QWidget( parent ),
      _renderer( new QSvgRenderer( this ) )
{
    _scale = 1.0;
    _translation = QPointF( 0, 0 );

    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
    
    connect( _renderer, SIGNAL( repaintNeeded() ), this, SLOT( update() ) );
}

void SvgView::load( const QString& filename )
{
    if( !( _renderer->load( filename ) ) )
    {
        qDebug() << "Loading SVG from File failed" << endl;
    }

    zoomFit();
}

void SvgView::load( const QByteArray& svgdata )
{
    if( !( _renderer->load( svgdata ) ) )
    {
        qDebug() << "Loading SVG from QByteArray failed" << endl;
    }

    zoomFit();
}

void SvgView::zoomFit()
{
    if( _renderer->isValid() )
    {
        const QRectF& viewbox = _renderer->viewBox();
                
        if( width() * viewbox.height() > height() * viewbox.width() )
        {
            _scale = height() / viewbox.height();
        }
        else
        {
            _scale = width() / viewbox.width();
        }
        
        _translation = QPointF( 0, 0 );
        
        update();
    }
}

void SvgView::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    p.fillRect( rect(), Qt::white );

    if( !( _renderer->isValid() ) ) return;
        
    p.setWindow( rect() );
    
    p.scale( _scale, _scale );
    p.translate( _translation );
    _renderer->render( &p, _renderer->viewBox() );
}

void SvgView::wheelEvent( QWheelEvent* e )
{
    double oldscale = _scale;
    
    if( e->delta() > 0 )
    {
        _scale *= 1.1;
    }
    else
    {
        _scale *= 0.9;
    }
    
    _translation += ( 1.0/_scale - 1.0/oldscale ) * e->pos();
    
    
    update();
}

void SvgView::mousePressEvent(QMouseEvent *event)
{
    mousePressPos = event->pos();
    _startTranslation = _translation;
    
    event->accept();
}

void SvgView::mouseMoveEvent(QMouseEvent *event)
{
    if( mousePressPos.isNull() )
    {
        event->ignore();
        return;
    }

    _translation = _startTranslation + QPointF( event->pos() - mousePressPos ) / _scale;
        
    event->accept();
    update();

}

void SvgView::mouseReleaseEvent(QMouseEvent *event)
{
    mousePressPos = QPoint();
    event->accept();
}

