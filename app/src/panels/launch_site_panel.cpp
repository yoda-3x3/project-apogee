#include "panels/launch_site_panel.hpp"

#include <cmath>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtMath>

#include "data/network_http_transport.hpp"
#include "data/nws_client.hpp"
#include "data/open_meteo_client.hpp"
#include "data/weather_service.hpp"
#include "models/launch_site.hpp"
#include "widgets/map_tile_widget.hpp"

namespace apogee::app {

namespace {
constexpr double kMetersPerDegreeLat = 111320.0;

QString formatWindAloft(const data::WindProfile& profile) {
    if (profile.levels.isEmpty()) return "No wind-aloft data available.";
    QStringList lines;
    for (const data::WindLevel& level : profile.levels) {
        lines << QString("%1 m: %2 m/s from %3 deg")
                     .arg(level.altitudeM, 0, 'f', 0)
                     .arg(level.speedMs, 0, 'f', 1)
                     .arg(level.directionDeg, 0, 'f', 0);
    }
    return lines.join("\n");
}
}  // namespace

LaunchSitePanel::LaunchSitePanel(QWidget* parent) : QWidget(parent) {
    launchSite_ = new LaunchSite(this);
    buildUi();
}

void LaunchSitePanel::buildUi() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* leftColumn = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftColumn);

    auto* coordGroup = new QGroupBox("Launch Site", leftColumn);
    auto* coordForm = new QFormLayout(coordGroup);

    latitudeSpin_ = new QDoubleSpinBox(coordGroup);
    latitudeSpin_->setRange(-90.0, 90.0);
    latitudeSpin_->setDecimals(5);
    coordForm->addRow("Latitude:", latitudeSpin_);

    longitudeSpin_ = new QDoubleSpinBox(coordGroup);
    longitudeSpin_->setRange(-180.0, 180.0);
    longitudeSpin_->setDecimals(5);
    coordForm->addRow("Longitude:", longitudeSpin_);
    connect(latitudeSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &LaunchSitePanel::onCoordinateSpinChanged);
    connect(longitudeSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &LaunchSitePanel::onCoordinateSpinChanged);

    elevationSpin_ = new QDoubleSpinBox(coordGroup);
    elevationSpin_->setRange(-500.0, 6000.0);
    elevationSpin_->setSuffix(" m ASL");
    connect(elevationSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &LaunchSitePanel::onRailOrWindInputsChanged);
    coordForm->addRow("Elevation:", elevationSpin_);

    railLengthSpin_ = new QDoubleSpinBox(coordGroup);
    railLengthSpin_->setRange(0.3, 10.0);
    railLengthSpin_->setSingleStep(0.1);
    railLengthSpin_->setSuffix(" m");
    railLengthSpin_->setValue(1.0);
    connect(railLengthSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &LaunchSitePanel::onRailOrWindInputsChanged);
    coordForm->addRow("Rail length:", railLengthSpin_);

    railAngleSpin_ = new QDoubleSpinBox(coordGroup);
    railAngleSpin_->setRange(0.0, 45.0);
    railAngleSpin_->setSuffix(" deg from vertical");
    connect(railAngleSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &LaunchSitePanel::onRailOrWindInputsChanged);
    coordForm->addRow("Rail angle:", railAngleSpin_);

    leftLayout->addWidget(coordGroup);

    auto* weatherGroup = new QGroupBox("Weather", leftColumn);
    auto* weatherLayout = new QVBoxLayout(weatherGroup);

    fetchWeatherButton_ = new QPushButton("Fetch Weather", weatherGroup);
    connect(fetchWeatherButton_, &QPushButton::clicked, this, &LaunchSitePanel::onFetchWeatherClicked);
    weatherLayout->addWidget(fetchWeatherButton_);

    weatherStatusLabel_ = new QLabel(weatherGroup);
    weatherStatusLabel_->setWordWrap(true);
    weatherLayout->addWidget(weatherStatusLabel_);

    surfaceConditionsLabel_ = new QLabel(weatherGroup);
    surfaceConditionsLabel_->setWordWrap(true);
    weatherLayout->addWidget(surfaceConditionsLabel_);

    windAloftLabel_ = new QLabel(weatherGroup);
    windAloftLabel_->setWordWrap(true);
    weatherLayout->addWidget(windAloftLabel_);

    useLiveWindCheck_ = new QCheckBox("Use live wind-aloft profile for flights", weatherGroup);
    connect(useLiveWindCheck_, &QCheckBox::toggled, this, &LaunchSitePanel::onUseLiveWindToggled);
    weatherLayout->addWidget(useLiveWindCheck_);

    auto* manualWindForm = new QFormLayout();
    manualWindSpeedSpin_ = new QDoubleSpinBox(weatherGroup);
    manualWindSpeedSpin_->setRange(0.0, 30.0);
    manualWindSpeedSpin_->setSingleStep(0.5);
    manualWindSpeedSpin_->setSuffix(" m/s");
    connect(manualWindSpeedSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &LaunchSitePanel::onRailOrWindInputsChanged);
    manualWindForm->addRow("Manual ground wind speed:", manualWindSpeedSpin_);

    manualWindDirectionSpin_ = new QDoubleSpinBox(weatherGroup);
    manualWindDirectionSpin_->setRange(0.0, 359.0);
    manualWindDirectionSpin_->setSingleStep(15.0);
    manualWindDirectionSpin_->setSuffix(" deg (from north)");
    connect(manualWindDirectionSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &LaunchSitePanel::onRailOrWindInputsChanged);
    manualWindForm->addRow("Manual wind direction:", manualWindDirectionSpin_);
    weatherLayout->addLayout(manualWindForm);

    leftLayout->addWidget(weatherGroup);
    leftLayout->addStretch(1);

    map_ = new MapTileWidget(this);
    connect(map_, &MapTileWidget::locationPicked, this, &LaunchSitePanel::onMapLocationPicked);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftColumn);
    splitter->addWidget(map_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    onRailOrWindInputsChanged();  // push initial spin box defaults into launchSite_
}

void LaunchSitePanel::onMapLocationPicked(double latitude, double longitude) {
    const QSignalBlocker latBlocker(latitudeSpin_);
    const QSignalBlocker lonBlocker(longitudeSpin_);
    latitudeSpin_->setValue(latitude);
    longitudeSpin_->setValue(longitude);
    launchSite_->setCoordinates(latitude, longitude);
}

void LaunchSitePanel::onCoordinateSpinChanged() {
    launchSite_->setCoordinates(latitudeSpin_->value(), longitudeSpin_->value());
    map_->setMarker(latitudeSpin_->value(), longitudeSpin_->value());
}

void LaunchSitePanel::onRailOrWindInputsChanged() {
    launchSite_->setElevationM(elevationSpin_->value());
    launchSite_->setRailLengthM(railLengthSpin_->value());
    launchSite_->setRailAngleDeg(railAngleSpin_->value());
    launchSite_->setManualWind(manualWindSpeedSpin_->value(), manualWindDirectionSpin_->value());
}

void LaunchSitePanel::onFetchWeatherClicked() {
    if (!launchSite_->hasCoordinates()) {
        weatherStatusLabel_->setText("Pick a launch site on the map (or enter lat/lon) first.");
        return;
    }

    weatherStatusLabel_->setText("Fetching weather...");

    data::NetworkHttpTransport transport;
    data::NwsClient nws(transport);
    data::OpenMeteoClient openMeteo(transport);
    data::WeatherService service(nws, openMeteo);
    const data::LaunchSiteWeather weather =
        service.fetchLaunchSiteWeather(launchSite_->latitude(), launchSite_->longitude());

    launchSite_->setWeather(weather);

    weatherStatusLabel_->setText(weather.surface.valid
                                      ? QString("Weather fetched (surface source: %1).")
                                            .arg(weather.surface.source)
                                      : "Could not fetch surface conditions (check network connection).");
    refreshWeatherReadout();

    if (weather.windAloft.levels.isEmpty()) {
        useLiveWindCheck_->setEnabled(false);
        useLiveWindCheck_->setChecked(false);
    } else {
        useLiveWindCheck_->setEnabled(true);
    }
}

void LaunchSitePanel::refreshWeatherReadout() {
    const std::optional<data::LaunchSiteWeather>& weather = launchSite_->weather();
    if (!weather) {
        surfaceConditionsLabel_->clear();
        windAloftLabel_->clear();
        return;
    }

    if (weather->surface.valid) {
        surfaceConditionsLabel_->setText(
            QString("Surface: %1 C, %2 hPa, wind %3 m/s from %4 deg (gust %5 m/s)")
                .arg(weather->surface.temperatureC, 0, 'f', 1)
                .arg(weather->surface.pressureHpa, 0, 'f', 0)
                .arg(weather->surface.surfaceWindSpeedMs, 0, 'f', 1)
                .arg(weather->surface.surfaceWindDirectionDeg, 0, 'f', 0)
                .arg(weather->surface.windGustMs, 0, 'f', 1));
    } else {
        surfaceConditionsLabel_->setText("Surface conditions unavailable.");
    }

    windAloftLabel_->setText("Wind aloft:\n" + formatWindAloft(weather->windAloft));
}

void LaunchSitePanel::onUseLiveWindToggled(bool checked) {
    launchSite_->setUseLiveWind(checked);
    manualWindSpeedSpin_->setEnabled(!checked);
    manualWindDirectionSpin_->setEnabled(!checked);
}

void LaunchSitePanel::onFlightCompleted(double landingOffsetEastM, double landingOffsetNorthM) {
    if (!launchSite_->hasCoordinates()) return;

    const double originLat = launchSite_->latitude();
    const double originLon = launchSite_->longitude();
    // Flat-earth approximation -- plenty accurate at typical model-rocket
    // drift distances (tens to low hundreds of meters).
    const double landingLat = originLat + landingOffsetNorthM / kMetersPerDegreeLat;
    const double landingLon =
        originLon + landingOffsetEastM / (kMetersPerDegreeLat * std::cos(qDegreesToRadians(originLat)));

    map_->setLandingMarker(landingLat, landingLon);
}

}  // namespace apogee::app
