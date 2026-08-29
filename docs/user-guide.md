# Luna's Stop Motion Studio — User Guide

## Requirements

- **Linux / Windows / macOS** with a display
- **Qt 6** (Ubuntu: `sudo apt install qt6-base-dev`)
- **ffmpeg** on your PATH — only required for video export (`sudo apt install ffmpeg`)

## Building from Source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/lsms
```

---

## Getting Started

### Opening Images

1. **File → Open Images…** (Ctrl+I)
2. A file browser opens with a live preview panel on the right — click any file to see it before selecting.
3. Select one or more image files (JPG, PNG, BMP, TIFF, WebP). Hold **Ctrl** or **Shift** to select multiple.
4. Click **Open**. The images are appended to the filmstrip in alphabetical order.

> **Tip:** Shoot your frames with zero-padded sequential names (e.g. `frame_001.jpg`, `frame_002.jpg`) so they sort in the correct order.

---

## The Interface

```
┌─────────────────────────────────────────────┐
│  Menu bar                                   │
├─────────────────────────────────────────────┤
│                                             │
│           Playback viewer                   │
│         (current frame here)                │
│                                             │
├─────────────────────────────────────────────┤
│  ⏮  ▶ Play  ⏹ Stop  ⏭      FPS: [12 ▲▼]  │  ← Transport bar
├─────────────────────────────────────────────┤
│  [1] [2] [3] [4] [5] [6] …                 │  ← Filmstrip
└─────────────────────────────────────────────┘
```

### Playback Viewer
The large area at the top shows the currently selected frame, scaled to fit while preserving the aspect ratio.

### Transport Bar

| Control | Action |
|---------|--------|
| ⏮ | Step to previous frame |
| ▶ Play / ⏸ Pause | Start or pause playback |
| ⏹ Stop | Stop playback and return to frame 1 |
| ⏭ | Step to next frame |
| FPS | Frames per second — drag or click to change (1–60) |

### Filmstrip
The horizontal strip at the bottom shows every frame as a numbered thumbnail.

- **Click** a thumbnail to jump to that frame.
- **Drag and drop** a thumbnail to reorder frames.
- **Right-click** a thumbnail for the context menu:
  - **Duplicate** — inserts a copy immediately after the selected frame
  - **Delete** — removes the frame

---

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Space | Play / Pause |
| Esc | Stop (return to frame 1) |
| ← | Previous frame |
| → | Next frame |
| Del | Delete selected frame |
| Ctrl+D | Duplicate selected frame |
| Ctrl+I | Open Images… |
| Ctrl+S | Save Project |
| Ctrl+Shift+S | Save Project As… |
| Ctrl+O | Open Project… |
| Ctrl+N | New Project |

---

## Projects

Projects are saved as `.lsms` files (JSON). Image paths are stored **relative** to the project file, so you can move the project folder to another location as long as you keep the images alongside it.

- **File → Save Project** (Ctrl+S) — save to the current file; prompts for a path on first save.
- **File → Open Project…** (Ctrl+O) — reopen a saved project.
- **File → New Project** (Ctrl+N) — start fresh (prompts to save unsaved changes).

---

## Exporting Video

> **Requires:** `ffmpeg` installed and available on your PATH.

1. **Export → Export as MP4…** or **Export as WebM…**
2. Choose an output file location.
3. A progress dialog appears while ffmpeg encodes the frames at the current FPS.

**MP4** (H.264, `yuv420p`) — best compatibility with video players and social platforms.  
**WebM** (VP9) — open format, smaller file size.

---

## Tips

- **Onion skinning** is not yet implemented. To compare frames, use the step buttons (← →) rapidly.
- If export fails, verify ffmpeg is installed: `ffmpeg -version` in a terminal.
- The FPS setting is saved with the project.
