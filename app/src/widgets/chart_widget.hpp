#pragma once

#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>

#include "core/flight_phase.hpp"

class QMouseEvent;
class QPaintEvent;

namespace apogee::app {

struct PhaseMarker {
    double timeS = 0;
    core::FlightPhase phase = core::FlightPhase::OnRail;
};

// A hand-rolled line chart (plain QWidget + QPainter, no QtCharts
// dependency -- see the plan's rationale) plotting one value series against
// time, with dashed vertical flight-phase-transition markers and a
// hover tooltip showing the nearest sample's exact time/value. Colors are
// drawn from the widget palette so it stays correct across the app's
// Classic/Dark/Light/High-Contrast themes.
class ChartWidget : public QWidget {
    Q_OBJECT
public:
    ChartWidget(QString title, QString yAxisLabel, QWidget* parent = nullptr,
                QString emptyStateMessage = "No data yet");

    // Replaces the plotted series and phase markers; pass empty vectors to
    // clear back to the "no data yet" placeholder state.
    void setData(QVector<QPointF> points, QVector<PhaseMarker> markers);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QRectF plotRect() const;
    QPointF dataToPixel(const QPointF& dataPoint, const QRectF& plot) const;
    int nearestIndexForPixelX(double pixelX, const QRectF& plot) const;

    QString title_;
    QString yAxisLabel_;
    QString emptyStateMessage_;
    QVector<QPointF> points_;
    QVector<PhaseMarker> markers_;
    double minY_ = 0;
    double maxY_ = 1;
    double maxX_ = 1;
    int hoverIndex_ = -1;
};

}  // namespace apogee::app
