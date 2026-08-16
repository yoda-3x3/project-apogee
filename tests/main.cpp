// Custom main (supersedes catch_amalgamated.cpp's default one, via the
// CATCH_AMALGAMATED_CUSTOM_MAIN compile definition in CMakeLists.txt):
// QSqlDatabase's plugin loading machinery segfaults without a live
// QCoreApplication instance (confirmed via gdb -- "QSqlDatabase requires a
// QCoreApplication" followed by a crash inside Qt6Sql.dll's open()), and
// the data-layer tests exercise real QSqlDatabase/SQLite connections.
#include <catch_amalgamated.hpp>

#include <QCoreApplication>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // seed.qrc is compiled into rocket_data, a STATIC library -- its
    // resource-registering static initializer only survives linking if
    // something forces that object file in, hence this explicit call
    // (without it, :/seed/*.json silently doesn't exist at runtime: no
    // error, QFile::open() just returns false).
    Q_INIT_RESOURCE(seed);
    return Catch::Session().run(argc, argv);
}
