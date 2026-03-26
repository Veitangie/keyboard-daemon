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
1. Download the latest `.zip` release for your architecture and OS (Windows version is compiled solely against x86_64, as it will work on arm too) from the GitHub Releases page and extract it;
   1. **MacOS only** Open your terminal and navigate to the extracted folder;
   2. **MacOS only** Remove the Apple quarantine flag by running: `xattr -cr .`
2. Navigate to the extracted folder and run the included installation wizard from your terminal:
   - **Linux:** `./install_linux.sh`
   - **macOS:** `./install_mac.sh`
   - **Windows:** Right click `install_win.ps1` and select "Run with PowerShell" (or run it via terminal);
3. The interactive wizard will prompt you for your platform/layout arguments, copy the binary to your local app data, and automatically register it to start silently in the background.

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

## On Code Quality
This codebase was written in about 24 hours from start to finish by a person who has never written a single line of code in a language without managed memory, much less in C. This means that the codebase is a mess by any standard, but what's important is that it works, therefore I'm leaving it as is. However, for future updates if one ever comes, here's a known list of improvements:

1. Parser in `linux_hypr/parser.c` seems pretty convoluted for the task, and while it is robust enough as an algorithm, it would be better represented as a table-driven state machine to remove the `goto` jumps;
2. Start using fixed-length buffers (bounded by `PATH_MAX`) or heap allocations for file paths instead of variable length arrays to prevent potential stack smashing;
3. Add bounds checking to the Unix domain socket path in `main.c`. `strcpy` into `sockaddr_un.sun_path` risks a buffer overflow if the user's `XDG_RUNTIME_DIR` exceeds 108 bytes;
4. Extract magic numbers (like the 33-byte HID write chunks or the `0x06 0x60 0xFF` device signatures) into readable `#define` macros;
5. Fix the parsing logic in the sway, kde and gnome implementations. I was getting lazy by that point, so there's no handling of message split across buffer boundary. The buffer is big enough for that to be a highly unlikely problem, but there's still a possibility, and it's a dirty solution.

## Support & Contributing

The feature set of this tool is intentionally very tight and tailored to personal needs. 

You (all two of you other ZSA keyboard users who need different layouts for different locales, and of course you, Greg) are welcome to open Issues or Pull Requests if you run into problems or have improvements, but please note that this is a personal project and I cannot guarantee they will be reviewed, fixed, or merged. 

> **Platform Support Note:** The main target for me personally is **Linux with Hyprland**, so I can't guarantee that I will be able to help you with other platforms.

Feel free to fork the repository under the terms of the GNU GPLv3 license!
