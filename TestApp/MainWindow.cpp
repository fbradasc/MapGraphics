#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "MapGraphicsView.h"
#include "MapGraphicsScene.h"
#include "tileSources/GridTileSource.h"
#include "tileSources/OSMTileSource.h"
#include "tileSources/CompositeTileSource.h"
#include "guts/CompositeTileSourceConfigurationWidget.h"
#include "CircleObject.h"
#include "PolygonObject.h"
#include "WeatherManager.h"

#include <QSharedPointer>
#include <QtDebug>
#include <QThread>
#include <QImage>

const char *tiles_servers[][2] =
{
    { "OpenStreetMap"  , "http://tile.openstreetmap.org/{z}/{x}/{y}.png"                                                           },
    { "OpenCycleMap"   , "https://a.tile.thunderforest.com/cycle/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"          },
    { "Transport"      , "https://a.tile.thunderforest.com/transport/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"      },
    { "Landscape"      , "https://a.tile.thunderforest.com/landscape/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"      },
    { "Outdoors"       , "https://a.tile.thunderforest.com/outdoors/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"       },
    { "Transport Dark" , "https://a.tile.thunderforest.com/transport-dark/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9" },
    { "Spinal Map"     , "https://a.tile.thunderforest.com/spinal-map/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"     },
    { "Pioneer"        , "https://a.tile.thunderforest.com/pioneer/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"        },
    { "Mobile Atlas"   , "https://a.tile.thunderforest.com/mobile-atlas/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"   },
    { "Neighbourhood"  , "https://a.tile.thunderforest.com/neighbourhood/{z}/{x}/{y}.png?apikey=def4ddae13e44bd79882ccc24cf486d9"  },
    { "OpenTopoMap"    , "https://a.tile.opentopomap.org//{z}/{x}/{y}.png"                                                         },
    { "MapQuest OSM"   , "http://otile1.mqcdn.com/tiles/1.0.0/osm/{z}/{x}/{y}.jpg"                                                 },
    { "MapQuest Aerial", "http://otile1.mqcdn.com/tiles/1.0.0/sat/{z}/{x}/{y}.jpg"                                                 },
    { NULL, NULL }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Setup the MapGraphics scene and view
    MapGraphicsScene * scene = new MapGraphicsScene(this);
    MapGraphicsView * view = new MapGraphicsView(scene,this);

    //The view will be our central widget
    this->setCentralWidget(view);

    QSharedPointer<CompositeTileSource> composite(new CompositeTileSource(), &QObject::deleteLater);

    for (int i=0; tiles_servers[i][0] != NULL; i++)
    {
        QSharedPointer<OSMTileSource> osmTiles
        (
            new OSMTileSource
            (
                tiles_servers[i][0],
                tiles_servers[i][1]
            ),
            &QObject::deleteLater
        );
        composite->addSourceBottom(osmTiles);
    }

    QSharedPointer<GridTileSource> gridTiles(new GridTileSource(), &QObject::deleteLater);
    composite->addSourceBottom(gridTiles);

    view->setTileSource(composite);

    //Create a widget in the dock that lets us configure tile source layers
    CompositeTileSourceConfigurationWidget * tileConfigWidget = new CompositeTileSourceConfigurationWidget(composite.toWeakRef(),
                                                                                         this->ui->dockWidget);
    this->ui->dockWidget->setWidget(tileConfigWidget);
    delete this->ui->dockWidgetContents;

    this->ui->menuWindow->addAction(this->ui->dockWidget->toggleViewAction());
    this->ui->dockWidget->toggleViewAction()->setText("&Layers");

    view->setZoomLevel(4);
    view->centerOn(-111.658752, 40.255456);

    WeatherManager * weatherMan = new WeatherManager(scene, this);
    Q_UNUSED(weatherMan)
}

MainWindow::~MainWindow()
{
    delete ui;
}

//private slot
void MainWindow::on_actionExit_triggered()
{
    this->close();
}
