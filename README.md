# Luna's Stop Motion Studio

A desktop stop motion animation tool built with C++ and Qt6.

![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-blue)

## Prerequisites

| Dependency | Version | Install (Ubuntu/Debian) |
|------------|---------|------------------------|
| GCC or Clang | C++17 capable | `sudo apt install build-essential` |
| CMake | 3.20+ | `sudo apt install cmake` |
| Qt6 Base | 6.x | `sudo apt install qt6-base-dev` |
| ffmpeg | any recent | `sudo apt install ffmpeg` *(export only)* |

Install everything at once on Ubuntu 24.04:

```bash
sudo apt install build-essential cmake qt6-base-dev ffmpeg
```

## Building

```bash
git clone https://github.com/AntonGronholm1975/lsms.git
cd lsms
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The binary is `build/lsms`.

## Running

```bash
./build/lsms
```

> **Note (VS Code on Ubuntu snap):** If you launch from VS Code's integrated terminal, use the provided **Build & Run** task (`Ctrl+Shift+P → Tasks: Run Task → Build & Run`) which works around a snap/libpthread conflict automatically.

## Features

- **Open images** — JPG, PNG, BMP, TIFF, WebP with live preview in the file dialog
- **Filmstrip editor** — drag-and-drop reorder, right-click duplicate/delete
- **Playback** — play/pause/stop, frame stepping, 1–60 FPS
- **Onion skinning** — shows previous 1–2 frames as red-tinted ghosts while editing
- **Non-destructive crop** — drag a crop region per-frame or across all frames; stored in the project file
- **Drawing overlay** — annotate frames with pen, line, rectangle, ellipse, or eraser (not exported to video)
- **Projects** — save/load `.lsms` project files (relative image paths, portable)
- **Export** — MP4 (H.264) and WebM (VP9) via ffmpeg (crop is applied to the exported video)

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Space` | Play / Pause |
| `Esc` | Stop (return to frame 1) |
| `←` / `→` | Previous / Next frame |
| `Del` | Delete selected frame |
| `Ctrl+D` | Duplicate selected frame |
| `Ctrl+K` | Crop frame… |
| `Ctrl+I` | Open Images… |
| `Ctrl+S` | Save Project |
| `Ctrl+O` | Open Project… |
| `Ctrl+N` | New Project |

See [docs/user-guide.md](docs/user-guide.md) for the full user guide.

## Project Structure

```
lsms/
├── src/
│   ├── main.cpp
│   ├── MainWindow.{h,cpp}       # Application shell, menus, toolbars
│   ├── PlaybackWidget.{h,cpp}   # Frame viewer, onion skin, drawing overlay
│   ├── FilmstripWidget.{h,cpp}  # Thumbnail strip with drag-drop
│   ├── CropDialog.{h,cpp}       # Interactive crop-region selector
│   ├── Project.{h,cpp}          # Data model and .lsms JSON persistence
│   └── VideoExporter.{h,cpp}    # ffmpeg-based MP4/WebM export
├── resources/
│   └── app.qrc
├── docs/
│   └── user-guide.md
└── CMakeLists.txt
```

## License

MIT
