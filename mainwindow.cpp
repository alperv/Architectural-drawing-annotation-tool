#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <iostream>
#include <ostream>
#include <QKeyEvent>
#include <boost/graph/iteration_macros.hpp>
#include <math.h>

using namespace std;
using namespace boost;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    /* Setup SvgView */
    _annotationTabSVGView = new SvgView(this);
    ui->verticalLayout->addWidget(_annotationTabSVGView);
    _viewerTabSVGView = new SvgView(this);
    ui->viewerLayout->addWidget(_viewerTabSVGView);

    _layoutProcess = new QProcess( this );

    connect( _layoutProcess, SIGNAL( finished( int, QProcess::ExitStatus ) ),
             this, SLOT( layoutProcessFinished( int, QProcess::ExitStatus ) ) );



    qApp->installEventFilter(this);



    /* Setup the map views */
    _annotationTabGraphicsView = new MyGraphicsView();
    _viewerTabGraphicsView = new MyGraphicsView();
    _annotationTabGraphicsScene = _annotationTabGraphicsView->scene();
    _viewerTabGraphicsScene = _viewerTabGraphicsView->scene();
    ui->map->addWidget(_annotationTabGraphicsView);
    ui->viewerLayout->addWidget(_viewerTabGraphicsView);
    _viewerTabGraphicsScene->setBackgroundBrush(Qt::white);
    // setImage("/Users/aydemir/cvap6.png");

    QStringList l;
    l.append(QString("CORR"));
    l.append(QString("OFF"));
    m = new QStringListModel;
    m->setStringList(l);
    v = new QListView;
    v->setEditTriggers(QAbstractItemView::NoEditTriggers);
    v->setModel(m);

    std::cout << "cursor at x: " << mousePosX << " "  << mousePosY << std::endl;
    std::cout.flush();
    rubberBand = new QRubberBand(QRubberBand::Rectangle, _annotationTabGraphicsView);
    ui->rooms1->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _annotationTabGraphicsView->show();

    currentMode = GRAPHMODE;
    scaleMouseInterval.first.setX(-1);
    scaleMouseInterval.first.setY(-1);
    scaleMouseInterval.second.setX(-1);
    scaleMouseInterval.second.setY(-1);

    nearestBlackPixel = _annotationTabGraphicsScene->addRect(QRectF(QPointF(0.0, 0.0),QSizeF(0.1,0.1)));

    scaleLine = _annotationTabGraphicsScene->addLine(0,0,0,0);
    drawLayoutLine = _annotationTabGraphicsScene->addLine(0,0,0,0);

    scaleStartPicked= false;
    scaleMouseDistance = -1;
    scaleRealDistance = -1;

    ui->lineTypesComboBox->addItem(QString("Wall"));
    ui->lineTypesComboBox->addItem(QString("Window"));
    ui->lineTypesComboBox->addItem(QString("Portal"));

    lineSegmentStarted = false;



    redPen.setColor( Qt::red);
    redPen.setWidthF(2.5);
    greenPen.setColor(Qt::green);
    greenPen.setWidthF(2.5);
    bluePen.setColor(Qt::blue);
    bluePen.setWidthF(2.5);

    usedPen = redPen;

    moveInVertical = false;
    moveInHorizontal = false;
    env = QProcessEnvironment::systemEnvironment();
    QDir::setCurrent(env.value("HOME"));
    env.insert("PATH", env.value("Path") + ";/opt/local/bin");

}

void MainWindow::setImage(std::string filename){
    QImage image(filename.c_str(), "png");
    QBitmap qmap = QBitmap::fromImage(image);
    sceneItem = new MyQGraphicsPixmapItem(qmap);
    _annotationTabGraphicsScene->addItem(sceneItem);
    sceneItem->grabMouse();

    QRect r = qmap.rect();
    _annotationTabGraphicsView->setSceneRect(r);
    _annotationTabGraphicsView->SetCenter(QPoint(qmap.size().width()/2,qmap.size().height()/2));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove && _annotationTabGraphicsView->isMouseOn)
    {
        //   this->layout()->activate();
        lastMouseEvent = static_cast<QMouseEvent*>(event);
        mousePosX = lastMouseEvent->x();
        mousePosY = lastMouseEvent->y();
        //cout<<"mouse pos: " << mousePosX << " " << mousePosY << endl; cout.flush();

        rubberBand->setGeometry(QRect(origin, lastMouseEvent->pos()).normalized());

        if (currentMode == PICKSCALE){
            if(ui->PickScaleButton->isChecked()){
                if(scaleStartPicked){
                    _annotationTabGraphicsScene->removeItem(scaleLine);
                    QPointF p = QPointF(_annotationTabGraphicsView->mapToScene(mousePosX,mousePosY));
                    if(moveInVertical){
                        scaleLine = _annotationTabGraphicsScene->addLine(scaleFirst.x(),scaleFirst.y(),scaleFirst.x(),p.y(),usedPen);
                    }
                    else if(moveInHorizontal) {
                        scaleLine = _annotationTabGraphicsScene->addLine(scaleFirst.x(),scaleFirst.y(),p.x(),scaleFirst.y(),usedPen);
                    }
                    else {
                        scaleLine = _annotationTabGraphicsScene->addLine(scaleFirst.x(),scaleFirst.y(),p.x(),p.y(),usedPen);
                    }
                }
            }
        }
        else if(currentMode == DRAWLAYOUT && ui->drawRoomOutlineButton->isChecked() &&  _annotationTabGraphicsView->isMouseOn){
            if(lineSegmentStarted){
                _annotationTabGraphicsScene->removeItem(drawLayoutLine);
                QPointF p = QPointF(_annotationTabGraphicsView->mapToScene(mousePosX,mousePosY));
                if(moveInVertical){
                    drawLayoutLine = _annotationTabGraphicsScene->addLine(currentLineSegment.startPos.x(),currentLineSegment.startPos.y(), currentLineSegment.startPos.x(),p.y(),usedPen);
                }
                else if(moveInHorizontal) {
                    drawLayoutLine = _annotationTabGraphicsScene->addLine(currentLineSegment.startPos.x(),currentLineSegment.startPos.y(), p.x(),currentLineSegment.startPos.y(),usedPen);
                }
                else {
                    drawLayoutLine = _annotationTabGraphicsScene->addLine(currentLineSegment.startPos.x(),currentLineSegment.startPos.y(), p.x(),p.y(),usedPen);
                }

            }
        }

        /* This piece of code detects the nearest black point near the pointer in a 100x100 window to guide the layout drawing process
        * In the end, I could not make it work when the zoom level changes and didn't have the patience. Another option was to lock the scale
        * in draw layout mode but this was to constraining.
        */

        /*detectionWindowSize = 100;
        QImage px =  QBitmap::grabWidget(mapview,mousePosX-detectionWindowSize/2, mousePosY-detectionWindowSize/2,detectionWindowSize,detectionWindowSize).toImage();
        px.save("/Users/Aydemir/lala.png");
        QPointF p = QPointF(mapview->mapToScene(mousePosX-detectionWindowSize/2,mousePosY-detectionWindowSize/2));
        cout<<"converted mouse pos: " << p.x() << " " << p.y() << endl; cout.flush();
        int lowi,lowj;
        double distance = 999;
        for (int i=0; i < px.width(); i++){
            for (int j=0; j<px.height(); j++){
                QRgb tmppixel = px.pixel(i,j);
                if(qGray(tmppixel) < 10){
                   double d1 = sqrt( std::pow((i - detectionWindowSize/2)+0.0001,2) + std::pow((j - detectionWindowSize/2)+0.0001,2));
                   if(d1<distance){
                       distance = d1;
                       lowi = i;
                       lowj = j;
                       //cout<<"black pixel at: " << i << " " << j << endl; cout.flush();
                   }
                }
            }
        }
        if(nearestBlackPixel != 0){
            scene->removeItem(nearestBlackPixel);
        }
        QPointF p1 = QPointF(mapview->mapToScene(lowi,lowj));
        cout<<"offset pos: " << mapview->offset.x() << " " << mapview->offset.y() << endl; cout.flush();
           nearestBlackPixel = scene->addRect(QRectF(QPointF(p.x()+p1.x(), p.y()+p1.y()),QSizeF(2,2)),b);*/
    }

    else if(event->type() == QEvent::MouseButtonPress && _annotationTabGraphicsView->isMouseOn)
    {
        QMouseEvent * mouseEvent = static_cast <QMouseEvent *> (event);

        if (mouseEvent->button() == Qt::RightButton && _annotationTabGraphicsView->isMouseOn){
            if (currentMode == GRAPHMODE) {
                origin = mouseEvent->pos(); // + mapview->pos();
                rubberBand->setGeometry(QRect(origin, QSize()));
                rubberBand->show();
            }
            else if (currentMode == PICKSCALE){
                if(ui->PickScaleButton->isChecked()){
                    QPointF p = QPointF(_annotationTabGraphicsView->mapToScene(mousePosX,mousePosY));
                    if(!scaleStartPicked){
                        scaleFirst = p;
                        scaleLast = p;
                        scaleStartPicked = true;
                    }
                    else{

                        if(moveInVertical){
                            scaleLast = QPointF(scaleFirst.x(),p.y());
                        }
                        else if(moveInHorizontal) {
                            scaleLast = QPointF(p.x(),scaleFirst.y());
                        }
                        else {
                            scaleLast = p;
                        }

                        scaleMouseDistance = sqrt( std::pow((scaleLast.x() - scaleFirst.x()+0.000000001),2) + std::pow((scaleLast.y() - scaleFirst.y()+0.000000001),2));
                        ui->mouseDistance->setText(QString("%1").arg(scaleMouseDistance));
                        scaleStartPicked = false;
                        ui->PickScaleButton->setChecked(false);
                    }

                }
            }
            else if(currentMode == DRAWLAYOUT && ui->drawRoomOutlineButton->isChecked() && !ui->roomsComboBox->currentText().contains(QString("Select Room"))){
                QPointF p = QPointF(_annotationTabGraphicsView->mapToScene(mousePosX,mousePosY));
                if(!lineSegmentStarted){
                    cout << "setting line segment start!" << endl; cout.flush();
                    currentLineSegment.startPos = p;
                    lineSegmentStarted=true;
                }
                else{
                    //   lineSegmentStarted=false;
                    cout << "setting line segment end!" << endl; cout.flush();

                    if(moveInVertical){
                        currentLineSegment.endPos = QPointF(currentLineSegment.startPos.x(),p.y());
                    }
                    else if(moveInHorizontal) {
                        currentLineSegment.endPos = QPointF(p.x(),currentLineSegment.startPos.y());
                    }
                    else {
                        currentLineSegment.endPos = p;
                    }
                    _annotationTabGraphicsScene->removeItem(drawLayoutLine);

                    /* If the user has picked the room to be traced */
                    if(!ui->roomsComboBox->currentText().contains("Select Room"))
                    {
                        /* If this is not a portal corner we add it */
                        if (!ui->lineTypesComboBox->currentText().contains("Portal")){
                            currentLineSegment.sceneItem = _annotationTabGraphicsScene->addLine(currentLineSegment.startPos.x(),currentLineSegment.startPos.y(), currentLineSegment.endPos.x(),currentLineSegment.endPos.y(),usedPen);
                            currentLineSegment.type = ui->lineTypesComboBox->currentText().toStdString();
                            currentRoomLayout.push_back(currentLineSegment);
                            currentLineSegment.startPos = currentLineSegment.endPos;
                            //      lineSegmentStarted = true;
                        }
                        /* if this is a portal we check if the user has selected a destination for this portal */
                        else {
                            if(!ui->portalToRoomComboBox->currentText().contains("Select portal destination")){
                                currentLineSegment.sceneItem = _annotationTabGraphicsScene->addLine(currentLineSegment.startPos.x(),currentLineSegment.startPos.y(), currentLineSegment.endPos.x(),currentLineSegment.endPos.y(),usedPen);
                                currentLineSegment.type = ui->lineTypesComboBox->currentText().toStdString();
                                currentLineSegment.portalToRoom =ui->portalToRoomComboBox->currentText().toStdString();
                                currentRoomLayout.push_back(currentLineSegment);
                                currentLineSegment.startPos = currentLineSegment.endPos;
                                //       lineSegmentStarted = true;
                            }
                            else {
                                statusBar()->showMessage("You haven't selected a portal destination!");
                            }
                        }
                    }
                    else {
                        statusBar()->showMessage("You haven't selected a room!");
                    }
                }
            }
        }
    }

    else if(event->type() == QEvent::MouseButtonRelease && _annotationTabGraphicsView->isMouseOn){
        QMouseEvent * mouseEvent = static_cast <QMouseEvent *> (event);
        if (mouseEvent -> button() == Qt::RightButton && currentMode == GRAPHMODE) {
            std::cout << "right button release!" << std::endl;
            std::cout.flush();
            rubberBand->hide();
            if(rubberBand->width()*rubberBand->height() < 5){
                cout<< "returning!" << endl; cout.flush(); return false;
            }

            std::cout << rubberBand->x() << " " << rubberBand->y() << std::endl;
            std::cout.flush();

            QPen b;
            b.setColor( Qt::darkBlue);

            QRectF visibleArea = _annotationTabGraphicsView->mapToScene(_annotationTabGraphicsView->rect()).boundingRect();
            QRectF r(QPoint(_annotationTabGraphicsView->mapToScene(rubberBand->geometry()).boundingRect().x(),_annotationTabGraphicsView->mapToScene(rubberBand->geometry()).boundingRect().y()), _annotationTabGraphicsView->mapToScene(rubberBand->geometry().bottomRight()));

            // Take a screenshot of the selected area
            // This needs to be converted to a Bitmap because otherwise cuneiform does not run on other types of images.
            QBitmap px;
            px =  QBitmap::grabWidget(_annotationTabGraphicsView, rubberBand->x(),rubberBand->y(),rubberBand->width(),rubberBand->height());

            // Draw rectangle for indicating where is already added to the graph
            QGraphicsRectItem* tmp = _annotationTabGraphicsScene->addRect(r,b);
            sceneRects.push_back(tmp);


            // Run the OCR process\

            px.save("myimage.png","png");
            QProcess cuneiform;
            cuneiform.setProcessEnvironment(env);

            cuneiform.start("/opt/local/bin/cuneiform", QStringList() << "-l eng" << "myimage.png");
            cuneiform.waitForFinished();

            QString result = cuneiform.readAllStandardOutput();
            if (result.contains("abort") && !result.contains("Cuneiform for Linux")){
                statusBar()->showMessage("Something went wrong with OCR!");
            }
            cuneiform.start("more", QStringList() << "cuneiform-out.txt");
            cuneiform.waitForFinished();
            result = cuneiform.readAllStandardOutput();

            if(result.trimmed().isEmpty() || result.contains("failed")){
                statusBar()->showMessage("OCR failed to read the text (or you've selected a bad area).");
                std::cout << result.toStdString() << std::endl;
            }
            else{
                statusBar()->showMessage(QString("Raw OCR output: %1").arg(result));

                std::cout << result.toStdString() << std::endl;
                std::cout.flush();
                QStringList resultList = result.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                if (resultList.size() > 0){
                    QString numberPart,categoryPart;

                    // numberPart always comes first
                    numberPart = resultList.at(0);
                    resultList.removeAt(0);

                    // Sometimes there's a letter with the number part e.g. 176 B
                    if(!resultList.isEmpty() && !resultList.at(0).isNull())
                    {
                        if(resultList.at(0).trimmed().size() == 1){
                            cout<< " aaa: " << resultList.at(0).toStdString() << endl; cout.flush();
                            numberPart.append(resultList.at(0));
                            resultList.removeAt(0);
                        }
                    }

                    // The rest is deemed as the category
                    foreach(QString str, resultList){
                        categoryPart.append(str);
                    }
                    if(categoryPart.isEmpty()){
                        categoryPart = "";
                    }

                    ui->roomNumberTextBox->setText(numberPart.trimmed());
                    ui->roomCatTextBox->setText(categoryPart.trimmed());
                }
            }
        }
    }

    return false;
}

void MainWindow::keyPressEvent(QKeyEvent* k)
{
    // cout << "pressed key" << endl; cout.flush();
    switch ( tolower(k->key()) ) {
    case 'a':                               // Add new node
        on_addRoomButton_clicked();
        break;
    case 'w':
        //  cout<<"pressed w!" << endl; cout.flush();
        writeGraphToXML();
        break;
    case 'u':
        if(currentMode== GRAPHMODE){
            if(sceneRects.size()!=0){
                _annotationTabGraphicsScene->removeItem(sceneRects.back());
                sceneRects.erase(sceneRects.begin()+(sceneRects.size()-1));
            }
        }
        else if (currentMode == DRAWLAYOUT){
            if(ui->drawRoomOutlineButton->isChecked() && currentRoomLayout.size() > 0){
                currentLineSegment.startPos = currentRoomLayout.back().startPos;
                _annotationTabGraphicsScene->removeItem(currentRoomLayout.back().sceneItem);
                currentRoomLayout.erase(currentRoomLayout.begin()+(currentRoomLayout.size()-1));
                if (!lineSegmentStarted){
                    lineSegmentStarted = true;
                }
            }
        }
        break;
    case 'v':
        if(moveInHorizontal){
            moveInHorizontal = !moveInHorizontal;
        }
        moveInVertical = !moveInVertical;
        if(moveInVertical){
            statusBar()->showMessage(QString("Mouse pointer in vertical mode"));
        }
        else{
            statusBar()->showMessage(QString("Mouse pointer not in vertical mode"));
        }
        break;
    case 'h':
        if(moveInVertical){
            moveInVertical = !moveInVertical;
        }
        moveInHorizontal = !moveInHorizontal;

        if(moveInVertical){
            statusBar()->showMessage(QString("Mouse pointer in horizontal mode"));
        }
        else{
            statusBar()->showMessage(QString("Mouse pointer not in horizontal mode"));
        }
        break;
    case 'p':
        ui->lineTypesComboBox->setCurrentIndex(2);
        break;
    case 'o':
        ui->lineTypesComboBox->setCurrentIndex(1);
        break;
    case 'l':
        ui->lineTypesComboBox->setCurrentIndex(0);
        break;
    case 't':
        if (currentRoomLayout.size() != 0){
            currentLineSegment.endPos = currentRoomLayout[0].startPos;
            currentLineSegment.sceneItem = _annotationTabGraphicsScene->addLine(currentLineSegment.startPos.x(),currentLineSegment.startPos.y(), currentLineSegment.endPos.x(),currentLineSegment.endPos.y(),usedPen);
            currentLineSegment.type = ui->lineTypesComboBox->currentText().toStdString();
            currentRoomLayout.push_back(currentLineSegment);
            lineSegmentStarted=false;
            _annotationTabGraphicsScene->removeItem(drawLayoutLine);
            //ui->addRoomLayout->click();
        }
        break;
    }
}

bool MainWindow::addVertex(std::string category, std::string id, FloorPlanGraph& g){
    bool vertexExists = false;

    graph_traits<FloorPlanGraph>::vertex_iterator vi, vi_end;
    for (tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi){
        if(id.compare(g.graph()[*vi].vertex_id) == 0){
            vertexExists = true;
        }
    }


    if(vertexExists){
        //cout<< "this vertex exists!" << endl; cout.flush();
        return false;
    }

    MyVertex u = boost::add_vertex(id, g);
    if(u == g.null_vertex()){
        cout << "null!" << endl; cout.flush();
    }

    g[id].category = category;

    g[id].vertex_id =id;
    //  cout << "vertex id: " << g.graph()[u].vertex_id << endl; cout.flush();
    //  cout<< "after there are " << numberofCurrentVertices() << "vertices" << endl; cout.flush();
    return true;
}

void MainWindow::addEdge(MyVertex u, MyVertex v, FloorPlanGraph& g){
    MyEdge e;

    bool added;
    boost::tie(e,added) = boost::add_edge(u,v,g);
    g.graph()[e].edge_id = g.graph()[u].vertex_id+g.graph()[v].vertex_id;
    refreshGraph(_annotationGraph, _annotationTabSVGView);
}

void MainWindow::writeGraph(FloorPlanGraph& g, std::ostream &out){
    // This doesn't work for listS edge lists for some reason hence the "homemade" alternative
    // boost::write_graphviz(out,g,Room_Writer(g), RoomEdge_Writer(g));
}

void MainWindow::writeGraphvizHomeMade(FloorPlanGraph& g, std::ostream &out){
    out << "graph TopoGraph {" << endl;
    out << "overlap=false; splines=true;" << endl;
    BGL_FORALL_VERTICES(v, g, FloorPlanGraph){
        out << g.graph()[v].vertex_id << " [shape=ellipse" << " label=\"" << g.graph()[v].vertex_id << " - " << g.graph()[v].category << "\"];" << std::endl;
    }


    BGL_FORALL_EDGES(e, g, FloorPlanGraph){
        out << g.graph()[source(e, g)].vertex_id << " -- " << g.graph()[target(e, g)].vertex_id << ";" << std::endl; cout.flush();
    }

    out <<"}";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openFolderButton_clicked()
{
    /* clear everything */
    FloorPlanGraph tmp;
    _annotationGraph= tmp;
    currentRoomLayout.clear();
    ui->rooms1->clear();
    ui->rooms2->clear();
    ui->buildingNamelineEdit->clear();
    ui->roomCatTextBox->clear();
    ui->roomNumberTextBox->clear();
    ui->mouseDistance->clear();
    ui->scaleInterval2->clear();
    ui->setScaleButton->setEnabled(true);
    ui->graphMode->click();
    ui->portalToRoomComboBox->clear();
    scaleStartPicked = false;
    lineSegmentStarted = false;
    _annotationTabGraphicsScene->clear();
    lastSavedFileName.clear();
    _annotationTabSVGView->load(QByteArray());
    filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                            env.value("HOME"),
                                            tr("Image Files (*.png *.jpg *.bmp)"));
    QStringList slist = filename.split("/");
    if (slist.size() >1){
        buildingName = slist.at(slist.size()-2);
        ui->buildingNamelineEdit->setText(buildingName);
    }
    std::cout << filename.toStdString() << std::endl;
    std::cout.flush();
    setImage(filename.toStdString());
    scaleStartPicked=false;
    _annotationTabGraphicsView->show();


}


void MainWindow::on_addRoomButton_clicked()
{
    if(addVertex(ui->roomCatTextBox->text().toStdString(), buildingName.toStdString() + "_" + ui->roomNumberTextBox->text().toStdString(),_annotationGraph)){
        statusBar()->showMessage(QString("Added room with id %1 and category %2").arg(ui->roomNumberTextBox->text(), ui->roomCatTextBox->text()));
    }
    else {
        statusBar()->showMessage("Cannot add vertex, possibly another vertex with the same id exists already!");
    }
    updateRoomListView();
    writeGraphToXML(true);

}

void MainWindow::updateRoomListView(){
    ui->rooms1->clear();
    ui->rooms2->clear();

    graph_traits<FloorPlanGraph>::vertex_iterator vi, vi_end;
    for (tie(vi, vi_end) = vertices(_annotationGraph); vi != vi_end; ++vi){

        ui->rooms1->addItem(QString((_annotationGraph.graph()[*vi].vertex_id + " - " + _annotationGraph.graph()[*vi].category).c_str()));
        ui->rooms2->addItem(QString((_annotationGraph.graph()[*vi].vertex_id + " - " + _annotationGraph.graph()[*vi].category).c_str()));

    }
    refreshGraph(_annotationGraph, _annotationTabSVGView);
}

//void parseSpace(QXmlStreamReader& xml){
//    if(xml.tokenType() != QXmlStreamReader::StartElement &&
//            xml.name() == "space") {
//        return;
//    }

//    /* Next element... */
//    xml.readNext();
//    while(!(xml.tokenType() == QXmlStreamReader::EndElement &&
//            xml.name() == "floor")) {
//        if(xml.tokenType() == QXmlStreamReader::StartElement) {
//            xml.attributes().h
//        }
//    }
//}


void MainWindow::on_loadFromXMLpushButton_clicked()
{

    QString XMLfilename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                       env.value("HOME"),
                                                       tr("XML Files (*.xml)"));
    loadFromXML(XMLfilename, _annotationGraph);
    drawGraphLayout(_annotationGraph,_annotationTabGraphicsScene);
    updateRoomListView();
    refreshGraph(_annotationGraph, _annotationTabSVGView);
    ui->buildingNamelineEdit->setText(buildingName);

}

QPointF MainWindow::calculateRoomCentroid(std::vector<LineSegment> roomLayout){
    if (roomLayout.size() == 0){
        return QPointF(0,0);
    }

    QPointF centroid;
    vector<QPointF> vertices;

    vertices.push_back(roomLayout[0].startPos);
    for(int i=0; i < roomLayout.size()-1; i++){
        vertices.push_back(roomLayout[i].endPos);
    }

    double signedArea = 0.0;
    double x0 = 0.0; // Current vertex X
    double y0 = 0.0; // Current vertex Y
    double x1 = 0.0; // Next vertex X
    double y1 = 0.0; // Next vertex Y
    double a = 0.0;  // Partial signed area

    // For all vertices except last
    int i=0;
    for (i=0; i<vertices.size()-1; ++i)
    {
        x0 = vertices[i].x();
        y0 = vertices[i].y();
        x1 = vertices[i+1].x();
        y1 = vertices[i+1].y();
        a = x0*y1 - x1*y0;
        signedArea += a;
        centroid.setX(centroid.x() + (x0 + x1)*a);
        centroid.setY( centroid.y() + (y0 + y1)*a);
    }
    // Do last vertex
    x0 = vertices[i].x();
    y0 = vertices[i].y();
    x1 = vertices[0].x();
    y1 = vertices[0].y();
    a = x0*y1 - x1*y0;
    signedArea += a;
    centroid.setX( centroid.x() + (x0 + x1)*a);
    centroid.setY(centroid.y() + (y0 + y1)*a);

    signedArea *= 0.5;
    centroid.setX( centroid.x() / (6*signedArea));
    centroid.setY( centroid.y() / (6*signedArea));
    return centroid;

}

void MainWindow::drawGraphLayout(FloorPlanGraph& g, QGraphicsScene* scene){
    std::vector<LineSegment> roomLayout;
    BGL_FORALL_VERTICES(v, g, FloorPlanGraph){
        roomLayout = g.graph()[v].roomLayout;
        foreach(LineSegment l, roomLayout){
            double linelength = sqrt( std::pow((l.startPos.x() - l.endPos.x()+0.000000001),2) + std::pow((l.startPos.y() - l.endPos.y()+0.000000001),2));
            cout << "line at (" << l.startPos.x() << "," << l.startPos.y() << ") - " << "(" << l.endPos.x() << "," << l.endPos.y() << ")" << endl; cout.flush();
            if(linelength > 0.1){
                if (l.type.compare("Portal") == 0){
                    l.sceneItem = scene->addLine(l.startPos.x(), l.startPos.y(),l.endPos.x(),l.endPos.y(),greenPen);
                }
                else if(l.type.compare("Window") == 0){
                    l.sceneItem = scene->addLine(l.startPos.x(), l.startPos.y(),l.endPos.x(),l.endPos.y(),bluePen);
                }
                else if(l.type.compare("Wall") == 0){
                    l.sceneItem = scene->addLine(l.startPos.x(), l.startPos.y(),l.endPos.x(),l.endPos.y(),redPen);
                }
                else {
                    l.sceneItem = scene->addLine(l.startPos.x(), l.startPos.y(),l.endPos.x(),l.endPos.y(),QPen(QColor(Qt::black),2.5));
                }
            }
            QRectF tmprect(QPointF(g.graph()[v].centroid.x() -5,g.graph()[v].centroid.y() -5 ), QSize(10,10));
            cout << "Centroid at" << g.graph()[v].centroid.x() << "," << g.graph()[v].centroid.y() << endl;
            g.graph()[v].centroidSceneItem = scene->addRect(tmprect, usedPen);
        }
    }

    drawLabels(g, scene);

}


void MainWindow::on_showPortalConnectionscheckBox_clicked(bool checked)
{
    if(checked){
        showPortalConnections(_annotationGraph, _annotationTabGraphicsScene);
    }
    else{
        for(int i=0; i < _portalConnectionLines.size(); i++){
            _annotationTabGraphicsScene->removeItem(_portalConnectionLines[i]);
        }
    }
}

void MainWindow::showPortalConnections(FloorPlanGraph g, QGraphicsScene* scene){
    std::vector<LineSegment> roomLayout;
    BGL_FORALL_VERTICES(v, g, FloorPlanGraph){
        roomLayout = g.graph()[v].roomLayout;
        foreach(LineSegment l, roomLayout){
            if (l.type.compare("Portal") == 0){
                QPointF middlePortalPoint = (l.startPos +l.endPos)/2;
                _portalConnectionLines.push_back(scene->addLine(middlePortalPoint.x(), middlePortalPoint.y(), g[l.portalToRoom].centroid.x(),g[l.portalToRoom].centroid.y()));
            }
        }
    }
}

void MainWindow::drawLabels(FloorPlanGraph g, QGraphicsScene* scene){
    QFont f;
    f.setPointSize(30);
    BGL_FORALL_VERTICES(v, g, FloorPlanGraph){
      //  QGraphicsTextItem* tmp = scene->addText(QString((g.graph()[v].vertex_id + " - " + g.graph()[v].category).c_str()),f);
      QGraphicsTextItem* tmp = scene->addText(QString((g.graph()[v].category).c_str()),f);

        _spaceTexts.push_back(tmp);
        tmp->setPos(g.graph()[v].centroid.x()-5, (g.graph()[v].centroid.y()-5));
    }
}

void MainWindow::loadFromXML(QString filename, FloorPlanGraph& g){

    QFile* file = new QFile(filename);
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this,
                              "QXSRExample::parseXML",
                              "Couldn't open file",
                              QMessageBox::Ok);
        return;
    }


    QXmlStreamReader xml(file);
    vector<pair<std::string, string> > xmledges;
    string id;
    bool hasPointAsLayout = false;
    double scaleFactor = 10;

    while(!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        //   cout << __LINE__ << " " << xml.name().toString().toStdString() << endl; cout.flush();
        if(token == QXmlStreamReader::StartDocument) {
            continue;
        }
        /* If token is StartElement, we'll see if we can read it.*/
        if(token == QXmlStreamReader::StartElement) {
            /* If it's named persons, we'll go to the next.*/
            if(xml.name() == "floor" && xml.attributes().value("name").toString().compare("FLOORCONTOUR") != 0) {
                buildingName= xml.attributes().value("BuildingName").toString();
            }
            else if (xml.name() == "floor" && xml.attributes().value("name").toString().compare("FLOORCONTOUR") == 0){
                _currentGraphProperties.maxx =  xml.attributes().value("maxx").toString().toDouble();
                _currentGraphProperties.maxy =  xml.attributes().value("maxy").toString().toDouble();
                _currentGraphProperties.minx =  xml.attributes().value("minx").toString().toDouble();
                _currentGraphProperties.miny =  xml.attributes().value("miny").toString().toDouble();
            }

            if(xml.name() == "Scale"){
                scaleMouseDistance=  xml.attributes().value("PixelDistance").toString().toDouble();
                scaleRealDistance = xml.attributes().value("RealDistance").toString().toDouble();
                ui->mouseDistance->setText(QString("%1").arg(scaleMouseDistance));
                ui->scaleInterval2->setText(QString("%1").arg(scaleRealDistance));
                on_setScaleButton_clicked();
            }
            if (xml.name() == "space"){
                vector<QPointF> points;
                vector<LineSegment > tmpsegment;
                string cat = xml.attributes().value("type").toString().toStdString();
                string id = xml.attributes().value("name").toString().toStdString();
                if(id.size() > 0 && QString(id.c_str()).at(0).isDigit()){
                    id = "S" + id;
                }
                if(QString(id.c_str()).contains("-")){
                    id = QString(id.c_str()).replace("-","_").toStdString();
                }
                if(QString(id.c_str()).contains(".")){
                    // id = QString(id.c_str()).replace(".","").toStdString();
                    id = QString(id.c_str()).split(".").at(0).toStdString();

                }
                QString q(id.c_str());
                q = q.remove("*");
                q = q.remove("&");
                id = q.toStdString();

                //   cout << "adding space of id " << id << "and type " << cat << endl; cout.flush();
                addVertex(cat, id, g);
                while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "space")){
                    xml.readNext();
                    points.clear();
                    if (xml.name() == "contour"){
                        while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "contour")){
                            if(xml.name() == "linesegment" && xml.tokenType() == QXmlStreamReader::StartElement){
                                // cout << xml.name().toString().toStdString() << " " << xml.attributes().value("x1").toString().toStdString() << endl; cout.flush();
                                LineSegment l(QPointF(xml.attributes().value("x1").toString().toDouble(),
                                                      xml.attributes().value("y1").toString().toDouble()),
                                              QPointF(xml.attributes().value("x2").toString().toDouble(),
                                                      xml.attributes().value("y2").toString().toDouble())
                                              );
                                l.type = xml.attributes().value("type").toString().toStdString();
                                if (l.type.compare("Portal") == 0){
                                    l.portalToRoom = xml.attributes().value("target").toString().toStdString();
                                }

                                tmpsegment.push_back(l);
                            }

                            if(xml.name() == "point" && xml.tokenType() == QXmlStreamReader::StartElement){
                                hasPointAsLayout=true;
                                points.push_back(QPointF(xml.attributes().value("x").toString().toDouble()*scaleFactor,xml.attributes().value("y").toString().toDouble()*scaleFactor));

                            }


                            if(xml.name() == "centroid" && xml.tokenType() == QXmlStreamReader::StartElement){
                                //cout << numberofCurrentVertices() << endl; cout.flush();
                                if(hasPointAsLayout){
                                    g[id].centroid.setX(xml.attributes().value("x").toString().toDouble()*scaleFactor);
                                    g[id].centroid.setY(xml.attributes().value("y").toString().toDouble()*scaleFactor);
                                }
                                else{
                                    g[id].centroid.setX(xml.attributes().value("x").toString().toDouble());
                                    g[id].centroid.setY(xml.attributes().value("y").toString().toDouble());
                                }

                            }
                            xml.readNext();
                        }
                    }
                    if(xml.name() == "portal" && xml.tokenType() == QXmlStreamReader::StartElement){
                        string targetid = xml.attributes().value("target").toString().toStdString();
                        if(targetid.size() > 0 && QString(targetid.c_str()).at(0).isDigit()){
                            targetid = "S" + targetid;
                        }
                        if(QString(targetid.c_str()).contains("-")){
                            targetid = QString(targetid.c_str()).replace("-","_").toStdString();
                        }
                        if(QString(targetid.c_str()).contains(".")){
                            //  targetid = QString(targetid.c_str()).replace(".","").toStdString();
                            targetid = QString(id.c_str()).split(".").at(0).toStdString();

                        }
                        xmledges.push_back(make_pair(targetid,id));

                    }
                    if(!hasPointAsLayout){
                        g[id].roomLayout = tmpsegment;
                    }
                    else{
                        //  cout << "room " << id <<  " has " << points.size() << " points" << endl; cout.flush();

                        if(points.size() > 0){
                            LineSegment l;
                            for (int i=0; i < points.size(); i++){
                                if(i == points.size()-1){
                                    l.startPos = points[i];
                                    l.endPos = points[0];
                                }
                                else{
                                    l.startPos = points[i];
                                    l.endPos = points[i+1];
                                }
                                //   cout << "line at (" << l.startPos.x() << "," << l.startPos.y() << ") - " << "(" << l.endPos.x() << "," << l.endPos.y() << ")" << endl; cout.flush();
                                g[id].roomLayout.push_back(l);
                            }
                        }
                    }
                }

            }
        }
    }

    for (int i =0; i < xmledges.size(); i++){
        // cout<< "checking for " << xmledges[i].first << " to " << xmledges[i].second << endl; cout.flush();
        if (!isAlreadyConnected(xmledges[i].first, xmledges[i].second) &&
                doesVertexExists(xmledges[i].first,g) && doesVertexExists(xmledges[i].first,g)){
            //   cout<< "connecting " << xmledges[i].first << " to " << xmledges[i].second << endl; cout.flush();
            add_edge_by_label(xmledges[i].first, xmledges[i].second, g);
        }
    }

}


void MainWindow::writeGraphToXML(bool isBackup){
    std::string XMLfilename;
    if(!isBackup){
        XMLfilename = (filename.split(".").at(0).toStdString()  +".xml");
    }
    else{
        if(!lastSavedFileName.isEmpty()){
            QFile::remove(lastSavedFileName);
        }
        QDateTime dateTime = QDateTime::currentDateTime();
        std::string dateTimeString = dateTime.toString().replace(" ", "_").toStdString();
        XMLfilename = (filename.split(".").at(0).toStdString() + "_tmp_" + dateTimeString+  ".xml");
        lastSavedFileName = QString(XMLfilename.c_str());
        //XMLfilename = (filename.split(".").at(0).toStdString() + "_tmp_" +  ".xml");
    }
    QFile file(XMLfilename.c_str());
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(0, "Read only", "The file is in read only mode");
    }
    else if((scaleMouseDistance == -1 || scaleRealDistance == -1) && !isBackup){
        statusBar()->showMessage("Cannot write XML file, specify scale!");

    }
    else
    {
        cout << "Writing to XML file!" << endl; cout.flush();
        QXmlStreamWriter* xmlWriter = new QXmlStreamWriter();
        xmlWriter->setDevice(&file);
        xmlWriter->setAutoFormatting(true);


        xmlWriter->writeStartDocument();

        /* Open floor tag */
        xmlWriter->writeStartElement("floor");
        xmlWriter->writeAttribute("BuildingName", buildingName);
        //xmlWriter->writeAttribute("FloorName",filename.split(".").at(0));
        xmlWriter->writeAttribute("FloorName",filename.split("/").back().split(".").at(0));
        xmlWriter->writeStartElement("Domain");
        xmlWriter->writeAttribute("name", "KTH");
        xmlWriter->writeEndElement();

        /* Write the scale information */
        xmlWriter->writeStartElement("Scale");
        xmlWriter->writeAttribute("PixelDistance", QString("%1").arg(scaleMouseDistance));
        xmlWriter->writeAttribute("RealDistance", QString("%1").arg(scaleRealDistance));
        xmlWriter->writeEndElement();


        /* Now write spaces one by one */
        BGL_FORALL_VERTICES(v, _annotationGraph, FloorPlanGraph){
            xmlWriter->writeStartElement("space");
            xmlWriter->writeAttribute(QString("name"), QString(_annotationGraph.graph()[v].vertex_id.c_str()));
            xmlWriter->writeAttribute(QString("type"),  QString(_annotationGraph.graph()[v].category.c_str()));

            xmlWriter->writeStartElement("contour");
            xmlWriter->writeStartElement("centroid");
            xmlWriter->writeAttribute(QString("x"), QString("%1").arg(_annotationGraph.graph()[v].centroid.x()));
            xmlWriter->writeAttribute(QString("y"), QString("%1").arg(_annotationGraph.graph()[v].centroid.y()));
            xmlWriter->writeEndElement();
            foreach(LineSegment l, _annotationGraph.graph()[v].roomLayout){

                xmlWriter->writeStartElement("linesegment");
                xmlWriter->writeAttribute(QString("x1"), QString("%1").arg(l.startPos.x()));
                xmlWriter->writeAttribute(QString("y1"), QString("%1").arg(l.startPos.y()));

                xmlWriter->writeAttribute(QString("x2"), QString("%1").arg(l.endPos.x()));
                xmlWriter->writeAttribute(QString("y2"), QString("%1").arg(l.endPos.y()));

                xmlWriter->writeAttribute(QString("type"), l.type.c_str());
                if(l.type.compare("Portal") == 0){
                    xmlWriter->writeAttribute(QString("target"), l.portalToRoom.c_str());
                }
                else{
                    xmlWriter->writeAttribute(QString("target"), "");
                }
                xmlWriter->writeEndElement();

            }
            xmlWriter->writeEndElement();

            BGL_FORALL_OUTEDGES(v, e, _annotationGraph, FloorPlanGraph) {
                xmlWriter->writeStartElement("portal");
                xmlWriter->writeAttribute(QString("target"), QString(_annotationGraph.graph()[target(e, _annotationGraph)].vertex_id.c_str()));
                // std::cout << source(e, g) << " -> " << target(e, g) << std::endl; cout.flush();
                // std::cout << g.graph()[source(e, g)].category << " -> " << target(e, g) << std::endl; cout.flush();
                xmlWriter->writeEndElement();
            }

            xmlWriter->writeEndElement();
        }
        xmlWriter->writeEndElement();

        xmlWriter->writeEndDocument();
        statusBar()->showMessage(QString("Wrote file %1").arg(XMLfilename.c_str()));
        delete xmlWriter;

        file.close();

        QFile file2(XMLfilename.c_str());
        loadXMLtoDOM(file2);
    }
}

bool MainWindow::doesVertexExists(string id, FloorPlanGraph& g){
    BGL_FORALL_VERTICES(v, g, FloorPlanGraph){
        if(_annotationGraph.graph()[v].vertex_id.compare(id) == 0){
            return true;
        }
    }
    return false;
}

void MainWindow::loadXMLtoDOM(QFile& file){
    QDomDocument doc("mydocument");
    if (!file.open(QIODevice::ReadOnly)){
        cout<< "file open error"<< endl;
        return;
    }
    if (!doc.setContent(&file)) {
        cout << "set content error" << endl;
        file.close();
        return;
    }
    file.close();

    //    QFrame* popup1 = new QFrame(this, Qt::Popup | Qt::Window );
    //    QTextEdit* mytree = new QTextEdit(popup1);
    //    mytree->setText(doc.toString());
    //    popup1->show();
    //    popup1->resize(mytree->geometry().width(), mytree->geometry().height());
    //    mytree->show();
}

MyVertex MainWindow::findVertex(std::string s){
    MyVertex ret;
    BGL_FORALL_VERTICES(v, _annotationGraph, FloorPlanGraph){
        if(_annotationGraph.graph()[v].vertex_id.compare(s) == 0){
            ret = v;
        }
    }
    return ret;
}

bool MainWindow::isAlreadyConnected(std::string v1, std::string v2){

    BGL_FORALL_VERTICES(v, _annotationGraph, FloorPlanGraph){
        if(_annotationGraph.graph()[v].vertex_id.compare(v1) == 0){
            //  cout<< "for all edges of " << g.graph()[v].vertex_id << endl; cout.flush();
            BGL_FORALL_OUTEDGES(v, e, _annotationGraph, FloorPlanGraph) {
                std::string s = _annotationGraph.graph()[target(e, _annotationGraph)].vertex_id;
                //    cout << "edge target is " << s << endl;  cout.flush();
                if ( s.compare(v2) == 0){
                    //      cout << "they are connected" << endl;  cout.flush();
                    return true;
                }
            }
        }
    }
    return false;
}

void MainWindow::on_connectVertices_clicked()
{
    if(ui->rooms1->selectedItems().size() == 0 || ui->rooms2->selectedItems().size() == 0){
        statusBar()->showMessage("No room is selected!");
        return;
    }
    QString toBeConnectedId =ui->rooms2->selectedItems().at(0)->data(Qt::DisplayRole).toString().split(" - ").at(0);

    foreach(QListWidgetItem* item, ui->rooms1->selectedItems()){
        QString s = item->data(Qt::DisplayRole).toString().split(" - ").at(0); // get the id
        if(!isAlreadyConnected(s.toStdString(),toBeConnectedId.toStdString())){
            cout << "connecting: " << s.toStdString() << " and " << toBeConnectedId.toStdString() << endl; cout.flush();
            add_edge_by_label(s.toStdString().c_str(),toBeConnectedId.toStdString().c_str(), _annotationGraph);
            statusBar()->showMessage(QString("Connected Vertex %1 and %2!").arg(s,toBeConnectedId));
            writeGraphToXML(true);
        }
        else{
            statusBar()->showMessage(QString("Vertex %1 and %2 are already connected!").arg(s,toBeConnectedId));
        }
    }
    refreshGraph(_annotationGraph, _annotationTabSVGView);
}

void MainWindow::drawSVGGraph(QByteArray dot, SvgView* view)
{
    killLayoutProcess();

    QStringList procargs;
    procargs.push_back( "-Tsvg" );
    procargs.push_back( "-Gcharset=latin1" );
    procargs.push_back( "graph.dot" );
    procargs.push_back( "> graph.svg" );

    QFile file("graph.dot");
    file.open(QIODevice::WriteOnly);
    file.write(dot);
    file.close();
    system( "/usr/local/bin/neato -Tsvg -Gcharset=latin1 graph.dot > graph.svg" );

    QFile graph("graph.svg");
    graph.open(QIODevice::ReadOnly);
    QByteArray result = graph.readAll();
    graph.close();
    // cout << QString(result).toStdString() << endl; cout.flush();
    view->load( result);
    _svgData =  result;
}


void MainWindow::killLayoutProcess()
{
    if( _layoutProcess->state() != QProcess::NotRunning )
    {
        _layoutProcess->kill();
        _layoutProcess->waitForFinished();
    }
}

void MainWindow::layoutProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{

    if( exitStatus == QProcess::NormalExit && exitCode == 0 )
    {
        //  cout<< "process finished!" << endl;
        //  cout.flush();
        //QByteArray result = _layoutProcess->readAll();
        //_svgView->load( result );
        //_svgData = result;
    }
}


void MainWindow::refreshGraph(FloorPlanGraph& g, SvgView* view){
    stringstream out;
    writeGraphvizHomeMade(g, out);
    //writeGraphvizHomeMade(g);
    drawSVGGraph(QByteArray(out.str().c_str(), out.str().size()),view);
}


void MainWindow::on_drawOutlineRadioButton_toggled(bool checked)
{
    if (checked){
        ui->PickScaleButton->setEnabled(true);
        ui->drawRoomOutlineButton->setEnabled(true);
        //mapview->isScaleLocked = true;
        currentMode = NOMODE;
        scaleStartPicked= false;
        lineSegmentStarted = false;
        ui->addRoomButton->setEnabled(false);
        ui->rooms1->setEnabled(false);
        ui->rooms2->setEnabled(false);
        ui->roomCatTextBox->setEnabled(false);
        ui->roomNumberTextBox->setEnabled(false);
    }
    else{

        ui->PickScaleButton->setChecked(false);
        ui->drawRoomOutlineButton->setChecked(false);
        ui->PickScaleButton->setEnabled(false);
        ui->drawRoomOutlineButton->setEnabled(false);
    }
}

void MainWindow::on_graphMode_toggled(bool checked)
{
    if(checked){
        currentMode = GRAPHMODE;
        ui->addRoomButton->setEnabled(true);
        ui->rooms1->setEnabled(true);
        ui->rooms2->setEnabled(true);
        ui->roomCatTextBox->setEnabled(true);
        ui->roomNumberTextBox->setEnabled(true);
        _annotationTabGraphicsView->isScaleLocked = false;


    }
    else{
        ui->addRoomButton->setEnabled(false);
        ui->rooms1->setEnabled(false);
        ui->rooms2->setEnabled(false);
        ui->roomCatTextBox->setEnabled(false);
        ui->roomNumberTextBox->setEnabled(false);
        ui->portalToRoomComboBox->setEnabled(false);
    }
}

void MainWindow::on_PickScaleButton_toggled(bool checked)
{
    if(checked){
        ui->mouseDistance->setEnabled(true);
        ui->scaleInterval2->setEnabled(true);
        ui->setScaleButton->setEnabled(true);
        currentMode = PICKSCALE;
        ui->setScaleButton->setEnabled(true);

        usedPen = redPen;
    }
    else{
        ui->mouseDistance->setEnabled(false);
        ui->scaleInterval2->setEnabled(false);
        ui->setScaleButton->setEnabled(false);
    }
}

void MainWindow::on_setScaleButton_clicked()
{
    if(scaleMouseDistance != -1 && !ui->scaleInterval2->text().isEmpty()){
        scaleRealDistance = ui->scaleInterval2->text().toDouble();
        scaleStartPicked=false;
        statusBar()->showMessage(QString("Scale set to %1").arg(scaleRealDistance));
        ui->setScaleButton->setEnabled(false);
        currentMode = NOMODE;
    }
    else{
        statusBar()->showMessage("You forgot to input distance in meters or didn't pick the scale.");
    }
}

void MainWindow::on_drawRoomOutlineButton_clicked()
{
    ui->roomsComboBox->clear();
    ui->portalToRoomComboBox->clear();

    ui->roomsComboBox->addItem(QString("Select Room"));
    ui->portalToRoomComboBox->addItem("Select portal destination");
    BGL_FORALL_VERTICES(v, _annotationGraph, FloorPlanGraph){
        ui->roomsComboBox->addItem(QString(_annotationGraph.graph()[v].vertex_id.c_str()));
        ui->portalToRoomComboBox->addItem(QString(_annotationGraph.graph()[v].vertex_id.c_str()));
    }
}

void MainWindow::on_drawRoomOutlineButton_toggled(bool checked)
{
    if(checked){
        ui->lineTypesComboBox->setEnabled(true);
        ui->roomsComboBox->setEnabled(true);

        currentMode = DRAWLAYOUT;
        ui->addRoomLayout->setEnabled(true);
        ui->removeRoomLayoutpushButton->setEnabled(true);
    }
    else{
        ui->lineTypesComboBox->setEnabled(false);
        ui->roomsComboBox->setEnabled(false);
        ui->addRoomLayout->setEnabled(false);
        ui->removeRoomLayoutpushButton->setEnabled(false);
        ui->portalToRoomComboBox->setEnabled(false);
    }
}

void MainWindow::on_addRoomLayout_clicked()
{
    if (ui->roomsComboBox->count() > 1 && numberofCurrentVertices() > 0){
        string roomid = ui->roomsComboBox->currentText().trimmed().toStdString();
        _annotationGraph[roomid].roomLayout = currentRoomLayout;
        QPointF centroid = calculateRoomCentroid(currentRoomLayout);
        _annotationGraph[roomid].centroid.setX(centroid.x());
        _annotationGraph[roomid].centroid.setY(centroid.y());
        QRectF tmprect(QPointF(_annotationGraph[roomid].centroid.x() -5,_annotationGraph[roomid].centroid.y() -5 ), QSize(10,10));
        _annotationGraph[roomid].centroidSceneItem =  _annotationTabGraphicsScene->addRect(tmprect, usedPen);
        writeGraphToXML(true);
        ui->roomsComboBox->setCurrentIndex(0);
        lineSegmentStarted=false;
        currentMode = NOMODE;
        ui->addRoomLayout->setEnabled(true);
        ui->removeRoomLayoutpushButton->setEnabled(true);
        currentRoomLayout.clear();
    }

}

void MainWindow::removeRoomLayout(std::string roomid){

    foreach(LineSegment l, _annotationGraph[roomid].roomLayout){
        _annotationTabGraphicsScene->removeItem(l.sceneItem);
    }
    if(_annotationGraph[roomid].centroidSceneItem != NULL){
        _annotationTabGraphicsScene->removeItem(_annotationGraph[roomid].centroidSceneItem);
    }
    _annotationGraph[roomid].roomLayout.clear();
    _annotationGraph[roomid].centroid.setX(-1);
    _annotationGraph[roomid].centroid.setY(-1);
}

void MainWindow::on_removeRoomLayoutpushButton_clicked()
{
    if (ui->roomsComboBox->count() > 1 && numberofCurrentVertices() > 0){
        string roomid = ui->roomsComboBox->currentText().trimmed().toStdString();
        removeRoomLayout(roomid);
    }

}

int MainWindow::numberofCurrentVertices(){
    int i=0;
    BGL_FORALL_VERTICES(v, _annotationGraph, FloorPlanGraph){
        i++;
    }
    return i;
}


void MainWindow::on_lineTypesComboBox_currentIndexChanged(const QString &arg1)
{
    if(arg1.compare("Wall") == 0){
        usedPen = redPen;
        ui->portalToRoomComboBox->setEnabled(false);
        ui->roomsComboBox->setEnabled(true);
    }
    else if (arg1.compare("Portal") == 0){
        usedPen = greenPen;
        ui->portalToRoomComboBox->setEnabled(true);
        ui->roomsComboBox->setEnabled(false);
    }
    else if (arg1.compare("Window") == 0){
        usedPen = bluePen;
        ui->portalToRoomComboBox->setEnabled(false);
        ui->roomsComboBox->setEnabled(true);
    }
}


void MainWindow::on_removeRoompushButton_clicked()
{
    std::vector<MyEdge> ev;
    MyVertex ve;

    if(ui->rooms2->selectedItems().size() == 0){
        return;
        statusBar()->showMessage("No room is selected!");
    }
    QString roomIdToBeRemoved = ui->rooms2->selectedItems().at(0)->data(Qt::DisplayRole).toString().split(" - ").at(0);
    removeRoomLayout(roomIdToBeRemoved.toStdString());
    cout << "room id to be removed is: " << roomIdToBeRemoved.toStdString() << endl; cout.flush();
    BGL_FORALL_VERTICES(v, _annotationGraph, FloorPlanGraph){
        if (QString(_annotationGraph.graph()[v].vertex_id.c_str()).compare(roomIdToBeRemoved) == 0){
            ve = v;
            BGL_FORALL_OUTEDGES(v, e, _annotationGraph, FloorPlanGraph) {
                ev.push_back(e);
            }
        }
    }

    foreach(MyEdge e, ev){
        remove_edge(e,_annotationGraph);
    }

    remove_vertex(roomIdToBeRemoved.toStdString().c_str(),_annotationGraph);
    on_drawRoomOutlineButton_clicked();

    /* First remove from the graph
    graph_traits<MyGraph>::vertex_iterator vi, vi_end,next;

    tie(vi, vi_end) = vertices(g);
    for (next=vi; vi != vi_end; ++vi) {

        ++next;

        if(roomIdToBeRemoved.compare(QString(g.graph()[*vi].vertex_id.c_str())) == 0){

            remove_vertex(g.graph()[*vi].vertex_id, g);

        }
    }*/
    updateRoomListView();

}

void MainWindow::on_addMultipleRoomspushButton_clicked()
{
    int start = ui->fromRoomIntervallineEdit_2->text().toInt();
    int end = ui->toRoomIntervallineEdit->text().toInt();
    for (int i = start; i < end; i++){
        if(addVertex(ui->roomCatTextBox->text().toStdString(), QString("%1").arg(i).toStdString(),_annotationGraph)){
            statusBar()->showMessage(QString("Added room with id %1 and category %2").arg(ui->roomNumberTextBox->text(), ui->roomCatTextBox->text()));
        }
        else {
            statusBar()->showMessage("Cannot add vertex, possibly another vertex with the same id exists already!");
        }
    }
    updateRoomListView();
    writeGraphToXML(true);

}


void MainWindow::on_tabWidget_currentChanged(int index)
{

    if(index == 1){
        writeGraphToXML(true);
        QFile file(lastSavedFileName);
        file.open(QIODevice::ReadOnly);
        QString str(file.readAll());
        ui->XMLFiletextEdit->setText(str);
        file.close();
    }
}

void MainWindow::on_buildingNamelineEdit_textChanged(const QString &arg1)
{
    buildingName = arg1;
}


void MainWindow::on_refreshGraphpushButton_clicked()
{
    refreshGraph(_annotationGraph, _annotationTabSVGView);
}



void MainWindow::on_displayXMLpushButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    env.value("HOME"),
                                                    tr("XML Files (*.xml)"));
    _viewerTabGraphicsScene->clear();
    _viewerTabSVGView->load(QByteArray());
    _viewerGraph = FloorPlanGraph();

    loadFromXML(filename, _viewerGraph);
    refreshGraph(_viewerGraph, _viewerTabSVGView);
    drawGraphLayout(_viewerGraph, _viewerTabGraphicsScene);

    QImage img(5000,5000,QImage::Format_ARGB32_Premultiplied);
    cout << "llala" << endl;
    QPainter p(&img);
    QFlag painterflags(QPainter::Antialiasing | QPainter::TextAntialiasing || QPainter::SmoothPixmapTransform);
    p.setRenderHints(painterflags);
    _viewerTabGraphicsScene->render(&p);
    p.end();
    cout << "bbb" << endl;
    _viewerTabGraphicsScene->clear();
    QPixmap qmap = QPixmap::fromImage(img);
    sceneItem = new MyQGraphicsPixmapItem(qmap);
    _viewerTabGraphicsScene->addItem(sceneItem);

    QRect r = qmap.rect();
    _viewerTabGraphicsView->setSceneRect(r);
    _viewerTabGraphicsView->SetCenter(QPoint(qmap.size().width()/2,qmap.size().height()/2));

}



void MainWindow::on_convertXMLtoImagepushButton_clicked()
{
    QStringList filenamelist = QFileDialog::getOpenFileNames(this, tr("Open File"),
                                                             env.value("HOME"),
                                                             tr("XML Files (*.xml)"));
    foreach(QString s, filenamelist){
        QGraphicsScene scene;
        FloorPlanGraph g;
        cout<< "Processing" << s.toStdString() << endl; cout.flush();
        loadFromXML(s, g);


        // save metric map to file
       drawGraphLayout(g, &scene);
        cout << scene.sceneRect().width() << "," << scene.sceneRect().height() << endl;
        QImage layoutmap(scene.sceneRect().width(), scene.sceneRect().height(),QImage::Format_ARGB32_Premultiplied);

        cout << "llala" << endl;
        cout << layoutmap.width() << "," << layoutmap.height() << endl;
        QPainter p(&layoutmap);
        QFlag painterflags(QPainter::Antialiasing);
        p.setRenderHints(painterflags);
        scene.render(&p);
        p.end();
        cout << "bbb" << endl;
        layoutmap.save(QString(s+"_Layout.png"));


        // save graph to file
        stringstream out;
        writeGraphvizHomeMade(g, out);

        QFile file("graph.dot");
        file.open(QIODevice::WriteOnly);
        file.write(QByteArray(out.str().c_str(), out.str().size()));
        file.close();
        system( ("/usr/local/bin/neato -Tpng -Gcharset=latin1 graph.dot >" +  s.toStdString() +  "_Graph.png").c_str() );

    }

    cout << "All done." << endl;
}
