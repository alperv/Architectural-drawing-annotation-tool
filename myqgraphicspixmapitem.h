#ifndef MYQGRAPHICSPIXMAPITEM_H
#define MYQGRAPHICSPIXMAPITEM_H

#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QPixmap>

class MyQGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    MyQGraphicsPixmapItem();
    MyQGraphicsPixmapItem(QPixmap map) { this->setPixmap(map);this->acceptHoverEvents(); this->acceptTouchEvents(); this->acceptedMouseButtons();  }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);


};

#endif // MYQGRAPHICSPIXMAPITEM_H
