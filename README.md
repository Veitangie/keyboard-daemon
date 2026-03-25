# Keyboard Daemon

This is a pretty simple program that monitors locale switching in the OS/display server/window manager/desktop environment and pushes commands to the keyboard to switch to the corresponding layer.

The mapping can be passed as arguments when launching the program - they will be treated as 0-indexed layer mapping, so something like `en ru` will be treated as:
- `en` = layer 0  
- `ru` = layer 1  

If you need to skip a layer (e.g. make a mapping Norwegian - layer 0, English - layer 2) then just use any character sequence that isn't part of any of your locales (`no - en` works, and `no oeinnhvtsatsafatrstarsta en` does too). The matching is done as case-insensitive prefix matching, so for most double-locale setups you can use a single letter (`f e` for French and English, for example).

> **NOTE:** MacOS is unique in the way they treat locales, so you should use the shortest unique **substring** for it instead of shortest unique prefix.

---

## Installation

You can either download a pre-compiled release for your OS or build from source.

### Option 1: Pre-compiled Release (Recommended)
1. Download the latest `.zip` release for your OS from the GitHub Releases page.
2. Extract the archive and run the included installation wizard from your terminal:
   - **Linux:** `./install_linux.sh`
   - **macOS:** `./install_mac.sh`
   - **Windows:** Right click `install_win.ps1` and select "Run with PowerShell" (or run it via terminal).
3. The interactive wizard will prompt you for your platform/layout arguments, copy the binary to your local app data, and automatically register it to start silently in the background!

### Option 2: Build from Source
If you are adapting the daemon for your own needs, you can easily compile and install it using `make`.

> **Arch Linux Note:** A standard `PKGBUILD` is included! You can simply clone the repo and run `makepkg -si` to install the daemon natively via `pacman`.

1. Clone the repository.
2. Run the make target for your specific display server or OS:
   - Linux: `make hyprland`, `make gnome`, `make kde`, `make sway`, `make x11`
   - macOS: `make macos`
   - Windows: `make win`
3. The interactive installer will launch automatically.
4. If you wish to bypass the interactive prompts, you can pass your layout directly: `make hyprland en ru`

---

## Support & Contributing

The feature set of this tool is intentionally very tight and tailored to personal needs. 

You (all two of you other ZSA keyboard users who need different layouts for different locales, and of course you, Greg) are welcome to open Issues or Pull Requests if you run into problems or have improvements, but please note that this is a personal project and I cannot guarantee they will be reviewed, fixed, or merged. 

> **Platform Support Note:** The main target for me personally is **Linux with Hyprland**, so I can't guarantee that I will be able to help you with other platforms.

Feel free to fork the repository under the terms of the GNU GPLv3 license!
