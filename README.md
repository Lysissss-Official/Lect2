> **NEO branch.**  
> Unfinished features, unstable APIs, and occasional bad ideas. Breaking changes are expected.

<br>

<a name="readme-top"></a>

<p align="center">
  <img src="./assets/lect_dark.svg" width="180" align="middle">
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <img src="./assets/lect_logo_dark.svg" width="120" align="middle">
</p>

<h1 align="center">Lect 2</h1>

<p align="center">
  <img src="https://img.shields.io/badge/Lect-2.0-lightgrey?style=for-the-badge">
  &nbsp;
  <img src="https://img.shields.io/badge/ApsisUI-%E2%85%A1-lightgrey?style=for-the-badge">
  &nbsp;
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=for-the-badge&link=https%3A%2F%2Fen.cppreference.com%2Fw%2Fcpp%2F20">
  &nbsp;
  <img src="https://img.shields.io/badge/Python-3.x-orange?style=for-the-badge&link=https%3A%2F%2Fpython.org">
  &nbsp;
  <img src="https://img.shields.io/badge/ESP32--S3-FreeRTOS-green?style=for-the-badge">
</p>

<h3 align="center">A small application runtime and UI framework for embedded systems</h3>

<p align="center">
  Built mainly for CalXis, an ESP32-S3-based axis calculator.
</p>

> [!IMPORTANT]
> Lect 2 is under active development.  
> APIs and architecture may change.

------------------------------------------------------------------------

## What is Lect 2

Lect 2 is the runtime framework for **CalXis**.  
Its UI layer includes a recursive UI structure, actions, rendering, and the **ChassisUI** theme.

It targets desktop and ESP32-S3, uses C++20, and keeps application code separated from platform-specific hardware code.

The main pieces are:

- `lcore` — application/service management, logging, paths, IDs, VM management
- `ldevice` — Resource-based hardware access
- `lui` — recursive UI structure, actions, rendering, themes
- `lapp` — application entry and management
- platform backends — desktop and ESP32-S3

Lect 2 does not try to hide the platform completely.  
Common operations have helpers, but lower-level interfaces remain available when needed.

------------------------------------------------------------------------

## Architecture

```text
┌──────────────────────────────────────────────────────────────┐
│                         Application                          │
│                                                              │
│                    lapp::Application                         │
│                                                              │
│   Native applications                                        │
│   VM applications                                            │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                           lcore                              │
│                                                              │
│   ThreadMgr     Application and service management           │
│   IDGenerator   Runtime object IDs                           │
│   Log           Logging                                      │
│   Path          Path abstraction                             │
│   VMMgr         Virtual-machine management                   │
└──────────────────────────────┬───────────────────────────────┘
                               │
               ┌───────────────┴─────────────────┐
               ▼                                 ▼
┌──────────────────────────────┐    ┌──────────────────────────┐
│         UI Framework         │    │         ldevice          │
│                              │    │                          │
│   UIStructure                │    │   Resource               │
│   UIAction                   │    │   ResourceMgr            │
│   UIRender                   │    │   Display devices        │
│   UITheme                    │    │   Input devices          │
│                              │    │   Storage                │
│   Recursive UI tree          │    │   Hardware resources     │
│   Focus navigation           │    │                          │
│   Scrolling                  │    │                          │
│   Render requests            │    │                          │
└──────────────┬───────────────┘    └────────────┬─────────────┘
               │                                 │
               └────────────────┬────────────────┘
                                ▼
                     ┌─────────────────────┐
                     │      Platform       │
                     │                     │
                     │   Linux / Desktop   │
                     │   ESP32-S3 / IDF    │
                     └─────────────────────┘
```

The dependency direction is intentionally small:

```text
lcore
  ↑
ldevice
  ↑
lapp / lui / platform
```

Lower layers do not depend on higher-level UI or application code.

------------------------------------------------------------------------

## UI Framework

The UI framework in Lect 2 uses a recursive tree instead of a fixed `Element → Block → Page → Screen` hierarchy.

```text
Page
├── Element
│   ├── Element
│   └── Element
│       └── Element
└── Element
```

Every `BasicItem` may contain child items.

This means an Element can act as:

- a visual item
- a container
- a scrolling container
- a focusable item
- a parent of another UI layer
- a custom-drawn component

### UI layers

```text
UIStructure
    Raw structure and state

UIAction
    Common UI operations

UIRender
    Render requests and animation

UITheme
    Structure → drawing commands
```

Applications can bypass helpers and work with lower-level structures when necessary.

### Coordinates

The UI framework uses signed logical coordinates.

```text
logical space
    ↓
recursive parent-relative coordinates
    ↓
physical coordinates
```

Children are positioned relative to their parent.  
Scroll offsets are applied during coordinate calculation.

Logical coordinates are not limited to `0..10000`.  
Values outside the visible screen are valid and are used for scrollable content.

### Focus

Focus relationships are calculated from the direct focusable children of a container.

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

Focus movement and scrolling are separate operations.  
`UIAction` provides common movement helpers.  
The underlying `Page` remains responsible for focus state and focus relationships.

Nested focus layers are supported.

------------------------------------------------------------------------

## Rendering

The UI framework uses request-driven rendering.

A render request may describe:

- a target structure item
- a parameter to animate
- start and end values
- start and end times
- a timing function
- a theme drawing operation
- a custom drawing callback

Redraws and animations use the same render queue.

```text
Application
     │
     │ request
     ▼
Render
     │
     ├── redraw
     ├── parameter animation
     └── custom drawing
     │
     ▼
Theme / display backend
```

The renderer uses centralized scheduling rather than giving every UI operation its own rendering thread.

Current work focuses on predictable queue ownership, animation generation, clipping, and scheduling.

------------------------------------------------------------------------

## Hardware Resources

Lect 2 is moving toward a Resource-based hardware model.

A Resource is a small black-box description of a hardware or system resource.

```text
Resource
├── ID
├── name
├── type
├── subtype
├── dependencies
└── information
```

Resources are not hardware driver classes.  
They describe what exists and how resources are connected.

Drivers provide the implementation.

```text
Resource
    │
    │ description
    ▼
Driver matching
    │
    ▼
Driver
    │
    ▼
Hardware
```

### Resource graph

Resources may depend on other Resources.

```text
gpio10
   ↑
   │
spi0
   ↑
   │
display0
```

Multiple Resources may reference the same Resource.

```text
             ┌── display0
             │
spi0 ────────┼── flash0
             │
             └── sensor0
```

A dependency represents a reference, not exclusive ownership.

This also supports naturally recursive hardware structures such as USB.

```text
usb0
└── hub0
    ├── keyboard0
    ├── mouse0
    └── hub1
        ├── device0
        └── device1
```

Resource initialization will eventually resolve the dependency graph, detect invalid dependencies and cycles, match drivers, and initialize resources in dependency order.

------------------------------------------------------------------------

## Hardware Configuration

Hardware configuration is intended to be declarative.

The planned configuration flow is:

```text
board.toml
    │
    ▼
Hardware configuration parser
    │
    ├── schema validation
    ├── dependency validation
    ├── cycle detection
    └── resource graph construction
    │
    ▼
generated HardwareConfig.h / .cpp
    │
    ▼
firmware
```

The TOML file is a build-time description.  
The firmware does not need to carry a TOML parser.

A Resource reference uses its resource name.

```toml
[resource.spi0]
type = "io_controller"
subtype = "spi"

miso = "gpio10"
mosi = "gpio11"
sclk = "gpio12"
speed = 10000000
```

Optional references are marked with `?`.

```toml
[resource.display0]
type = "device"
subtype = "display"

spi = "spi0"
te = "?gpio9"
res = "?gpio14"
```

The hardware schema is still being finalized.

------------------------------------------------------------------------

## Example

A small UI application can build a recursive structure directly.

```cpp
lui::strc::Page page;

auto* panel =
    page.createChild<lui::strc::Element>(
        lui::strc::ItemStyle {
            .lx1 = 500,
            .ly1 = 500,
            .lx2 = 9500,
            .ly2 = 9500
        }
    );

auto* button =
    panel->createChild<lui::strc::Element>(
        lui::strc::ItemStyle {
            .lx1 = 500,
            .ly1 = 500,
            .lx2 = 3000,
            .ly2 = 1500,
            .item_cfg = 0x01
        }
    );

page.rebuildFocusMap();
page.setFocus(button);
```

Common operations can then be performed through `UIAction`.

```cpp
lui::action::moveNextFocus(
    &page,
    lui::strc::DirecIndex::right,
    renderer
);
```

The API shown here is not final.

------------------------------------------------------------------------

## Current Components

```text
include/
├── lcore/
│   ├── IDGenerator.h
│   ├── Log.h
│   ├── Path.h
│   ├── ThreadMgr.h
│   └── VMMgr.h
│
├── lapp/
│   └── Application.h
│
├── ldevice/
│   ├── Resource.h
│   ├── ResourceMgr.h
│   ├── input/
│   ├── screen/
│   └── storage/
│
├── lui/
│   ├── base/
│   │   ├── UIAction.h
│   │   ├── UICtrller.h
│   │   ├── UIRender.h
│   │   ├── UIStructure.h
│   │   └── UITheme.h
│   │
│   ├── extension/
│   │   ├── font/
│   │   └── image/
│   │
│   ├── platform/
│   └── theme/
│
└── lmath/
```

The repository also contains desktop and ESP32-S3 platform code, example applications, assets, and third-party dependencies.

------------------------------------------------------------------------

## Platforms

### Desktop

The current desktop environment uses an EasyX-compatible backend.

It is mainly used for UI development and testing.

### ESP32-S3

ESP32-S3 is the current embedded target.

The platform directory contains the ESP-IDF integration and target-specific components.

The hardware abstraction is being separated from the platform implementation so application and UI code do not need to depend directly on ESP-IDF.

------------------------------------------------------------------------

## Build

The project currently uses CMake and C++20.

```bash
cmake -S . -B build
cmake --build build
```

The desktop build currently depends on the bundled EasyX-compatible backend.

The ESP32-S3 target is maintained separately under:

```text
platform/esp32s3/
```

------------------------------------------------------------------------

## Development Status

Lect 2 is currently a work in progress.

### Working

- Recursive UI structure
- Nested Elements
- Parent-relative coordinates
- Dirty-region and partial-refresh rendering
- Recursive coordinate calculation
- Nested scrolling
- Focus maps
- Nested focus layers
- Focus movement helpers
- Request-driven rendering
- Render animations
- Render request generation handling
- Recursive clipping
- Bitmap font support
- Desktop display backend
- Basic Resource abstraction
- Basic Resource management

### In development

- Hardware Resource schema
- Resource dependency graph
- Hardware configuration parser
- Driver registry and matching
- Automatic hardware initialization
- Input resource model
- Multiple physical display support
- More display and input drivers
- Storage abstraction
- VM integration with the new architecture
- Rendering acceleration algorithm based on segment trees

### Planned

- Hardware configuration generation
- Runtime resource inspection
- Resource / driver CLI
- More platform backends
- More complete application management

------------------------------------------------------------------------

## Repository Structure

```text
.
├── apps/           Example applications
├── assets/         Project assets
├── include/        Framework headers and implementations
├── platform/       Platform-specific entry points
├── src/            Desktop entry points and compatibility code
├── third_party/    Third-party libraries
└── tools/          Build-time development tools
```

------------------------------------------------------------------------

## Related Projects

Lect 2 is developed together with the CalXis ecosystem.

- **CalXis** — the programmable calculator project
- **SCalRR** — the mathematical backend of Lect 2
- **Lect0** — the previous-generation Lect implementation

------------------------------------------------------------------------

## Notes

Lect 2 is a personal R&D project.

The aim is a small architecture that stays understandable and flexible enough for embedded applications.

The code is experimental, and the architecture will keep changing while the remaining systems are built.

<p align="right">
  <img src="./assets/lect_dark.svg" width="64" alt="Lect">
  <img src="./assets/lect_logo_dark.svg" width="42" alt="Lect logo">
  <a href="#readme-top">Back to top</a>
</p>
