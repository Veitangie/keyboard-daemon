# Maintainer: Veitangie

pkgbase=kbd-daemon-git
pkgname=(
  'kbd-daemon-hyprland-git'
  'kbd-daemon-gnome-git'
  'kbd-daemon-kde-git'
  'kbd-daemon-sway-git'
  'kbd-daemon-x11-git'
)
pkgver=r1.0
pkgrel=1
arch=('x86_64' 'aarch64' 'armv7h')
url="https://github.com/Veitangie/keyboard-daemon"
license=('GPL3')
depends=('glibc' 'systemd')
makedepends=('git' 'make' 'gcc' 'libx11')
source=("git+https://github.com/Veitangie/keyboard-daemon.git")
md5sums=('SKIP')

pkgver() {
  cd "$srcdir/kbd"
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cd "$srcdir/kbd"
  make build-hyprland && mv kbd kbd_hyprland
  make build-gnome && mv kbd kbd_gnome
  make build-kde && mv kbd kbd_kde
  make build-sway && mv kbd kbd_sway
  make build-x11 && mv kbd kbd_x11
}

_package_common() {
  cd "$srcdir/kbd"
  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/${pkgname}/LICENSE"
}

package_kbd-daemon-hyprland-git() {
  pkgdesc="A lightweight daemon to sync OS locale with keyboard layers (Hyprland)"
  provides=('kbd-daemon')
  conflicts=('kbd-daemon')
  install -Dm755 "$srcdir/kbd/kbd_hyprland" "$pkgdir/usr/bin/kbd-daemon"
  _package_common
}

package_kbd-daemon-gnome-git() {
  pkgdesc="A lightweight daemon to sync OS locale with keyboard layers (GNOME)"
  provides=('kbd-daemon')
  conflicts=('kbd-daemon')
  install -Dm755 "$srcdir/kbd/kbd_gnome" "$pkgdir/usr/bin/kbd-daemon"
  _package_common
}

package_kbd-daemon-kde-git() {
  pkgdesc="A lightweight daemon to sync OS locale with keyboard layers (KDE)"
  provides=('kbd-daemon')
  conflicts=('kbd-daemon')
  install -Dm755 "$srcdir/kbd/kbd_kde" "$pkgdir/usr/bin/kbd-daemon"
  _package_common
}

package_kbd-daemon-sway-git() {
  pkgdesc="A lightweight daemon to sync OS locale with keyboard layers (Sway)"
  provides=('kbd-daemon')
  conflicts=('kbd-daemon')
  install -Dm755 "$srcdir/kbd/kbd_sway" "$pkgdir/usr/bin/kbd-daemon"
  _package_common
}

package_kbd-daemon-x11-git() {
  pkgdesc="A lightweight daemon to sync OS locale with keyboard layers (X11)"
  depends+=('glibc' 'systemd' 'libx11')
  provides=('kbd-daemon')
  conflicts=('kbd-daemon')
  install -Dm755 "$srcdir/kbd/kbd_x11" "$pkgdir/usr/bin/kbd-daemon"
  _package_common
}
