# VAR Compositor

## What this project does

This repository contains two Windows desktop applications used for a VAR (Video Assistant Referee) broadcast workflow:

1. **VAR Compositor** (`VAR Compositor` project)
   - Builds and runs a GStreamer video pipeline.
   - Mixes multiple DeckLink video inputs with a background video.
   - Draws on-screen overlay text (for example: `GOAL`, `PENALTY`, `OFFSIDE`).
   - Shows a local preview window.
   - Sends final output to a DeckLink output device.
   - Exposes a TCP command server on port `5000`.

2. **VAR Compositor Control Application** (`VAR_Compositor_Control_Application` project)
   - Qt-based remote control panel.
   - Connects to the compositor via TCP.
   - Sends commands to switch modes and update/clear overlay text.
   - Supports reusable custom text presets.

## How it works

### Core compositor behavior

- On startup, `VAR Compositor` initializes GStreamer and builds a pipeline in **VAR mode**.
- It uses hardcoded template files:
  - `C:/Temp/VAR.mp4`
  - `C:/Temp/VAR_BUG.mp4`
- Both template files must be **`.mp4`** files.
- The UI has mode buttons (**VAR** / **Program**) and text controls.
- A TCP server listens on `0.0.0.0:5000` for remote commands.
- Switching modes rebuilds the active pipeline layout.

### Supported DeckLink inputs (1080i)

The pipeline expects **1080i50** DeckLink sources:

- **VAR mode inputs**
  - `decklinkvideosrc device-number=1 mode=1080i50`
  - `decklinkvideosrc device-number=2 mode=1080i50`
  - `decklinkvideosrc device-number=3 mode=1080i50`

- **Program mode input**
  - `decklinkvideosrc device-number=5 mode=1080i50`

- **Output**
  - `decklinkvideosink device-number=4 mode=1080i50`

### Remote commands

Supported command examples:

- `MODE VAR`
- `MODE PROGRAM`
- `TEXT GOAL CHECK`
- `TEXT` (clear overlay)
- `KILL` / `EXIT` / `QUIT`

The control app sends these commands over TCP and shows status feedback in its UI.

## Required tools and dependencies (build)

You need the following installed on **Windows x64**:

1. **Visual Studio 2022** with **Desktop development with C++**
2. **MSVC toolset matching project settings** (`v145` in the `.vcxproj` files)
3. **Windows 10 SDK**
4. **Qt 6 (MSVC 2022 x64)**
   - The control project is configured for `Qt 6.11.1 msvc2022_64`
5. **Qt Visual Studio Tools / QtMsBuild integration**
6. **GStreamer 1.0 MSVC x64 development package**
7. **Blackmagic DeckLink SDK + GStreamer DeckLink plugins** (runtime + development)

## Required environment/project variables

Before building `VAR Compositor`, ensure these are set:

- `GSTREAMER_1_0_ROOT_MSVC_X86_64` -> GStreamer install root
- `QTDIR` -> Qt install root (MSVC x64 build)

For the control app, verify `QtMsBuild` and the selected Qt installation are available in Visual Studio.

## Build steps

1. Open the solution/projects in Visual Studio.
2. Select configuration:
   - `x64`
   - `Debug` or `Release`
3. Build both projects:
   - `VAR Compositor`
   - `VAR_Compositor_Control_Application`

## Run steps

1. Start **VAR Compositor** first (it hosts the TCP command server on port `5000`).
2. Start **VAR_Compositor_Control_Application**.
3. (Optional) Set `VAR_CONTROL_IP` environment variable if the control app connects to a different host.

## Notes

- This project expects DeckLink capture/output hardware and matching device indices configured in pipeline strings.
- Required template files are `C:/Temp/VAR.mp4` and `C:/Temp/VAR_BUG.mp4` (both must be `.mp4`).
