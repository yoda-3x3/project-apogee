#include "data/network_http_transport.hpp"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace apogee::data {

namespace {
constexpr const char* kUserAgent =
    "ProjectApogee/0.1 (model rocket flight simulator; "
    "https://github.com/yoda-3x3/project-apogee)";
}

NetworkHttpTransport::NetworkHttpTransport() : manager_(new QNetworkAccessManager()) {}

NetworkHttpTransport::~NetworkHttpTransport() {
    delete manager_;
}

HttpResponse NetworkHttpTransport::get(const QUrl& url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArray(kUserAgent));

    QNetworkReply* reply = manager_->get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    HttpResponse response;
    response.statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.body = reply->readAll();
    response.networkError = (reply->error() != QNetworkReply::NoError) && response.statusCode == 0;
    reply->deleteLater();
    return response;
}

}  // namespace apogee::data
