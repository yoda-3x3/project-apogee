#pragma once

#include <QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace apogee::app {

class LaunchSite;
class MapTileWidget;

// The Launch tab: a satellite map (click to set the launch site), rail
// geometry inputs, and a "Fetch Weather" action pulling live NWS + Open-
// Meteo data via WeatherService -- surface conditions for the readout,
// wind-aloft profile for the physics engine's WindField when "Use live
// wind" is checked (manual ground wind is used otherwise). Owns the shared
// LaunchSite model that FlightPanel reads from when flying, mirroring how
// RocketBuilderPanel owns RocketDesign.
class LaunchSitePanel : public QWidget {
    Q_OBJECT
public:
    explicit LaunchSitePanel(QWidget* parent = nullptr);

    LaunchSite& launchSite() { return *launchSite_; }

public slots:
    // Drops a marker at the flight's landing point, computed from the
    // launch site plus the summary's ENU landing offset (flat-earth
    // approximation -- plenty accurate at typical model-rocket drift
    // distances).
    void onFlightCompleted(double landingOffsetEastM, double landingOffsetNorthM);

private slots:
    void onMapLocationPicked(double latitude, double longitude);
    void onCoordinateSpinChanged();
    void onFetchWeatherClicked();
    void onUseLiveWindToggled(bool checked);
    void onRailOrWindInputsChanged();

private:
    void buildUi();
    void refreshWeatherReadout();

    LaunchSite* launchSite_ = nullptr;

    MapTileWidget* map_ = nullptr;
    QDoubleSpinBox* latitudeSpin_ = nullptr;
    QDoubleSpinBox* longitudeSpin_ = nullptr;
    QDoubleSpinBox* elevationSpin_ = nullptr;
    QDoubleSpinBox* railLengthSpin_ = nullptr;
    QDoubleSpinBox* railAngleSpin_ = nullptr;

    QPushButton* fetchWeatherButton_ = nullptr;
    QLabel* weatherStatusLabel_ = nullptr;
    QLabel* surfaceConditionsLabel_ = nullptr;
    QLabel* windAloftLabel_ = nullptr;

    QCheckBox* useLiveWindCheck_ = nullptr;
    QDoubleSpinBox* manualWindSpeedSpin_ = nullptr;
    QDoubleSpinBox* manualWindDirectionSpin_ = nullptr;
};

}  // namespace apogee::app
