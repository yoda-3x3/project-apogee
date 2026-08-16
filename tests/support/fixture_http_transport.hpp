#pragma once

#include <QByteArray>
#include <QVector>

#include "data/http_transport.hpp"

// Replays recorded JSON fixtures instead of hitting the network. Requests
// are matched by substring against the request URL, in registration order,
// which is enough to disambiguate the handful of endpoints each client
// calls (points vs. gridpoints, search vs. download, etc.).
class FixtureHttpTransport : public apogee::data::HttpTransport {
public:
    void addFixture(const QString& urlSubstring, const QByteArray& body, int statusCode = 200) {
        entries_.push_back({urlSubstring, body, statusCode});
    }

    apogee::data::HttpResponse get(const QUrl& url) override {
        const QString urlString = url.toString();
        for (const Entry& entry : entries_) {
            if (urlString.contains(entry.urlSubstring)) {
                return {entry.statusCode, entry.body, false};
            }
        }
        return {404, {}, false};
    }

private:
    struct Entry {
        QString urlSubstring;
        QByteArray body;
        int statusCode;
    };
    QVector<Entry> entries_;
};
