#pragma once

namespace apogee::core {

struct AtmosphericState {
    double temperatureK = 0;
    double pressurePa = 0;
    double densityKgM3 = 0;
    double speedOfSoundMs = 0;
};

// Standard ISA piecewise model (troposphere 0-11km, stratosphere 11-20km).
// altitudeMslM is height above mean sea level, not above the launch site --
// callers add launch-site elevation to AGL themselves. g0 is treated as
// constant (9.80665) throughout -- the altitude range model rockets fly at
// is far too small for g(h) to matter.
AtmosphericState standardAtmosphere(double altitudeMslM);

}  // namespace apogee::core
