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

#ifndef SVGVIEW_HH
#define SVGVIEW_HH

class QPaintEvent;
class QSvgRenderer;
class QWheelEvent;
class QMouseEvent;

#include <QtSvg/QtSvg>
#include <QWidget>

class SvgView : public QWidget
{
Q_OBJECT
         
public:
    SvgView( QWidget *parent = 0 );
    virtual ~SvgView() {}
    

public slots:
    void load( const QString& filename );
    void load( const QByteArray& svgdata );
    void zoomFit();
    
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
      
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
      
private:
    double _scale;
    QPointF _translation;
    QPointF _startTranslation;
    
    QPoint mousePressPos;
    
    QSvgRenderer* _renderer;
};

#endif
