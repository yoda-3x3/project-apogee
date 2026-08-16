#include "core/atmosphere.hpp"

#include <cmath>

namespace apogee::core {

namespace {
constexpr double kSeaLevelTemperatureK = 288.15;
constexpr double kSeaLevelPressurePa = 101325.0;
constexpr double kTroposphereLapseRateKPerM = 0.0065;
constexpr double kTropopauseAltitudeM = 11000.0;
constexpr double kStratosphereTemperatureK = 216.65;  // constant through 11-20km
constexpr double kGravityMs2 = 9.80665;
constexpr double kSpecificGasConstantAir = 287.05;  // J/(kg*K)
constexpr double kAdiabaticIndexAir = 1.4;
}  // namespace

AtmosphericState standardAtmosphere(double altitudeMslM) {
    AtmosphericState s;

    if (altitudeMslM <= kTropopauseAltitudeM) {
        s.temperatureK = kSeaLevelTemperatureK - kTroposphereLapseRateKPerM * altitudeMslM;
        s.pressurePa = kSeaLevelPressurePa *
                       std::pow(s.temperatureK / kSeaLevelTemperatureK,
                                kGravityMs2 / (kSpecificGasConstantAir * kTroposphereLapseRateKPerM));
    } else {
        const double tropopauseTemperatureK =
            kSeaLevelTemperatureK - kTroposphereLapseRateKPerM * kTropopauseAltitudeM;
        const double tropopausePressurePa =
            kSeaLevelPressurePa *
            std::pow(tropopauseTemperatureK / kSeaLevelTemperatureK,
                     kGravityMs2 / (kSpecificGasConstantAir * kTroposphereLapseRateKPerM));

        s.temperatureK = kStratosphereTemperatureK;
        s.pressurePa = tropopausePressurePa *
                       std::exp(-kGravityMs2 * (altitudeMslM - kTropopauseAltitudeM) /
                                (kSpecificGasConstantAir * kStratosphereTemperatureK));
    }

    s.densityKgM3 = s.pressurePa / (kSpecificGasConstantAir * s.temperatureK);
    s.speedOfSoundMs = std::sqrt(kAdiabaticIndexAir * kSpecificGasConstantAir * s.temperatureK);
    return s;
}

}  // namespace apogee::core
