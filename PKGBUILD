# Maintainer: kolosp
pkgname=aeyes-git
pkgver=0.1.0.r0.g0000000
pkgrel=1
pkgdesc='Ask an AI about anything on your screen (KDE Plasma, Circle-to-Search style)'
arch=(x86_64)
url='https://github.com/kolosp/aeyes'
license=(GPL-3.0-or-later)
depends=(qt6-base qt6-declarative qt6-webengine kirigami qqc2-desktop-style ki18n kconfig kcoreaddons
         kdbusaddons kcrash qtkeychain-qt6 spectacle)
makedepends=(git cmake ninja extra-cmake-modules)
checkdepends=(nodejs appstream)
provides=(aeyes)
conflicts=(aeyes)
source=("git+$url.git")
sha256sums=(SKIP)

pkgver() {
  cd aeyes
  printf '0.1.0.r%s.g%s' "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cmake -S aeyes -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}

check() {
  # The AppStream test validates the files listed in install_manifest.txt.
  cmake --install build --prefix "$srcdir/stage"
  QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
