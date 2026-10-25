cmake_minimum_required(VERSION 3.20)
project(aeyes VERSION 0.1.0 LANGUAGES CXX)

set(QT_MIN_VERSION 6.6.0)
set(KF_MIN_VERSION 6.5.0)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

set(CMAKE_SKIP_INSTALL_RPATH ON) # only system libraries are linked dynamically
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Sql WebEngineQuick Test)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons Config DBusAddons I18n Crash Kirigami)
find_package(Qt6Keychain REQUIRED)

# Core logic, shared by the app and the tests.
add_library(aeyes_core STATIC
    src/imageutil.cpp
    src/providers/provider.cpp
    src/providers/geminiprovider.cpp
    src/providers/openaiprovider.cpp
    src/store/historystore.cpp
    src/store/sessionmodel.cpp
    src/settings/settings.cpp
    src/settings/secretstore.cpp
    src/capture/spectaclecapture.cpp
    src/chat.cpp
    src/controller.cpp
)
target_include_directories(aeyes_core PUBLIC src)
target_link_libraries(aeyes_core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Network Qt6::Sql
    KF6::CoreAddons KF6::ConfigCore KF6::I18n
    qt6keychain
)
target_compile_definitions(aeyes_core PUBLIC AEYES_VERSION="${PROJECT_VERSION}")

qt_policy(SET QTP0001 NEW) # QML module resources live under :/qt/qml
qt_policy(SET QTP0004 NEW)
add_executable(aeyes src/main.cpp)
qt_add_qml_module(aeyes
    URI org.kde.aeyes.ui
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/ErrorBar.qml
        src/qml/OnboardingPage.qml
        src/qml/ConfirmPage.qml
        src/qml/ChatPage.qml
        src/qml/HistoryPage.qml
        src/qml/SettingsPage.qml
    NO_PLUGIN
)

file(GLOB_RECURSE WEB_FILES CONFIGURE_DEPENDS resources/web/*)
list(FILTER WEB_FILES EXCLUDE REGEX "LICENSE")
qt_add_resources(aeyes web
    PREFIX /web
    BASE resources/web
    FILES ${WEB_FILES}
)

target_link_libraries(aeyes PRIVATE
    aeyes_core
    Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::WebEngineQuick
    KF6::DBusAddons KF6::Crash KF6::I18nQml
)

install(TARGETS aeyes ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS data/org.kde.aeyes.desktop DESTINATION ${KDE_INSTALL_APPDIR})
# Makes Plasma list the app (and its X-KDE-Shortcuts) under System Settings → Keyboard → Shortcuts.
install(FILES data/org.kde.aeyes.desktop DESTINATION ${KDE_INSTALL_DATADIR}/kglobalaccel)
install(FILES data/org.kde.aeyes.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES data/org.kde.aeyes.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
foreach(sz 48 128 256)
    install(FILES data/org.kde.aeyes-${sz}.png DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/${sz}x${sz}/apps RENAME org.kde.aeyes.png)
endforeach()

enable_testing()
add_subdirectory(tests)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
