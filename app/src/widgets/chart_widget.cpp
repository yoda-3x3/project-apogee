#include "widgets/chart_widget.hpp"

#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QToolTip>

namespace apogee::app {

namespace {
QString phaseName(core::FlightPhase phase) {
    switch (phase) {
        case core::FlightPhase::OnRail: return "Rail";
        case core::FlightPhase::Boost: return "Boost";
        case core::FlightPhase::Coast: return "Coast";
        case core::FlightPhase::Apogee: return "Apogee";
        case core::FlightPhase::Drogue: return "Drogue";
        case core::FlightPhase::Main: return "Main";
        case core::FlightPhase::Landed: return "Landed";
    }
    return QString();
}

constexpr int kMarginLeftPx = 56;
constexpr int kMarginRightPx = 16;
constexpr int kMarginTopPx = 28;
constexpr int kMarginBottomPx = 28;
}  // namespace

ChartWidget::ChartWidget(QString title, QString yAxisLabel, QWidget* parent)
    : QWidget(parent), title_(std::move(title)), yAxisLabel_(std::move(yAxisLabel)) {
    setMinimumHeight(140);
    setMouseTracking(true);
}

void ChartWidget::setData(QVector<QPointF> points, QVector<PhaseMarker> markers) {
    points_ = std::move(points);
    markers_ = std::move(markers);
    hoverIndex_ = -1;

    minY_ = 0;
    maxY_ = 1;
    maxX_ = 1;
    if (!points_.isEmpty()) {
        minY_ = maxY_ = points_.front().y();
        for (const QPointF& p : points_) {
            minY_ = std::min(minY_, p.y());
            maxY_ = std::max(maxY_, p.y());
        }
        maxX_ = std::max(1.0, points_.back().x());
        if (maxY_ - minY_ < 1e-9) maxY_ = minY_ + 1.0;
        // A little headroom so the series doesn't touch the top/bottom edge.
        const double pad = (maxY_ - minY_) * 0.08;
        maxY_ += pad;
        if (minY_ > 0.0) minY_ = std::max(0.0, minY_ - pad);
    }

    update();
}

QRectF ChartWidget::plotRect() const {
    return QRectF(kMarginLeftPx, kMarginTopPx, width() - kMarginLeftPx - kMarginRightPx,
                  height() - kMarginTopPx - kMarginBottomPx);
}

QPointF ChartWidget::dataToPixel(const QPointF& dataPoint, const QRectF& plot) const {
    const double xFrac = maxX_ > 0.0 ? dataPoint.x() / maxX_ : 0.0;
    const double yFrac = (maxY_ - minY_) > 0.0 ? (dataPoint.y() - minY_) / (maxY_ - minY_) : 0.0;
    return QPointF(plot.left() + xFrac * plot.width(), plot.bottom() - yFrac * plot.height());
}

int ChartWidget::nearestIndexForPixelX(double pixelX, const QRectF& plot) const {
    if (points_.isEmpty()) return -1;
    const double xFrac = plot.width() > 0.0 ? (pixelX - plot.left()) / plot.width() : 0.0;
    const double targetT = std::clamp(xFrac, 0.0, 1.0) * maxX_;

    int lo = 0, hi = static_cast<int>(points_.size()) - 1;
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (points_[mid].x() < targetT) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

void ChartWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().base());

    const QColor textColor = palette().color(QPalette::Text);
    const QColor gridColor = palette().color(QPalette::Mid);
    const QColor seriesColor(66, 133, 200);
    const QColor markerColor = palette().color(QPalette::PlaceholderText);

    painter.setPen(textColor);
    painter.drawText(QRectF(0, 2, width(), kMarginTopPx - 4), Qt::AlignHCenter, title_);

    if (points_.isEmpty()) {
        painter.setPen(markerColor);
        painter.drawText(rect(), Qt::AlignCenter, "No flight data yet -- click Fly");
        return;
    }

    const QRectF plot = plotRect();

    // Gridlines + axis labels (5 horizontal bands).
    painter.setPen(gridColor);
    constexpr int kBands = 4;
    for (int i = 0; i <= kBands; ++i) {
        const double frac = static_cast<double>(i) / kBands;
        const double y = plot.bottom() - frac * plot.height();
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));

        const double value = minY_ + frac * (maxY_ - minY_);
        painter.setPen(textColor);
        painter.drawText(QRectF(0, y - 8, kMarginLeftPx - 6, 16), Qt::AlignRight | Qt::AlignVCenter,
                          QString::number(value, 'f', value < 10 ? 2 : 0));
        painter.setPen(gridColor);
    }
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());

    painter.setPen(textColor);
    painter.drawText(QRectF(0, height() - kMarginBottomPx + 6, width(), kMarginBottomPx - 6),
                      Qt::AlignHCenter, QString("time (s)  |  %1").arg(yAxisLabel_));

    // Phase-transition markers.
    for (const PhaseMarker& marker : markers_) {
        const QPointF top = dataToPixel(QPointF(marker.timeS, maxY_), plot);
        const QPointF bottom = dataToPixel(QPointF(marker.timeS, minY_), plot);
        painter.setPen(QPen(markerColor, 1, Qt::DashLine));
        painter.drawLine(QPointF(top.x(), plot.top()), QPointF(bottom.x(), plot.bottom()));
        painter.setPen(markerColor);
        painter.save();
        painter.translate(top.x() + 3, plot.top() + 2);
        painter.rotate(90);
        painter.drawText(0, 0, phaseName(marker.phase));
        painter.restore();
    }

    // Series polyline.
    QPainterPath path;
    path.moveTo(dataToPixel(points_.front(), plot));
    for (int i = 1; i < points_.size(); ++i) {
        path.lineTo(dataToPixel(points_[i], plot));
    }
    painter.setPen(QPen(seriesColor, 2));
    painter.drawPath(path);

    // Hover crosshair + value readout.
    if (hoverIndex_ >= 0 && hoverIndex_ < points_.size()) {
        const QPointF hoverPx = dataToPixel(points_[hoverIndex_], plot);
        painter.setPen(QPen(textColor, 1, Qt::DotLine));
        painter.drawLine(QPointF(hoverPx.x(), plot.top()), QPointF(hoverPx.x(), plot.bottom()));
        painter.setBrush(seriesColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(hoverPx, 3, 3);
    }
}

void ChartWidget::mouseMoveEvent(QMouseEvent* event) {
    if (points_.isEmpty()) return;

    const QRectF plot = plotRect();
    const QPointF pos = event->position();
    if (!plot.adjusted(-4, -4, 4, 4).contains(pos)) {
        if (hoverIndex_ != -1) {
            hoverIndex_ = -1;
            update();
        }
        QToolTip::hideText();
        return;
    }

    const int index = nearestIndexForPixelX(pos.x(), plot);
    if (index == hoverIndex_) return;
    hoverIndex_ = index;
    update();

    if (index >= 0 && index < points_.size()) {
        const QPointF& p = points_[index];
        QToolTip::showText(event->globalPosition().toPoint(),
                            QString("t = %1 s\n%2 = %3")
                                .arg(p.x(), 0, 'f', 2)
                                .arg(yAxisLabel_)
                                .arg(p.y(), 0, 'f', 2),
                            this);
    }
}

void ChartWidget::leaveEvent(QEvent*) {
    if (hoverIndex_ != -1) {
        hoverIndex_ = -1;
        update();
    }
    QToolTip::hideText();
}

}  // namespace apogee::app
