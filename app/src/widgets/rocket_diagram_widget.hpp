#pragma once

#include <QWidget>

#include "models/rocket_design.hpp"

namespace apogee::app {

// A simple 2D side-profile of the current design: body tube, nose (ogive
// curve or conical straight lines), fins, and dashed CG/CP markers.
// Deliberately not geometrically precise (e.g. it doesn't render the nose
// shoulder overlapping into the body tube) -- it's a live sanity-check
// picture, not a technical drawing.
class RocketDiagramWidget : public QWidget {
    Q_OBJECT
public:
    explicit RocketDiagramWidget(QWidget* parent = nullptr);

    void setStabilityInfo(const StabilityInfo& info);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    StabilityInfo info_;
};

}  // namespace apogee::app
