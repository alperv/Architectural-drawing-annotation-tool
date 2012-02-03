#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QStringListModel>
#include <QListView>
#include <QAbstractItemView>
#include <QPainter>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <QLabel>
#include <QFileDialog>
#include "mygraphicsview.h"
#include <QRubberBand>
#include <QProcess>
#include <boost/graph/labeled_graph.hpp>
#include "SvgView.h"
#include <myqgraphicspixmapitem.h>
#include <QXmlStreamWriter>
#include <QFile>
#include <QMessageBox>
#include <QPoint>
#include <QtXml/QDomDocument>
#include <QTreeView>
#include <QFrame>
#include <QtXml/QDomNode>
#include <QStandardItemModel>
#include <QDateTime>
#include <QPointer>
namespace Ui {
class MainWindow;
}


struct LineSegment{
    LineSegment(){portalToRoom="";}
    LineSegment(QPointF startPos_, QPointF endPos_): startPos(startPos_), endPos(endPos_) { portalToRoom="";}
    QPointF startPos;
    QPointF endPos;
    QGraphicsLineItem* sceneItem;
    std::string type;
    std::string portalToRoom;
};



struct Room{
    Room(){centroid.setX(-1); centroid.setY(-1);}

    std::string category;
    std::string vertex_id;
    std::vector<LineSegment> roomLayout;
    QPointF centroid;
    QGraphicsRectItem* centroidSceneItem;
};

struct RoomEdge{
    std::string edge_id;
};

struct GraphProperties{
    GraphProperties(){ maxx = -1; maxy=-1; minx=-1; miny=-1;}
    double maxx,maxy,minx,miny;
    double floorCentroidX;
    double floorCentroidY;
};

typedef boost::labeled_graph<boost::adjacency_list<boost::listS, boost::listS, boost::undirectedS, Room, RoomEdge>,std::string> FloorPlanGraph;
typedef boost::graph_traits<FloorPlanGraph>::vertex_descriptor MyVertex;
typedef boost::graph_traits<FloorPlanGraph>::edge_descriptor MyEdge;

struct Room_Writer{
    Room_Writer(FloorPlanGraph& g_): g(g_) {}
    template <class Vertex>
    void operator()(std::ostream& out, Vertex v) {
        out << " [label=\"" << g.graph()[v].vertex_id << " - " << g.graph()[v].category << "\"]" << std::endl;

    }


    FloorPlanGraph g;
};

struct RoomEdge_Writer{
    RoomEdge_Writer(FloorPlanGraph& g_): g(g_) {}
    template <class Edge>
    void operator()(std::ostream& out, Edge e) {
        out << " [label=\"" << g.graph()[e].edge_id  << "\"]" << std::endl;

    }


    FloorPlanGraph g;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    void killLayoutProcess();

    int numberofCurrentVertices();
    bool addVertex(std::string category, std::string id, FloorPlanGraph& g);
    void addEdge(MyVertex u, MyVertex v, FloorPlanGraph& g);
    MyVertex findVertex(std::string s);
    bool isAlreadyConnected(std::string v1, std::string v2);
    bool doesVertexExists(std::string id, FloorPlanGraph& g);
    QPointF calculateRoomCentroid(std::vector<LineSegment> roomLayout);
    void removeRoomLayout(std::string roomid);

    void loadFromXML(QString filename, FloorPlanGraph& g);
    void loadXMLtoDOM(QFile& file);
    void writeGraph(FloorPlanGraph& g, std::ostream &out  = std::cout );
    void writeGraphvizHomeMade(FloorPlanGraph& g, std::ostream &out = std::cout);
    void writeGraphToXML(bool isBackup = false);
    void drawSVGGraph(QByteArray dot, SvgView* view);
    void refreshGraph(FloorPlanGraph& g, SvgView* view);
    void updateRoomListView();
    void drawGraphLayout(FloorPlanGraph& g, QGraphicsScene* scene);
    void showPortalConnections(FloorPlanGraph g, QGraphicsScene* scene);

    void drawLabels(FloorPlanGraph g, QGraphicsScene* scene);

private slots:

    void on_openFolderButton_clicked();

    void on_addRoomButton_clicked();

    void on_connectVertices_clicked();

    void layoutProcessFinished( int exitCode, QProcess::ExitStatus exitStatus );

    // void on_scaleButton_clicked();

    void on_drawOutlineRadioButton_toggled(bool checked);

    void on_graphMode_toggled(bool checked);

    void on_PickScaleButton_toggled(bool checked);

    void on_setScaleButton_clicked();

    void on_drawRoomOutlineButton_clicked();

    void on_drawRoomOutlineButton_toggled(bool checked);

    void on_addRoomLayout_clicked();

    void on_lineTypesComboBox_currentIndexChanged(const QString &arg1);

    void on_removeRoompushButton_clicked();

    void on_addMultipleRoomspushButton_clicked();

    void on_loadFromXMLpushButton_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_buildingNamelineEdit_textChanged(const QString &arg1);


    void on_refreshGraphpushButton_clicked();

    void on_removeRoomLayoutpushButton_clicked();

    void on_displayXMLpushButton_clicked();

    void on_showPortalConnectionscheckBox_clicked(bool checked);

    void on_convertXMLtoImagepushButton_clicked();

private:
    Ui::MainWindow *ui;
    void keyPressEvent(QKeyEvent* k);
    bool eventFilter(QObject *obj, QEvent *event);
    void setImage(std::string filename);

    int mousePosX,mousePosY;

    QStringListModel* m;
    QStringListModel* fileListModel;
    QListView* v;
    bool _roomCatWidgetAdded;
    bool _rightMousePressed;


    MyGraphicsView* _annotationTabGraphicsView;
    QGraphicsScene* _annotationTabGraphicsScene;

    MyGraphicsView* _viewerTabGraphicsView;
    QGraphicsScene* _viewerTabGraphicsScene;


    MyQGraphicsPixmapItem* item;

    FloorPlanGraph _annotationGraph;
    FloorPlanGraph _viewerGraph;
    GraphProperties _currentGraphProperties;
    QGraphicsPixmapItem* sceneItem;
    QRubberBand* rubberBand;
    QPoint origin;


    QString buildingName;
    QString filename;

    std::vector<QGraphicsRectItem*> sceneRects;
    std::map<std::string, MyVertex> idToVertex;

    SvgView* _annotationTabSVGView;
    SvgView* _viewerTabSVGView;

    QProcess* _layoutProcess;
    QByteArray _svgData;
    QGraphicsRectItem* nearestBlackPixel;
    QGraphicsLineItem* scaleLine;

    int scaleInterval;
    int scaleValue;
    int detectionWindowSize;
    double scaleMouseDistance;
    double scaleRealDistance;
    bool scaleStartPicked;
    QPointF scaleFirst, scaleLast;

    LineSegment currentLineSegment;
    bool lineSegmentStarted;
    bool lineSegmentFinished;
    QGraphicsLineItem* drawLayoutLine;
    std::vector<QGraphicsLineItem*> _portalConnectionLines;
    std::vector<QGraphicsTextItem*> _spaceTexts;

    QProcessEnvironment env;
    enum Mode{
        GRAPHMODE,
        PICKSCALE,
        DRAWLAYOUT,
        NOMODE
    };

    Mode currentMode;
    std::vector<LineSegment> currentRoomLayout;
    std::pair<QPoint,QPoint> scaleMouseInterval;

    QPen redPen;
    QPen bluePen;
    QPen greenPen;
    QPen usedPen;


    bool moveInVertical;
    bool moveInHorizontal;
    QMouseEvent* lastMouseEvent;

    QString lastSavedFileName;

};

#endif // MAINWINDOW_H
