#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <iostream>
#include <ostream>
#include <QRubberBand>
class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT;
public:
    MyGraphicsView(QWidget* parent = NULL);
    QPointF GetCenter() { return CurrentCenterPoint; }
    QPointF offset;
    //Set the current centerpoint in the
    void SetCenter(const QPointF& centerPoint);
     bool isScaleLocked;
     double currentScale;
     bool isMouseOn;
protected:
    //Holds the current centerpoint for the view, used for panning and zooming
    QPointF CurrentCenterPoint;

    //From panning the view
    QPoint LastPanPoint;

    QPoint origin;


    //Take over the interaction
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
     QRubberBand* rubberBand;

};

#endif // MYGRAPHICSVIEW_H
