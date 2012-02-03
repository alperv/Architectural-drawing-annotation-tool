#include "myqgraphicspixmapitem.h"
#include <iostream>
#include <ostream>


using namespace std;
MyQGraphicsPixmapItem::MyQGraphicsPixmapItem()
{
}

void MyQGraphicsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
    cout << "MyQGraphicsPixmapItem MouseMoved!" << endl; cout.flush();
}

void MyQGraphicsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event){

    cout << "MyQGraphicsPixmapItem MousePressed" << endl; cout.flush();
}
