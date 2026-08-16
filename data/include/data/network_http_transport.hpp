#pragma once

#include "data/http_transport.hpp"

class QNetworkAccessManager;

namespace apogee::data {

// Real network implementation, blocking synchronously on a local event loop.
// Sends a descriptive User-Agent on every request -- required by NWS policy
// (api.weather.gov) and good practice for Open-Meteo/ThrustCurve.org too.
class NetworkHttpTransport : public HttpTransport {
public:
    NetworkHttpTransport();
    ~NetworkHttpTransport() override;

    HttpResponse get(const QUrl& url) override;

private:
    QNetworkAccessManager* manager_;
};

}  // namespace apogee::data
