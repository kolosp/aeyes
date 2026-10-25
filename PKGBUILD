# Local development package: makepkg -si
pkgname=aeyes-git
pkgver=0.1.0
pkgrel=1
pkgdesc='Ask an AI about anything on your screen (KDE Plasma)'
arch=(x86_64)
license=(GPL-3.0-or-later)
depends=(qt6-base qt6-declarative qt6-webengine kirigami qqc2-desktop-style ki18n kconfig kcoreaddons
         kdbusaddons kcrash qtkeychain-qt6 spectacle)
makedepends=(cmake ninja extra-cmake-modules)
checkdepends=(nodejs)

build() {
  cmake -S "$startdir" -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}
check() { QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure; }
package() { DESTDIR="$pkgdir" cmake --install build; }
