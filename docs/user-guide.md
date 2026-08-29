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
│  ⏮  ▶ Play  ⏹ Stop  ⏭   Onion   FPS: [12]  │  ← Transport bar
├─────────────────────────────────────────────┤
│  ✏ Draw  Pen Line Rect Ellipse Eraser ■ Size  │  ← Drawing bar
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
| Onion | Toggle onion skinning (see below) |
| FPS | Frames per second — 1–60, default 12 |

### Filmstrip
The horizontal strip at the bottom shows every frame as a numbered thumbnail.

- **Click** a thumbnail to jump to that frame.
- **Drag and drop** a thumbnail to reorder frames.
- **Right-click** a thumbnail for the context menu:
  - **Duplicate** — inserts a copy immediately after the selected frame
  - **Delete** — removes the frame

---

## Onion Skinning

Click **Onion** in the transport bar to toggle. While enabled:

- The **previous frame** is shown as a red-tinted ghost at 60% opacity behind the current frame.
- The **frame before that** appears at 35% opacity.
- Onion skin is automatically hidden during playback and restored when you pause.
- A small hint reads “Onion: no previous frame” when you are on the first frame.

---

## Cropping

**Edit → Crop Frame…** (Ctrl+K)

1. A dialog opens with the current frame displayed.
2. **Drag** on the image to draw the crop rectangle. The area outside the selection is dimmed; the crop dimensions are shown in pixels.
3. Choose how to apply:
   - **Apply to This Frame** — crops only the current frame.
   - **Apply to All Frames** — applies the same crop to every frame in the project.
   - **Clear Crop** — removes the crop from all frames.

Crop is **non-destructive**: the original image files are never modified. The crop rectangle is saved in the `.lsms` project file and applied automatically during playback and video export.

---

## Drawing Overlay

The drawing bar sits between the transport bar and the filmstrip.

1. Click **✏ Draw** to enter drawing mode (the button stays pressed).
2. Select a tool:

| Tool | Description |
|------|-------------|
| Pen | Freehand brush stroke |
| Line | Click-drag straight line |
| Rect | Click-drag rectangle |
| Ellipse | Click-drag ellipse |
| Eraser | Erase previously drawn marks |

3. Click **■** to choose the drawing colour.
4. Adjust **Size** for brush/stroke thickness.
5. **Clear Frame** removes annotations from the current frame; **Clear All** clears every frame.

> Annotations are **reference guides only** — they are visible while editing but are **not included** in the exported video.

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
| Ctrl+K | Crop Frame… |
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

- Use **Onion skinning** while shooting: enable it before snapping the next frame so you can see how far to move your subject.
- Use **Crop** to remove the edges of the shooting area that show the table or backdrop.
- Use **Drawing** to sketch a rough path or position guide before shooting a frame — then clear it before export.
- If export fails, verify ffmpeg is installed: `ffmpeg -version` in a terminal.
- The FPS setting and crop rects are saved with the project.
