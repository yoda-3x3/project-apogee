#include "widgets/map_tile_widget.hpp"

#include <algorithm>
#include <cmath>

#include <QBrush>
#include <QDir>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTimer>
#include <QWheelEvent>
#include <QtMath>

namespace apogee::app {

namespace {
constexpr int kTileSizePx = 256;
constexpr int kMinZoom = 2;
constexpr int kMaxZoom = 18;
constexpr const char* kUserAgent =
    "ProjectApogee/0.1 (model rocket flight simulator; "
    "https://github.com/yoda-3x3/project-apogee)";

QString tileKey(int x, int y) { return QString("%1/%2").arg(x).arg(y); }
}  // namespace

MapTileWidget::MapTileWidget(QWidget* parent) : QGraphicsView(parent) {
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setBackgroundBrush(QColor(30, 32, 38));
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setMinimumHeight(220);

    network_ = new QNetworkAccessManager(this);
    auto* diskCache = new QNetworkDiskCache(network_);
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                                  "/map_tiles");
    network_->setCache(diskCache);

    refreshTimer_ = new QTimer(this);
    refreshTimer_->setSingleShot(true);
    refreshTimer_->setInterval(50);
    connect(refreshTimer_, &QTimer::timeout, this, &MapTileWidget::refreshTiles);

    marker_ = new QGraphicsEllipseItem();
    marker_->setBrush(QColor(220, 40, 40));
    marker_->setPen(QPen(Qt::white, 1.5));
    marker_->setZValue(10);
    marker_->setVisible(false);
    scene_->addItem(marker_);

    landingMarker_ = new QGraphicsEllipseItem();
    landingMarker_->setBrush(QColor(240, 200, 40));
    landingMarker_->setPen(QPen(Qt::black, 1.5));
    landingMarker_->setZValue(10);
    landingMarker_->setVisible(false);
    scene_->addItem(landingMarker_);

    rebuildSceneForZoom();
    // Roughly the center of the continental US -- a reasonable default
    // before the user picks a real site.
    centerOnLatLon(39.8, -98.5);
}

QPointF MapTileWidget::latLonToScenePixel(double latitude, double longitude, int zoom) const {
    const double latRad = qDegreesToRadians(latitude);
    const double n = std::pow(2.0, zoom) * kTileSizePx;
    const double x = (longitude + 180.0) / 360.0 * n;
    const double y = (1.0 - std::asinh(std::tan(latRad)) / M_PI) / 2.0 * n;
    return QPointF(x, y);
}

QPointF MapTileWidget::scenePixelToLatLon(const QPointF& scenePixel, int zoom) const {
    const double n = std::pow(2.0, zoom) * kTileSizePx;
    const double longitude = scenePixel.x() / n * 360.0 - 180.0;
    const double latRad = std::atan(std::sinh(M_PI * (1.0 - 2.0 * scenePixel.y() / n)));
    return QPointF(qRadiansToDegrees(latRad), longitude);  // (latitude, longitude)
}

void MapTileWidget::rebuildSceneForZoom() {
    ++sceneGeneration_;
    for (QGraphicsPixmapItem* item : std::as_const(tileItems_)) {
        scene_->removeItem(item);
        delete item;
    }
    tileItems_.clear();

    const double n = std::pow(2.0, zoom_) * kTileSizePx;
    scene_->setSceneRect(0, 0, n, n);
    repositionMarkers();
    scheduleTileRefresh();
}

void MapTileWidget::repositionMarkers() {
    constexpr double kMarkerRadiusPx = 6.0;
    if (markerLatLon_) {
        const QPointF p = latLonToScenePixel(markerLatLon_->x(), markerLatLon_->y(), zoom_);
        marker_->setRect(p.x() - kMarkerRadiusPx, p.y() - kMarkerRadiusPx, 2 * kMarkerRadiusPx,
                          2 * kMarkerRadiusPx);
        marker_->setVisible(true);
    }
    if (landingLatLon_) {
        const QPointF p = latLonToScenePixel(landingLatLon_->x(), landingLatLon_->y(), zoom_);
        landingMarker_->setRect(p.x() - kMarkerRadiusPx, p.y() - kMarkerRadiusPx, 2 * kMarkerRadiusPx,
                                 2 * kMarkerRadiusPx);
        landingMarker_->setVisible(true);
    }
}

void MapTileWidget::setMarker(double latitude, double longitude) {
    markerLatLon_ = QPointF(latitude, longitude);
    repositionMarkers();
}

void MapTileWidget::setLandingMarker(double latitude, double longitude) {
    landingLatLon_ = QPointF(latitude, longitude);
    repositionMarkers();
}

void MapTileWidget::clearLandingMarker() {
    landingLatLon_.reset();
    landingMarker_->setVisible(false);
}

void MapTileWidget::centerOnLatLon(double latitude, double longitude) {
    QGraphicsView::centerOn(latLonToScenePixel(latitude, longitude, zoom_));
    scheduleTileRefresh();
}

void MapTileWidget::scheduleTileRefresh() { refreshTimer_->start(); }

void MapTileWidget::refreshTiles() {
    const QRectF visible = mapToScene(viewport()->rect()).boundingRect();
    const qint64 tilesPerSide = 1LL << zoom_;

    const int xMin = static_cast<int>(
        std::max<qint64>(0, static_cast<qint64>(std::floor(visible.left() / kTileSizePx))));
    const int xMax = static_cast<int>(
        std::min<qint64>(tilesPerSide - 1, static_cast<qint64>(std::floor(visible.right() / kTileSizePx))));
    const int yMin = static_cast<int>(
        std::max<qint64>(0, static_cast<qint64>(std::floor(visible.top() / kTileSizePx))));
    const int yMax = static_cast<int>(std::min<qint64>(
        tilesPerSide - 1, static_cast<qint64>(std::floor(visible.bottom() / kTileSizePx))));

    for (int x = xMin; x <= xMax; ++x) {
        for (int y = yMin; y <= yMax; ++y) {
            requestTile(x, y, zoom_);
        }
    }
}

void MapTileWidget::requestTile(int tileX, int tileY, int zoom) {
    const QString key = tileKey(tileX, tileY);
    if (tileItems_.contains(key)) return;

    auto* item = new QGraphicsPixmapItem();
    item->setPos(tileX * kTileSizePx, tileY * kTileSizePx);
    item->setZValue(0);
    scene_->addItem(item);
    tileItems_.insert(key, item);

    // Esri's REST tile scheme is {z}/{y}/{x}, not the more common {z}/{x}/{y}.
    const QUrl url(QString("https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/"
                            "MapServer/tile/%1/%2/%3")
                       .arg(zoom)
                       .arg(tileY)
                       .arg(tileX));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArray(kUserAgent));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QNetworkReply* reply = network_->get(request);
    const int requestGeneration = sceneGeneration_;
    connect(reply, &QNetworkReply::finished, this, [this, reply, item, requestGeneration]() {
        reply->deleteLater();
        // item is a dangling pointer if the scene was rebuilt (zoom change)
        // while this request was in flight -- the generation check must
        // happen before item is touched in any way.
        if (requestGeneration != sceneGeneration_) return;
        if (reply->error() != QNetworkReply::NoError) return;

        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll())) {
            item->setPixmap(pixmap);
        }
    });
}

void MapTileWidget::wheelEvent(QWheelEvent* event) {
    const int newZoom = std::clamp(zoom_ + (event->angleDelta().y() > 0 ? 1 : -1), kMinZoom, kMaxZoom);
    if (newZoom != zoom_) {
        const QPointF centerLatLon = scenePixelToLatLon(mapToScene(viewport()->rect().center()), zoom_);
        zoom_ = newZoom;
        rebuildSceneForZoom();
        centerOnLatLon(centerLatLon.x(), centerLatLon.y());
    }
    event->accept();
}

void MapTileWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    dragging_ = true;
    dragMoved_ = false;
    dragStartPos_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
}

void MapTileWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!dragging_) return;
    const QPoint delta = event->pos() - dragStartPos_;
    if (delta.manhattanLength() > 3) dragMoved_ = true;
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    dragStartPos_ = event->pos();
}

void MapTileWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !dragging_) return;
    dragging_ = false;
    setCursor(Qt::ArrowCursor);

    if (!dragMoved_) {
        const QPointF latLon = scenePixelToLatLon(mapToScene(event->pos()), zoom_);
        setMarker(latLon.x(), latLon.y());
        emit locationPicked(latLon.x(), latLon.y());
    }
}

void MapTileWidget::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    scheduleTileRefresh();
}

void MapTileWidget::scrollContentsBy(int dx, int dy) {
    QGraphicsView::scrollContentsBy(dx, dy);
    scheduleTileRefresh();
}

}  // namespace apogee::app
