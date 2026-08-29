#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QDir>
#include <QUrl>
#include <iostream>
#include "ui/ViewportItem.hpp"

int main(int argc, char *argv[]) {
    // Configure OpenGL Surface Format for smooth rendering
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(4, 5);
    format.setSwapInterval(1); // Enable VSync
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    app.setApplicationName("Antigravity NLE Core");
    app.setOrganizationName("Antigravity Systems");

    // Register C++ ViewportItem to QML
    qmlRegisterType<antigravity::ui::ViewportItem>("Antigravity.Video", 1, 0, "ViewportItem");

    QQmlApplicationEngine engine;

    // Load QML interface
    const QString mainQmlPath = QString::fromUtf8(SRC_DIR) + "/src/ui/main.qml";
    QUrl url;
    if (QFile::exists(mainQmlPath)) {
        url = QUrl::fromLocalFile(mainQmlPath);
    } else {
        url = QUrl(QStringLiteral("qrc:/ui/main.qml"));
    }

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                std::cerr << "[Main] Error: Failed to load QML interface: "
                          << url.toString().toStdString() << std::endl;
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        std::cerr << "[Main] Failed to load QML root object." << std::endl;
        return -1;
    }

    // If an initial video file path was provided on command line
    if (argc > 1) {
        QString initialFile = QString::fromUtf8(argv[1]);
        auto rootObjs = engine.rootObjects();
        if (!rootObjs.isEmpty()) {
            QObject* rootWindow = rootObjs.first();
            auto* viewport = rootWindow->findChild<antigravity::ui::ViewportItem*>("viewport");
            if (viewport) {
                viewport->openFile(initialFile);
            }
        }
    }

    return app.exec();
}
