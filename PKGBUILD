# Maintainer: Veitangie

pkgbase=kbd-daemon-git
pkgname=(
  'kbd-daemon-hyprland-git'
  'kbd-daemon-gnome-git'
  'kbd-daemon-plasma-git'
  'kbd-daemon-sway-git'
  'kbd-daemon-x11-git'
  'kbd-daemon-all-git'
)
pkgver=r2.aa89c20
pkgrel=1
arch=('x86_64' 'aarch64' 'armv7h')
url="https://github.com/Veitangie/keyboard-daemon"
license=('GPL3')

install=kbd.install

makedepends=('git' 'make' 'gcc')
depends_common=('glibc' 'systemd')

source=("git+https://github.com/Veitangie/keyboard-daemon.git")
md5sums=('SKIP')

pkgver() {
  cd "$srcdir/keyboard-daemon"
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cd "$srcdir/keyboard-daemon"

  make build-hyprland && mv kbd kbd_hyprland
  make build-gnome    && mv kbd kbd_gnome
  make build-plasma      && mv kbd kbd_plasma
  make build-sway     && mv kbd kbd_sway
  make build-x11      && mv kbd kbd_x11
}

_package_common() {
  cd "$srcdir/keyboard-daemon"
  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

_install_service() {
  local name="$1"
  local bin="$2"

  install -Dm644 /dev/stdin \
    "$pkgdir/usr/lib/systemd/user/kbd-daemon-${name}.service" <<EOF
[Unit]
Description=Keyboard Daemon (${name})
After=graphical-session.target

[Service]
ExecStart=/usr/bin/${bin}
Restart=always
RestartSec=2

[Install]
WantedBy=default.target
EOF
}

package_kbd-daemon-hyprland-git() {
  pkgdesc="Keyboard layout/locale sync daemon (Hyprland)"
  depends=("${depends_common[@]}")

  install -Dm755 "$srcdir/keyboard-daemon/kbd_hyprland" \
    "$pkgdir/usr/bin/kbd-daemon-hyprland"

  _install_service "hyprland" "kbd-daemon-hyprland"
  _package_common
}

package_kbd-daemon-gnome-git() {
  pkgdesc="Keyboard layout/locale sync daemon (GNOME)"
  depends=("${depends_common[@]}")

  install -Dm755 "$srcdir/keyboard-daemon/kbd_gnome" \
    "$pkgdir/usr/bin/kbd-daemon-gnome"

  _install_service "gnome" "kbd-daemon-gnome"
  _package_common
}

package_kbd-daemon-plasma-git() {
  pkgdesc="Keyboard layout/locale sync daemon (Plasma)"
  depends=("${depends_common[@]}")

  install -Dm755 "$srcdir/keyboard-daemon/kbd_plasma" \
    "$pkgdir/usr/bin/kbd-daemon-plasma"

  _install_service "plasma" "kbd-daemon-plasma"
  _package_common
}

package_kbd-daemon-sway-git() {
  pkgdesc="Keyboard layout/locale sync daemon (Sway)"
  depends=("${depends_common[@]}")

  install -Dm755 "$srcdir/keyboard-daemon/kbd_sway" \
    "$pkgdir/usr/bin/kbd-daemon-sway"

  _install_service "sway" "kbd-daemon-sway"
  _package_common
}

package_kbd-daemon-x11-git() {
  pkgdesc="Keyboard layout/locale sync daemon (X11)"
  depends=("${depends_common[@]}" 'libx11')

  install -Dm755 "$srcdir/keyboard-daemon/kbd_x11" \
    "$pkgdir/usr/bin/kbd-daemon-x11"

  _install_service "x11" "kbd-daemon-x11"
  _package_common
}

package_kbd-daemon-all-git() {
  pkgdesc="Keyboard layout/locale sync daemon (all backends)"
  depends=("${depends_common[@]}" 'libx11')

  cd "$srcdir/keyboard-daemon"

  install -Dm755 kbd_hyprland "$pkgdir/usr/bin/kbd-daemon-hyprland"
  install -Dm755 kbd_gnome    "$pkgdir/usr/bin/kbd-daemon-gnome"
  install -Dm755 kbd_plasma      "$pkgdir/usr/bin/kbd-daemon-plasma"
  install -Dm755 kbd_sway     "$pkgdir/usr/bin/kbd-daemon-sway"
  install -Dm755 kbd_x11      "$pkgdir/usr/bin/kbd-daemon-x11"

  _install_service "hyprland" "kbd-daemon-hyprland"
  _install_service "gnome"    "kbd-daemon-gnome"
  _install_service "plasma"      "kbd-daemon-plasma"
  _install_service "sway"     "kbd-daemon-sway"
  _install_service "x11"      "kbd-daemon-x11"

  _package_common
}
