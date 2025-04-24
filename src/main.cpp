
#include "probescopewindow.h"
#include <QTranslator>
#include <qobject.h>
// #include <qtermwidget.h>
#include <singleapplication.h>

#ifdef Q_OS_WIN
// clang-format off
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DbgHelp.h>
#include <ShlObj.h>

LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo);
// clang-format on
#endif

int main(int argc, char **argv) {
    SingleApplication app(argc, argv);

#ifdef Q_OS_WIN
    // SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
#endif

    // Add probelibs as a library path
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/probelibs");

    // Prepare for QSettings
    app.setOrganizationName("RigoLigoRLC");
    app.setOrganizationDomain("rigoligo.cc");
    app.setApplicationName("ProbeScope");

    QIcon::setThemeName("light"); // TODO: Make this configurable

    // qApp->setStyle("windowsvista"); // Fuck QWindows11Style

#ifdef Q_OS_WIN
    // Workaround for Qt5 font issue
    if (QLocale::system().language() == QLocale::Chinese) {
        auto currentFont = qApp->font();
        qApp->setFont(QFont("Microsoft YaHei UI", currentFont.pointSize(), currentFont.weight()));
    }
    // Add probelibs to PATH
    qputenv("PATH", qgetenv("PATH") + ";" + QCoreApplication::applicationDirPath().toUtf8() + "/probelibs");
#endif

    QTranslator translator;
    if (translator.load(QLocale(), "probescope", "_", ":/lang", ".qm")) {
        QCoreApplication::installTranslator(&translator);
    }

    ProbeScopeWindow window;
    window.show();

    return app.exec();
}

#ifdef Q_OS_WIN
LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo) {
    wchar_t dumpPath[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, dumpPath);
    wcscat_s(dumpPath, L"\\ProbeScope\\CrashDumps");

    // Ensure directory exists
    CreateDirectoryW(dumpPath, nullptr);

    // Create file name with timestamp
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t filename[MAX_PATH];
    swprintf_s(filename, L"%s\\crash_%04d%02d%02d_%02d%02d%02d.dmp", dumpPath, st.wYear, st.wMonth, st.wDay, st.wHour,
               st.wMinute, st.wSecond);

    // Create file
    HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Write dump
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionInfo;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithDataSegs, &mdei, nullptr,
                          nullptr);
        CloseHandle(hFile);
    }

    // Notify user
    QMessageBox::critical(
        nullptr, QObject::tr("Crash"),
        QObject::tr(
            "The application has crashed.\n\nA crash dump was saved and please send it back to the developer:\n%1")
            .arg(QString(filename)));

    // Open the directory
    QProcess::startDetached("explorer.exe", {QString::fromWCharArray(dumpPath)});

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
