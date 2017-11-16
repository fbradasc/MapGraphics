#ifndef OSMTILESOURCE_H
#define OSMTILESOURCE_H

#include "MapTileSource.h"
#include "MapGraphics_global.h"
#include <QSet>
#include <QHash>
#include <QUrl>

//Forward declaration so that projects that import us as a library don't necessarily have to use QT += network
class QNetworkReply;

class MAPGRAPHICSSHARED_EXPORT OSMTileSource : public MapTileSource
{
    Q_OBJECT
public:
    enum OSMTileType
    {
        OSMTiles,
        MapQuestOSMTiles,
        MapQuestAerialTiles,
        TopoMapOSMTiles,
    };

public:
    explicit OSMTileSource(QString name, QString queryUrl);

    virtual ~OSMTileSource();

    virtual QPointF ll2qgs(const QPointF& ll, quint8 zoomLevel) const;

    virtual QPointF qgs2ll(const QPointF& qgs, quint8 zoomLevel) const;

    virtual quint64 tilesOnZoomLevel(quint8 zoomLevel) const;

    virtual quint16 tileSize() const;

    virtual quint8 minZoomLevel(QPointF ll);

    virtual quint8 maxZoomLevel(QPointF ll);

    virtual QString name() const;

    virtual QString tileFileExtension() const;

protected:
    virtual void fetchTile(quint32 x,
                           quint32 y,
                           quint8 z);

private:
    typedef QPair<QString, QString> QueryItem_t;
    typedef QList<QueryItem_t> Query_t;

    class OSMUrl
    {
    public:
        OSMUrl(QString url);

        const QUrl toQUrl(quint32 x, quint32 y, quint8 z) const;

        inline const QString &getScheme() const { return _scheme; }
        inline const QString &getHost  () const { return _host  ; }
        inline const QString &getPath  () const { return _path  ; }
        inline const QString &getExt   () const { return _ext   ; }
        inline const Query_t &getQuery () const { return _query ; }

    private:
        QString _scheme;
        QString _host;
        QString _path;
        QString _ext;
        Query_t _query;
        int     _port;
    };

    QString _name;
    OSMUrl  _url;

    //Set used to ensure a tile with a certain cacheID isn't requested twice
    QSet<QString> _pendingRequests;

    //Hash used to keep track of what cacheID goes with what reply
    QHash<QNetworkReply *, QString> _pendingReplies;

signals:

public slots:

private slots:
    void handleNetworkRequestFinished();
    
};

#endif // OSMTILESOURCE_H
