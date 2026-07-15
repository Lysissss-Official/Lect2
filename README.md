> **NEO** is a playground branch.  
> Expect unfinished features, questionable code, and occasional bad ideas.  
> If it works, that's great. If it doesn't, that's expected.

<br>

<a name="readme-top"></a>

<p align="center">
  <img src="./assets/lect_dark.svg" width="180" align="middle">
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <img src="./assets/lect_logo_dark.svg" width="120" align="middle">
</p>

<h1 align="center">LectOS 2</h1>

<p align="center">
  An embedded application and user-interface framework for Calxis.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/LectOS-2.0-lightgrey?style=for-the-badge">
  &nbsp;
  <img src="https://img.shields.io/badge/ApsisUI-%E2%85%A1-lightgrey?style=for-the-badge">
  &nbsp;
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=for-the-badge&link=https%3A%2F%2Fen.cppreference.com%2Fw%2Fcpp%2F20">
  &nbsp;
  <img src="https://img.shields.io/badge/Desktop-MinGW--w64-informational?style=for-the-badge">
  &nbsp;
  <img src="https://img.shields.io/badge/Target-ESP32--S3-green?style=for-the-badge">
</p>

<p align="center">
  Built for Calxis, a programmable axis calculator based on the ESP32-S3.
  <br>
  LectOS provides application lifecycle management, a hierarchical UI model,
  request-driven rendering, focus navigation, and portable device backends.
</p>

---

## Project Status

LectOS 2 is currently in the middle of a major architectural rewrite.

The new architecture has already validated:

- Recursive `Page` / `Element` structures
- Arbitrarily nested UI containers
- Parent-relative logical coordinates
- Recursive physical-coordinate calculation
- Recursive clipping
- Independent focus maps for each container layer
- Entering and leaving nested focus layers
- Automatic scrolling inside nested containers
- Request-driven drawing through Theme and custom callbacks
- Platform-independent bitmap-font rendering
- Desktop execution through the EasyX-compatible backend

The following systems are still being developed:

- Animated scrolling and focus transitions
- Multiple physical displays
- Device-oriented display and input abstractions
- ESP-IDF compilation and flashing
- ESP32-S3 display and input drivers
- Dirty-region and partial-refresh rendering
- Application launcher and complete foreground arbitration
- pocketpy VM integration with the rewritten UI architecture

---

## Architecture

```text
┌──────────────────────────────────────────────────────────────┐
│                         Applications                         │
│                                                              │
│  lapp::Application                                           │
│  ├── APP_NATIVE       Native C++ applications                │
│  └── APP_VM           Script applications                    │
│                                                              │
│  app_setup() → app_main()                                    │
│  Services and devices are injected before application start. │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                          lcore                               │
│                                                              │
│  ThreadMgr       Application and service thread management   │
│  IDGenerator     Typed runtime object identifiers            │
│  Log             Platform-independent logging                │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                         ApsisUI II                           │
│                                                              │
│  lui::strc::Page                                             │
│  └── BasicItem                                               │
│      ├── Element                                             │
│      │   ├── Element                                         │
│      │   │   └── Element                                     │
│      │   └── Element                                         │
│      └── Element                                             │
│                                                              │
│  Each BasicItem may contain children.                        │
│  Elements may therefore act as both visual items and         │
│  nested layout or scrolling containers.                      │
└──────────────────────────────┬───────────────────────────────┘
                               │
                ┌──────────────┴──────────────┐
                ▼                             ▼
┌───────────────────────────────┐ ┌────────────────────────────┐
│          lui::Render          │ │       lui::Ctrller         │
│                               │ │                            │
│ RenderRequest queue           │ │ Input event translation    │
│ Time-based parameter updates  │ │ Focus navigation           │
│ Theme callback dispatch       │ │ Layer enter / leave        │
│ Custom-draw callback dispatch │ │ Input-device routing       │
│ Recursive redraw support      │ │                            │
└───────────────┬───────────────┘ └──────────────┬─────────────┘
                │                                │
                ▼                                ▼
┌──────────────────────────────────────────────────────────────┐
│                         ldevice                             │
│                                                              │
│  Display devices                 Input devices               │
│  ├── Desktop display             ├── Desktop keyboard        │
│  ├── ST7306 display              ├── Key matrix              │
│  └── Secondary display           ├── Rotary encoder          │
│                                  └── Touch / pointer          │
│                                                              │
│  A physical screen owns or binds its display backend.        │
│  InputDevice instances provide events to UICtrller.          │
└──────────────────────────────────────────────────────────────┘
````

The `ldevice` layer shown above is the target structure of the current
device-abstraction rewrite. Some device classes are still located under
the existing platform directories until migration is complete.

---

## UI Structure

ApsisUI II no longer uses the former fixed hierarchy:

```text
Element → Block → Page → Screen
```

The current structure is a recursive tree:

```text
Page
├── Element
│   ├── Element
│   ├── Element
│   │   └── Element
│   └── Element
└── Element
```

Every `BasicItem` contains:

* Logical coordinates
* Calculated physical coordinates
* Visual parameters
* Scroll offsets
* A parent pointer
* A list of child items

An `Element` may therefore represent:

* A button
* A text box
* A list entry
* A panel
* A custom-drawn component
* A focusable container
* A scrolling container
* A parent of another complete interface layer

### Coordinates

Logical coordinates are signed integer values.

`10000` represents one complete screen dimension during logical-to-physical
coordinate conversion, but logical coordinates are not percentages and are
not limited to the range `0–10000`.

For example:

```cpp
element.getParam(ParamIndex::logic_x1) = 500;
element.getParam(ParamIndex::logic_y1) = 12000;
element.getParam(ParamIndex::logic_x2) = 9500;
element.getParam(ParamIndex::logic_y2) = 13200;
```

Coordinates beyond `10000` are valid and are used for scrollable content.

Nested elements are positioned relative to their parent. Each parent may
apply its own `scroll_x` and `scroll_y` offsets before physical coordinates
are generated.

---

## Focus Navigation

Each container owns an independent focus layer.

`Page::rebuildFocusMap()` builds directional relationships between the
focusable direct children of every container.

Supported directions:

```text
right
up
left
down
up-right
up-left
down-left
down-right
```

Focus movement remains inside the current layer:

```cpp
page.moveFocus(lui::strc::DirecIndex::down);
```

Applications may explicitly enter or leave nested layers:

```cpp
page.enterFocusLayer();
page.leaveFocusLayer();
```

When focus moves to an item outside its parent's visible area,
`scrollToShow()` adjusts the appropriate parent scroll offset. The operation
is recursive, allowing nested containers to scroll independently while
remaining visible inside their ancestors.

---

## Rendering

Rendering is request-driven.

`Render` no longer contains a monolithic platform-specific drawing
implementation. Instead, a render request may provide a Theme callback or a
custom drawing callback.

```cpp
renderer.requestReDraw(
    &page,
    drawPage,
    user_data
);
```

A drawing callback receives a `DrawContext` containing:

```cpp
struct DrawContext {
    strc::BasicItem& target;
    DisplayBackend& display;
    ClipRect clip;
    float progress;
};
```

The exact device-facing type is currently being migrated from the former
`TranslatorService` abstraction into the `ldevice` display model.

### Render Requests

A render request may contain:

* Target structure item
* Target physical screen
* Optional animated parameter
* Start and end values
* Start and end times
* Animation function
* Theme or custom drawing function
* Additional callback data
* Refresh-policy information

Single-frame redraws and timed animations use the same queue.

```cpp
renderer.requestAnimate(
    screen,
    &page,
    ParamIndex::scroll_y,
    target_scroll,
    drawPage,
    user_data,
    start_time,
    std::chrono::milliseconds(180),
    ease_out
);
```

The renderer is designed to support:

* Multiple `Render` instances
* Multiple physical screens
* Independent pages on each screen
* Theme drawing
* Custom components
* Future dirty-region rendering
* Future partial display refresh

---

## Theme and Custom Drawing

ApsisUI II themes are collections of predefined drawing functions rather
than renderer subclasses.

```cpp
lui::theme::apsis::button(context, text_data);
lui::theme::apsis::textBox(context, text_data);
lui::theme::apsis::panel(context, nullptr);
```

Custom components use the same callback interface:

```cpp
void drawFormula(
    lui::Render::DrawContext& context,
    void* extra_data
);
```

This keeps UI structure independent from visual style and prevents platform
drawing code from entering `Element` or `Page`.

---

## Device Architecture

LectOS is moving hardware abstractions into the independent `ldevice`
namespace.

```text
ldevice
├── Screen
├── InputDevice
├── display
│   ├── DesktopScreen
│   ├── ST7306Screen
│   └── SecondaryScreen
└── input
    ├── DesktopKeyboard
    ├── KeyMatrix
    ├── RotaryEncoder
    └── TouchDevice
```

### Display Devices

A display device is responsible for:

* Physical dimensions
* Color mode
* Device initialization and shutdown
* Drawing commands or framebuffer access
* Display-specific refresh behavior
* The hardware driver associated with that screen

A render request identifies its target `Screen`. The renderer obtains the
appropriate display backend from that screen rather than receiving a
separate backend pointer.

This allows a single `Render` instance to process requests for multiple
physical displays while preserving support for multiple renderer instances.

### Input Devices

`UICtrller` is being changed from a platform-driver base class into an input
coordination service.

Platform-specific input implementations will become `InputDevice` objects:

```text
InputDevice
    ↓
UICtrller
    ↓
Page focus and application input
```

This allows one controller service to combine several devices, such as a
key matrix, rotary encoder, touch panel, and desktop keyboard.

---

## Components

| Component                | Description                                                                                                    |
| ------------------------ | -------------------------------------------------------------------------------------------------------------- |
| **LectOS 2 Runtime**     | Application lifecycle, service management, foreground control, and device injection                            |
| **ApsisUI II Structure** | Recursive `BasicItem` / `Element` / `Page` tree with nested coordinates, clipping, focus layers, and scrolling |
| **Render**               | Time-ordered request queue for redraws and parameter animations                                                |
| **ApsisUI II Theme**     | Platform-independent predefined drawing callbacks                                                              |
| **UICtrller**            | Focus navigation and input-event coordination                                                                  |
| **ldevice**              | Physical display and input-device definitions and drivers                                                      |
| **ThreadMgr**            | Application and service thread registration, startup, shutdown, and foreground arbitration                     |
| **IDGenerator**          | Typed runtime ID generation shared by UI, applications, services, and devices                                  |
| **Bitmap Fonts**         | Embedded platform-independent bitmap glyph rendering through the active display backend                        |
| **Native Applications**  | C++ applications compiled into the LectOS image                                                                |
| **VM Applications**      | Planned pocketpy-based applications loaded at runtime                                                          |

---

## Current Platforms

| Platform            |                  Status | Graphics / Device Backend                                                    |
| ------------------- | ----------------------: | ---------------------------------------------------------------------------- |
| **Linux**           |             Development | MinGW build executed through Wine with EasyX-compatible graphics             |
| **Windows**         |             Development | Native MinGW-w64 desktop build                                               |
| **ESP32-S3**        | Integration in progress | ESP-IDF, FreeRTOS, custom ST7306 driver                                      |
| **Calxis hardware** |                  Target | Main display, secondary display, key matrix, storage, and peripheral devices |

The desktop backend is primarily used to test the operating framework and
ApsisUI II without repeatedly flashing the target hardware.

---

## Repository Layout

```text
LectOS2/
├── apps/                       Application implementations
├── assets/                     README and project assets
├── include/
│   ├── lapp/                   Application model
│   ├── lcore/                  Runtime infrastructure
│   ├── ldevice/                Device abstractions and drivers
│   └── lui/
│       ├── base/               Structure, renderer, controller
│       ├── extension/          Fonts and UI extensions
│       ├── platform/           Transitional platform backends
│       └── theme/              ApsisUI II theme functions
├── src/                        Desktop entry point and sources
├── platform/
│   └── esp32s3/                ESP-IDF application entry point
├── third_party/                External dependencies
├── tools/                      Development tools
└── CMakeLists.txt              Desktop CMake entry point
```

The exact layout is still evolving as the existing platform backends are
moved into `ldevice`.

---

## Build

### Desktop MinGW Build

```bash
cmake \
  -S . \
  -B cmake-build-debug-mingw \
  -G "MinGW Makefiles"

cmake --build cmake-build-debug-mingw
```

The resulting Windows executable may be run natively on Windows or through
Wine on Linux.

### ESP32-S3

ESP-IDF support is being integrated as a separate CMake entry point.

The intended workflow is:

```bash
cd platform/esp32s3

idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

The desktop and ESP-IDF builds use separate build directories and platform
entry points while sharing the platform-independent LectOS, ApsisUI, and
application code.

---

## Development Demos

The current demos validate different parts of the rewritten architecture.

### Demo 2

Tests:

* New request-driven renderer
* Theme callbacks
* Focus navigation
* Long-page coordinates
* Automatic scrolling

### Demo 3

Tests:

* Recursive UI structures
* Nested containers
* Recursive clipping
* Independent focus layers
* `Enter` to enter a child layer
* `Backspace` to leave a child layer
* Nested container scrolling

---

## Screenshots

### Desktop development build

<p align="left">
  <img src="./assets/demo.png" width="360" align="middle">
  &nbsp;&nbsp;&nbsp;&nbsp;
  <img src="./assets/pycon.png" width="360" align="middle">
</p>

> Some screenshots may show an earlier LectOS 2 interface and will be updated
> as the rewritten renderer and device architecture are completed.

### Calxis hardware

<p align="left">
  Hardware integration is in progress.
</p>

---

## Related Calxis Projects

### SCalRR

High-precision calculation and expression-processing core.

![Repo ScalRR](https://lysissss-readme-stats.vercel.app/api/pin/?username=Lysissss-Official\&repo=ScalRR\&theme=transparent)

### Lect0

Former Calxis operating-system implementation.

![Repo Lect0](https://lysissss-readme-stats.vercel.app/api/pin/?username=Lysissss-Official\&repo=Lect0\&theme=transparent)

<p align="right">
  <img src="./assets/lect_dark.svg" width="72" align="middle">
  &nbsp;&nbsp;
  <img src="./assets/lect_logo_dark.svg" width="48" align="middle">
  &nbsp;&nbsp;
  <span> | LectOS 2 | ApsisUI Ⅱ | </span>
  <a href="#readme-top">Back to the top</a>
</p>

