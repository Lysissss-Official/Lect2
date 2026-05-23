<p align="center">
  <img src="./assets/lect_dark.svg" width="180" align="middle">
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <img src="./assets/lect_logo_dark.svg" width="120" align="middle">
</p>

<h1 align="center">LectOS 2 | 面向 CalXis 的矩形派生操作框架</h1>

<p align="center">
  <img src="https://img.shields.io/badge/LectOS-2.0-lightgrey?style=for-the-badge">
  &nbsp;
  <img src="https://img.shields.io/badge/ApsisUI-%E2%85%A1-lightgrey?style=for-the-badge">
  &nbsp;
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=for-the-badge&link=https%3A%2F%2Fen.cppreference.com%2Fw%2Fcpp%2F20">
  &nbsp;
  <img src="https://img.shields.io/badge/Python-3.x-orange?style=for-the-badge&link=https%3A%2F%2Fpython.org">
  &nbsp;
  <img src="https://img.shields.io/badge/ESP32--S3-FreeRTOS-green?style=for-the-badge">
</p>

---

<p align="center">
  面向 CalXis（基于 ESP32-S3 的轴计算器）的矩形派生操作框架。
  <br>
  围绕机械轴计算、二元状态和后工业视觉语言构建。
</p>

## 架构

```
┌──────────────────────────────────────────────────────┐
│                   lui::app::Application              │
│  ┌───────────────────┐  ┌──────────────────────────┐ │
│  │    APP_NATIVE     │  │        APP_VM            │ │
│  │  (C++ 二进制文件)   │  │  (pocketpy Python 热解释) │ │
│  └────────┬──────────┘  └─────────────┬────────────┘ │
│           └──────────┬────────────────┘              │
│                      ▼                               │
│              core::ThreadMgr                         │
│       (每应用独立 std::thread，前后台仲裁)               │
├──────────────────────────────────────────────────────┤
│                 lui::scr::Screen                     │
│               (显示目标：宽×高 色彩模式)                 │
├──────────────────────────────────────────────────────┤
│                 lui::pge::Page                       │
│    ┌───────────────────────────────────────────┐     │
│    │  scroll_y / max_scroll / viewport_h       │     │
│    │  焦点导航 (↑↓←→ 几何关系图)                  │     │
│    │  PAGE_FULL <-> PAGE_HALF 页高切换          │     │
│    └───────────────────────────────────────────┘     │
│  ┌───────────────────┐  ┌────────────────────────┐   │
│  │   lui::blk::Block │  │  lui::blk::Block       │   │
│  │  ┌──────┐┌──────┐ │  │  ┌──────┐┌──────┐      │   │
│  │  │ Ele  ││ Ele  │ │  │  │ Ele  ││ Ele  │ ...  │   │
│  │  └──────┘└──────┘ │  │  └──────┘└──────┘      │   │
│  └───────────────────┘  └────────────────────────┘   │
│         lui::ele::Element (ELE_TEXTBOX / BUTTON      │
│                            / LIST / EMPTY)           │
├──────────────────────────────────────────────────────┤
│                    服务层                             │
│  ┌────────────┐ ┌──────────┐ ┌───────────────────┐   │
│  │  Render    │ │ Ctrller  │ │  Translator       │   │
│  │ (双队列     │ │(键盘矩阵   │ │  (drawPixelCmd,   │   │
│  │  渲染管线   │ │ + 模拟    │ │   drawFontCmd,    │   │
│  │  + 焦点     │ │  开关)    │ │   drawLineCmd,   │   │
│  │  动画)      │ │          │ │   ClearDeviceCmd) │   │
│  └─────┬──────┘ └─────┬────┘ └─────────┬─────────┘   │
│        └──────────────┼────────────────┘             │
│                       ▼                              │
│                 平台后端                               │
│     ┌──────────┐  ┌──────────┐  ┌──────────────┐     │
│     │ Linux 7  │  │ Windows  │  │  ESP32-S3    │     │
│     │(Wine+    │  │(EasyX)   │  │(FreeRTOS+    │     │
│     │ EasyX)   │  │          │  │ ST7306 LCD)  │     │
│     └──────────┘  └──────────┘  └──────────────┘     │
└──────────────────────────────────────────────────────┘
```

## 组件

| 组件 | 说明 |
|-----------|-------------|
| **LectOS 2 运行时** | 核心操作系统框架，管理应用生命周期、前后台仲裁和服务注入 |
| **ApsisUI II 主题** | `lui::render::UIRenderApsisUI2` — 分层渲染管线（背景 → 状态栏 → 区块 → 焦点动画 → 滚动条），双队列请求系统，easeOut 焦点过渡动画 |
| **线程管理器** | `core::ThreadMgr` — 每应用独立 `std::thread`，支持注册/启动/停止/挂起/恢复，通过 `allow_input` / `allow_render` 标志实现前后台仲裁 |
| **原生 / VM 应用** | `APP_NATIVE`（C++ 编译进二进制）与 `APP_VM`（通过 pocketpy 单头解释器运行 Python 3.x） |
| **LUI 元素层级** | 五层架构：`Element` → `Block` → `Page` → `Screen` → `Application`，逻辑坐标（% 0–100）到物理坐标（px）映射 |
| **焦点导航** | 基于几何关系的四向焦点图，由 `Page::recalculateFocus()` 自动计算 — O(n²) 最近邻算法，含水平/垂直重叠启发式判定 |
| **位图字体扩展** | `lui::ext::font::FontBase` — 基于 Transor 的逐像素位图字形渲染（SarasaMonoSC 20px，607 字符），不依赖平台驱动 |
| **跨平台后端** | `lui::transor::TranslatorService` 抽象接口，平台实现包括 Linux（Wine+EasyX）、Windows（EasyX）、ESP32-S3（FreeRTOS+ST7306） |

## 平台

| 平台 | 图形后端 | 备注 |
|----------|-------|-------|
| **Linux** | EasyX via Wine | 主要开发主机；`linux7` 后端 |
| **Windows** | EasyX 原生 | 从 Linux 通过 MinGW-w64 交叉编译 |
| **ESP32-S3** | ST7306 LCD + FreeRTOS | 目标硬件；16 位色深 |

## 构建

```bash
# 配置（从 Linux 交叉编译 MinGW）
cmake -S . -B cmake-build-debug-mingw/ -G "MinGW Makefiles"

# 编译
cmake --build cmake-build-debug-mingw/
```

> 顺便一提，我创造 CalXis 和 LectOS 完全是因为 CASIO fx-991 计算器太难用了（笑

```
cmake --target ApsisUI2
[0%] Building target (ETA: 2000 yrs)
```

## CalXis 仓库

- 计算核心  
  ![Repo ScalRR](https://lysissss-readme-stats.vercel.app/api/pin/?username=Lysissss-Official&repo=ScalRR&theme=transparent)
- 前代 OS 版本  
  ![Repo Lect0](https://lysissss-readme-stats.vercel.app/api/pin/?username=Lysissss-Official&repo=Lect0&theme=transparent)
