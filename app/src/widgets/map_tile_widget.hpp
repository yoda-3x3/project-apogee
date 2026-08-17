#pragma once

#include <optional>

#include <QGraphicsView>
#include <QHash>
#include <QPoint>
#include <QPointF>
#include <QString>

class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsEllipseItem;
class QNetworkAccessManager;
class QTimer;

namespace apogee::app {

// A minimal slippy map: Esri World Imagery XYZ tiles drawn on a
// QGraphicsScene in Web Mercator pixel space, so panning is just ordinary
// QGraphicsView scrolling and no manual re-centering math is needed.
// Left-drag pans; a left-click that doesn't move (see dragMoved_) sets the
// launch-site marker and emits locationPicked(); the wheel zooms. Tiles are
// disk-cached via QNetworkDiskCache and kept in an in-memory tileItems_ map
// per zoom level so re-entering a previously-seen area doesn't even hit the
// disk cache again.
class MapTileWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapTileWidget(QWidget* parent = nullptr);

    void setMarker(double latitude, double longitude);
    void setLandingMarker(double latitude, double longitude);
    void clearLandingMarker();
    void centerOnLatLon(double latitude, double longitude);

signals:
    void locationPicked(double latitude, double longitude);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    void scheduleTileRefresh();
    void refreshTiles();
    void requestTile(int tileX, int tileY, int zoom);
    void rebuildSceneForZoom();
    void repositionMarkers();
    QPointF latLonToScenePixel(double latitude, double longitude, int zoom) const;
    QPointF scenePixelToLatLon(const QPointF& scenePixel, int zoom) const;

    QGraphicsScene* scene_ = nullptr;
    QNetworkAccessManager* network_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    int zoom_ = 4;
    // Bumped every time rebuildSceneForZoom() tears down tileItems_ -- an
    // in-flight tile request's finished lambda compares its captured
    // generation against this to know whether its QGraphicsPixmapItem* is
    // still alive, since QGraphicsItem isn't a QObject and so can't use
    // QPointer for that.
    int sceneGeneration_ = 0;
    QHash<QString, QGraphicsPixmapItem*> tileItems_;  // "x/y" at the current zoom_ only

    QGraphicsEllipseItem* marker_ = nullptr;
    QGraphicsEllipseItem* landingMarker_ = nullptr;
    std::optional<QPointF> markerLatLon_;
    std::optional<QPointF> landingLatLon_;

    QPoint dragStartPos_;
    bool dragging_ = false;
    bool dragMoved_ = false;
};

}  // namespace apogee::app
