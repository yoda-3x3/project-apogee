#include "widgets/rocket_diagram_widget.hpp"

#include <algorithm>

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

namespace apogee::app {

RocketDiagramWidget::RocketDiagramWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(180);
}

void RocketDiagramWidget::setStabilityInfo(const StabilityInfo& info) {
    info_ = info;
    update();
}

void RocketDiagramWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().base());

    if (!info_.hasMinimumParts || info_.totalLengthM <= 0.0) {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(rect(), Qt::AlignCenter,
                          "Select a nose cone, body tube, and fin set to see the diagram");
        return;
    }

    const core::RocketDefinition& def = info_.definition;
    const double totalLengthM = info_.totalLengthM;
    const double halfBodyM = def.referenceDiameterM / 2.0;
    const double finTipHalfM = halfBodyM + def.finSemiSpanM;
    const double maxHalfExtentM = std::max(halfBodyM, finTipHalfM);

    constexpr int kMarginPx = 24;
    const double availableW = std::max(1.0, static_cast<double>(width()) - 2 * kMarginPx);
    const double availableH = std::max(1.0, static_cast<double>(height()) - 2 * kMarginPx);
    const double scale = std::min(availableW / totalLengthM, availableH / (2.2 * maxHalfExtentM));

    const double centerY = height() / 2.0;
    auto toPx = [&](double xM, double yM) {
        return QPointF(kMarginPx + xM * scale, centerY - yM * scale);
    };

    // Body tube
    QRectF bodyRect(toPx(def.noseLengthM, halfBodyM), toPx(totalLengthM, -halfBodyM));
    painter.setBrush(QColor(190, 190, 200));
    painter.setPen(QPen(QColor(90, 90, 100), 1.5));
    painter.drawRect(bodyRect.normalized());

    // Nose: an ogive curves outward toward the base, a cone is straight lines.
    const QPointF tip = toPx(0, 0);
    const QPointF baseTop = toPx(def.noseLengthM, halfBodyM);
    const QPointF baseBottom = toPx(def.noseLengthM, -halfBodyM);
    QPainterPath nosePath;
    nosePath.moveTo(tip);
    if (def.noseShape == core::NoseShape::Conical) {
        nosePath.lineTo(baseTop);
        nosePath.lineTo(baseBottom);
    } else {
        const QPointF ctrlTop = toPx(def.noseLengthM * 0.4, halfBodyM * 0.95);
        const QPointF ctrlBottom = toPx(def.noseLengthM * 0.4, -halfBodyM * 0.95);
        nosePath.quadTo(ctrlTop, baseTop);
        nosePath.lineTo(baseBottom);
        nosePath.quadTo(ctrlBottom, tip);
    }
    nosePath.closeSubpath();
    painter.drawPath(nosePath);

    // Fins: trapezoids projected above and below the body tube.
    if (def.finCount > 0) {
        const double rootLeM = def.finRootLeadingEdgeFromNoseM;
        const double rootTeM = rootLeM + def.finRootChordM;
        const double tipLeM = rootLeM + def.finSweepLengthM;
        const double tipTeM = tipLeM + def.finTipChordM;

        QPainterPath finTop;
        finTop.moveTo(toPx(rootLeM, halfBodyM));
        finTop.lineTo(toPx(tipLeM, finTipHalfM));
        finTop.lineTo(toPx(tipTeM, finTipHalfM));
        finTop.lineTo(toPx(rootTeM, halfBodyM));
        finTop.closeSubpath();
        painter.drawPath(finTop);

        QPainterPath finBottom;
        finBottom.moveTo(toPx(rootLeM, -halfBodyM));
        finBottom.lineTo(toPx(tipLeM, -finTipHalfM));
        finBottom.lineTo(toPx(tipTeM, -finTipHalfM));
        finBottom.lineTo(toPx(rootTeM, -halfBodyM));
        finBottom.closeSubpath();
        painter.drawPath(finBottom);
    }

    // CG/CP markers.
    auto drawMarker = [&](double xM, const QColor& color, const QString& label) {
        const QPointF top = toPx(xM, maxHalfExtentM * 1.1);
        const QPointF bottom = toPx(xM, -maxHalfExtentM * 1.1);
        painter.setPen(QPen(color, 2, Qt::DashLine));
        painter.drawLine(top, bottom);
        painter.setPen(color);
        painter.drawText(QPointF(top.x() - 8, top.y() - 6), label);
    };
    drawMarker(info_.cgFromNoseM, QColor(40, 140, 40), "CG");
    drawMarker(info_.cpFromNoseM, QColor(200, 50, 50), "CP");
}

}  // namespace apogee::app
