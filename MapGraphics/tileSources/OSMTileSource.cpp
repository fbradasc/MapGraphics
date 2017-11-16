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

OSMTileSource::OSMTileSource(QString name, QString queryUrl) :
    MapTileSource(), _name(name), _url(queryUrl)
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
    return _name.isEmpty() ? "Unknown Tiles" : _name;
}

QString OSMTileSource::tileFileExtension() const
{
    return _url.getExt();
}

//protected
void OSMTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    MapGraphicsNetwork * network = MapGraphicsNetwork::getInstance();

    QUrl url;

    url = _url.toQUrl(x,y,z);

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

OSMTileSource::OSMUrl::OSMUrl(QString url)
{
    _query.clear();

    url.replace(QString("{z}")       ,QString("%1"));
    url.replace(QString("{x}")       ,QString("%2"));
    url.replace(QString("{y}")       ,QString("%3"));
    url.replace(QRegExp("{[a-z-]+}."),QString( "" ));

    _scheme = url.section("://",0,0); url = url.section("://",1,-1);
    _host   = url.section("/"  ,0,0); url = url.section("/"  ,1,-1);
    _path   = url.section("?"  ,0,0); url = url.section("?"  ,1,-1);

    _ext    = _path.section(".",-1,-1);

    _port   = -1;

    QString port = _host.section(":",1,1);

    if (!port.isEmpty())
    {
        _host = _host.section(":",0,0);

        bool success = true;

        _port = port.toInt(&success);

        if (!success)
        {
            _port = -1;
        }
    }

    if (!url.isEmpty())
    {
        QStringList queries = url.split("&");

        for (int i=0; i<queries.size(); i++)
        {
            if (!queries[i].isEmpty())
            {
                const QueryItem_t item
                (
                    queries[i].section(QString("="),0,0),
                    queries[i].section(QString("="),1,1)
                );
                _query.append(item);
            }
        }
    }
}

const QUrl OSMTileSource::OSMUrl::toQUrl(quint32 x, quint32 y, quint8 z) const
{
    QUrl url;

    url.setScheme(_scheme);
    url.setHost  (_host);
    url.setPath  (QString(_path).arg(QString::number(z),QString::number(x),QString::number(y)));
    url.setPort  (_port);

    if (!_query.empty())
    {
        url.setQueryItems(_query);
    }

qDebug() << url;

    return url;
}
