#pragma once

#include <QByteArray>
#include <QUrl>

namespace apogee::data {

struct HttpResponse {
    int statusCode = 0;
    QByteArray body;
    bool networkError = false;
};

// Thin synchronous HTTP GET interface. Real requests (QNetworkTransport)
// block on a local QEventLoop until the reply finishes -- acceptable for a
// CLI tool and for repository-level clients that a GUI wraps in a
// background QThread (see SimulationWorker-style workers, Phase 4+).
// FixtureTransport (tests/) replays recorded JSON with no network at all.
class HttpTransport {
public:
    virtual ~HttpTransport() = default;
    virtual HttpResponse get(const QUrl& url) = 0;
};

}  // namespace apogee::data
