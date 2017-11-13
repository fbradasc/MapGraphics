#include "OSMTileSource.h"

#include "guts/MapGraphicsNetwork.h"

#include <cmath>
#include <QPainter>
#include <QStringBuilder>
#include <QtDebug>
#include <QNetworkReply>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

OSMTileSource::OSMTileSource(OSMTileType tileType) :
    MapTileSource(), _tileType(tileType)
{
    this->setCacheMode(MapTileSource::DiskAndMemCaching);
}

OSMTileSource::~OSMTileSource()
{
    qDebug() << this << this->name() << "Destructing";
}

QPointF OSMTileSource::ll2qgs(const QPointF &ll, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal x = (ll.x()+180) * (tilesOnOneEdge*tileSize)/360; // coord to pixel!
    qreal y = (1-(log(tan(PI/4+(ll.y()*deg2rad)/2)) /PI)) /2  * (tilesOnOneEdge*tileSize);

    return QPoint(int(x), int(y));
}

QPointF OSMTileSource::qgs2ll(const QPointF &qgs, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal longitude = (qgs.x()*(360/(tilesOnOneEdge*tileSize)))-180;
    qreal latitude = rad2deg*(atan(sinh((1-qgs.y()*(2/(tilesOnOneEdge*tileSize)))*PI)));

    return QPointF(longitude, latitude);
}

quint64 OSMTileSource::tilesOnZoomLevel(quint8 zoomLevel) const
{
    return pow(4.0,zoomLevel);
}

quint16 OSMTileSource::tileSize() const
{
    return 256;
}

quint8 OSMTileSource::minZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 0;
}

quint8 OSMTileSource::maxZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 18;
}

QString OSMTileSource::name() const
{
    switch(_tileType)
    {
    case OSMTiles:
        return "OpenStreetMap Tiles";
        break;

    case MapQuestOSMTiles:
        return "MapQuestOSM Tiles";
        break;

    case MapQuestAerialTiles:
        return "MapQuest Aerial Tiles";
        break;

    case TopoMapOSMTiles:
        return "OpenTopoMap Tiles";
        break;

    default:
        return "Unknown Tiles";
        break;
    }
}

QString OSMTileSource::tileFileExtension() const
{
    if (_tileType == OSMTiles || _tileType == MapQuestOSMTiles || _tileType == TopoMapOSMTiles)
        return "png";
    else
        return "jpg";
}

#define FIT_URL(url, scheme, host, path, query) \
{ \
    url.setScheme(scheme); \
    url.setHost  ( host ); \
    url.setPath  ( path ); \
    \
    if (!query.empty()) \
    { \
        url.setQueryItems(query); \
    } \
}

//protected
void OSMTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    MapGraphicsNetwork * network = MapGraphicsNetwork::getInstance();

    QUrl url;

    //Figure out which server to request from based on our desired tile type
    if (_tileType == OSMTiles)
    {
        url.setScheme("http");
        url.setHost("tile.openstreetmap.org");
        url.setPath(QString("/%1/%2/%3.png").arg(QString::number(z),QString::number(x),QString::number(y)));
    }
    else if (_tileType == TopoMapOSMTiles)
    {
        QString host   = QString("a.tile.thunderforest.com");
        QString scheme = QString("https");
        QString file   = QString("%1/%2/%3.png").arg(QString::number(z),QString::number(x),QString::number(y));
        typedef QPair<QString, QString> QueryItem_t;
        typedef QList<QueryItem_t> Query_t;

        const QueryItem_t apikey("apikey", "def4ddae13e44bd79882ccc24cf486d9");

        Query_t query;

        query.append(apikey);

        // FIT_URL(url, scheme, host, "/cycle/"          + file, query); // OpenCycleMap
        // FIT_URL(url, scheme, host, "/transport/"      + file, query); // Transport
        // FIT_URL(url, scheme, host, "/landscape/"      + file, query); // Landscape
        // FIT_URL(url, scheme, host, "/outdoors/"       + file, query); // Outdoors
        // FIT_URL(url, scheme, host, "/transport-dark/" + file, query); // Transport Dark
        // FIT_URL(url, scheme, host, "/spinal-map/"     + file, query); // Spinal Map
        // FIT_URL(url, scheme, host, "/pioneer/"        + file, query); // Pioneer
        // FIT_URL(url, scheme, host, "/mobile-atlas/"   + file, query); // Mobile Atlas
        // FIT_URL(url, scheme, host, "/neighbourhood/"  + file, query); // Neighbourhood

        query.clear();
        FIT_URL(url, "https", "a.tile.opentopomap.org", "/" + file, query);        // OpenTopoMap

//         url.setScheme("http");
//         url.setHost("tile.opentopomap.org");
//         url.setPath(QString("/%1/%2/%3.png").arg(QString::number(z),QString::number(x),QString::number(y)));
    }
    else if (_tileType == MapQuestOSMTiles)
    {
return;
        url.setScheme("http");
        url.setHost("otile1.mqcdn.com");
        url.setPath(QString("/tiles/1.0.0/osm/%1/%2/%3.jpg").arg(QString::number(z),QString::number(x),QString::number(y)));
    }
    else
    {
return;
        url.setScheme("http");
        url.setHost("otile1.mqcdn.com");
        url.setPath(QString("/tiles/1.0.0/sat/%1/%2/%3.jpg").arg(QString::number(z),QString::number(x),QString::number(y)));
    }

    // url.setPort(80);

    //Use the unique cacheID to see if this tile has already been requested
    const QString cacheID = this->createCacheID(x,y,z);
    if (_pendingRequests.contains(cacheID))
        return;
    _pendingRequests.insert(cacheID);

    //Build the request
    QNetworkRequest request(url);
    qDebug() << request.url().toString();

    //Send the request and setupd a signal to ensure we're notified when it finishes
    QNetworkReply * reply = network->get(request);
    _pendingReplies.insert(reply,cacheID);

    connect(reply,
            SIGNAL(finished()),
            this,
            SLOT(handleNetworkRequestFinished()));
}

//private slot
void OSMTileSource::handleNetworkRequestFinished()
{
    QObject * sender = QObject::sender();
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender);

    if (reply == 0)
    {
        qWarning() << "QNetworkReply cast failure";
        return;
    }

    /*
      We can do this here and use reply later in the function because the reply
      won't be deleted until execution returns to the event loop.
    */
    reply->deleteLater();

    if (!_pendingReplies.contains(reply))
    {
        qWarning() << "Unknown QNetworkReply";
        return;
    }

    //get the cacheID
    const QString cacheID = _pendingReplies.take(reply);

    _pendingRequests.remove(cacheID);

    //If there was a network error, ignore the reply
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "ErrorNo: " << reply->error() << "for url: " << reply->url().toString();
        qDebug() << "Request failed, " << reply->errorString();
        qDebug() << "Headers:"<<  reply->rawHeaderList() << "content:" << reply->readAll();
        return;
    }

    //Convert the cacheID back into x,y,z tile coordinates
    quint32 x,y,z;
    if (!MapTileSource::cacheID2xyz(cacheID,&x,&y,&z))
    {
        qWarning() << "Failed to convert cacheID" << cacheID << "back to xyz";
        return;
    }

    QByteArray bytes = reply->readAll();
    QImage * image = new QImage();

    if (!image->loadFromData(bytes))
    {
        delete image;
        qWarning() << "Failed to make QImage from network bytes";
        return;
    }

    //Figure out how long the tile should be cached
    QDateTime expireTime;
    if (reply->hasRawHeader("Cache-Control"))
    {
        //We support the max-age directive only for now
        const QByteArray cacheControl = reply->rawHeader("Cache-Control");
        QRegExp maxAgeFinder("max-age=(\\d+)");
        if (maxAgeFinder.indexIn(cacheControl) != -1)
        {
            bool ok = false;
            const qint64 delta = maxAgeFinder.cap(1).toULongLong(&ok);

            if (ok)
                expireTime = QDateTime::currentDateTimeUtc().addSecs(delta);
        }
    }

    //Notify client of tile retrieval
    this->prepareNewlyReceivedTile(x,y,z, image, expireTime);
}
