# Movesi

Movesi is a lightweight, zero-dependency system tray (Windows) and menu bar (macOS) utility designed to prevent session timeouts, screen locks, and system sleep. It works by simulating user activity at randomized micro-intervals. 

Movesi is designed to be fully offline, private (no telemetry), and extremely resource-efficient.

## Features & Memory Optimization
* **Extremely Lightweight**: 
  - **Windows**: Built as a native Win32 C++ application with custom GDI double-buffered rendering (no heavy frameworks like WPF or Electron). It uses a dynamic memory-trimming technique (`EmptyWorkingSet` via `psapi.dll`) upon startup and window minimize/hide events. This keeps active memory consumption at **~0.6MB RAM** (600KB) and compiles to a single **~150KB** executable.
  - **macOS**: Built in native Swift and SwiftUI as a status menu agent.
* **Launch Minimized**: Opens directly to the system tray/menu bar on startup to stay out of your way.
* **Runtime Icon Generation**: Renders status indicators dynamically. For example, on Windows, it draws a bright emerald green dot for active status and a custom white ring with a grey center for paused status, ensuring visibility on both dark and light taskbars.
* **Theme Adaptability**: Dynamically queries the OS theme and monitors registry changes (`WM_SETTINGCHANGE`) to repaint dark or light themes live.
* **Simulation Actions**:
  - **Mouse Move**: Tiny micro-movement (1-3px offset) from current coordinates so it's visually invisible.
  - **Mouse Click**: Simulates a left-click in place.
  - **Key Press**: Sends a neutral `VK_F15` key event (F15 function key) which acts as a dead key on modern systems, meaning it does not disrupt active typing or gaming.
* **Automation Schedules**:
  - **Work Hours**: Automatically runs between 9:00 AM and 5:00 PM, Monday through Friday.
  - **Blackout Window**: Automatically pauses between 6:00 PM and 9:00 AM on weekdays and all day on weekends. Stats freeze when paused and resume accumulating when restarted.

---

## Windows Version (Native Win32 C++)

### Prerequisites
* **MinGW Compiler**: Ensure `g++` and `windres` are installed. Common distributions include [MSYS2](https://www.msys2.org/) or [MinGW-w64](https://www.mingw-w64.org/). Ensure the bin folder (e.g. `C:\msys64\mingw64\bin` or `C:\MinGW\bin`) is added to your system `PATH`.

### Compilation
Simply run the included build script in the root directory:
```cmd
build.bat
```
This script compiles the Windows manifest, resource metadata, and C++ source code to produce a single executable: `Movesi.exe`.

### Running & System Tray
Double-click `Movesi.exe`. It will launch directly into the system tray. 
* **Left-click or Double-click** the tray icon to open the settings interface.
* **Right-click** the tray icon to quickly Toggle session protection or Exit the application.
* **Close button (X)** hides the interface back to the system tray. Use **Exit** from the tray menu to fully shut down.

### SendInput & UAC Elevation (Windows Security)
> [!NOTE]
> If you have a window focused that is running with Administrator privileges (such as Task Manager, Command Prompt, or remote support tools), Windows User Interface Privilege Isolation (UIPI) will block simulated inputs from a non-elevated `Movesi.exe`. To simulate clicks or keypresses inside these windows, you must run `Movesi.exe` as Administrator (right-click -> **Run as administrator**).

### Adding to Startup Manually
To make Movesi launch automatically on system logon:
1. Open the **Run** dialog (Win + R).
2. Type `shell:startup` and press Enter. This opens the Startup folder.
3. Right-click inside the folder and select **New > Shortcut**.
4. Browse to the location of your compiled `Movesi.exe` and create the shortcut.

### Uninstallation
1. Exit Movesi via the system tray menu.
2. Delete `Movesi.exe`.
3. If you added a startup shortcut, open `shell:startup` and delete the Movesi shortcut.
4. Movesi does not write user files, AppData, or registry keys outside of standard temporary OS caches (which are cleared on reboot).

---

## macOS Version (SwiftUI / Cocoa)

### Prerequisites
* **macOS 11.0+** (for SF Symbols support)
* **Xcode 12.0+**

### Compilation
You can compile the app using the command line via Xcode tools:
```bash
swiftc -O -sdk $(xcrun --show-sdk-path --sdk macosx) -target x86_64-apple-macos11.0 -o Movesi macos/MovesiApp.swift macos/StatusController.swift
```

### Accessibility & Screen Recording Permissions
Because macOS restricts applications from programmatically posting events to safeguard user privacy, you must grant permissions:
1. **Accessibility Permission**:
   - The UI displays a warning card if permission is missing. Click **"Grant Permission"** or navigate to **System Settings > Privacy & Security > Accessibility**.
   - Authenticate, click the `+` button, select `Movesi`, and toggle the switch to **On**.
2. **Screen Recording Permission (Sonoma 14+)**:
   - On macOS Sonoma and later, CoreGraphics coordinate calculations for mouse movement simulation may require **Screen Recording** permissions.
   - If mouse movements fail to post, verify that Movesi is enabled in **System Settings > Privacy & Security > Screen Recording**.

### Code Signing & Notarization
Unsigned applications are blocked by macOS Gatekeeper.

#### For Local Testing (Self-signing)
To bypass Gatekeeper on your local machine, apply a ad-hoc signature:
```bash
codesign --force --deep --sign - Movesi.app
```

#### For Distribution (Official Developer ID Notarization)
To distribute Movesi to other users, sign it using a valid Apple Developer Account ID and submit it to Apple's Notarization service:
1. Sign with your Developer ID Application certificate:
   ```bash
   codesign --force --options runtime --deep --sign "Developer ID Application: Your Name (TeamID)" Movesi.app
   ```
2. Compress the signed `.app` into a `.zip`:
   ```bash
   ditto -c -k --sequesterRsrc Movesi.app Movesi.zip
   ```
3. Upload to Apple notary service:
   ```bash
   xcrun notarytool submit Movesi.zip --apple-id "your-apple-id@email.com" --password "your-app-specific-password" --team-id "YourTeamID" --wait
   ```
4. Staple the notarization ticket to the bundle:
   ```bash
   xcrun stapler staple Movesi.app
   ```

### Uninstallation
1. Exit Movesi by clicking the menu bar icon and selecting **Exit**.
2. Drag `Movesi.app` to the Trash.
3. Open **System Settings > Privacy & Security > Accessibility** and remove `Movesi` from the list.
