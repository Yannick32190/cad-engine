#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <iostream>
#include "ui/mainwindow/MainWindow.h"

int main(int argc, char *argv[]) {
#ifdef _WIN32
    // Force desktop OpenGL (not ANGLE) with legacy Compatibility Profile
    qputenv("QT_OPENGL", "desktop");
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 1);  // Request 2.1: guarantees Compatibility Profile on Intel/AMD/Nvidia
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    QApplication app(argc, argv);
    
    app.setApplicationName("CAD-ENGINE");
    app.setApplicationVersion("0.8.0");
    app.setOrganizationName("CAD-ENGINE");
    app.setOrganizationDomain("cad-engine.local");
    
    // Portable mode: set OCCT env vars relative to app dir
    QString appDir = QApplication::applicationDirPath();
    
    // For AppImage: CASROOT may be set by AppRun
    if (qEnvironmentVariableIsEmpty("CASROOT")) {
        QString occtShare = appDir + "/../share/opencascade";
        if (QDir(occtShare).exists()) {
            qputenv("CASROOT", occtShare.toUtf8());
            qputenv("CSF_OCCTResourcePath", (occtShare + "/resources").toUtf8());
        }
    }
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
