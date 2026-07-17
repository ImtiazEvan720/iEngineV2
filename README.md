# iEngineV2

iEngineV2 is an experimental C++ game engine built around an entity/component architecture, backend-neutral rendering, Lua scripting, Box2D physics, custom assets, editor tooling, and multi-platform build experiments.

The project is currently focused on building a practical 2D engine foundation with runtime/editor workflows, scriptable gameplay, UI, prefabs, custom level serialization, and future multiplayer support.

## Architecture Overview

```text
Application
  -> Window Backend
  -> Render Backend
  -> AssetManager
  -> Level
  -> Entity / Component system
  -> PhysicsSystem
  -> ScriptSystem
  -> Input systems
  -> Editor UI
```

The engine separates platform/window handling, rendering, input, physics, scripting, asset loading, and editor features into dedicated systems.

## ECS Model

iEngineV2 uses an ECS-inspired entity/component model.

### Entity

An entity is a runtime object that owns a list of components.

Entities support:

- name
- tag
- guid
- enabled state
- destroyed state
- parent/child hierarchy
- display order
- component storage

Entities are intentionally lightweight. Behavior is added through components and Lua scripts.

### Component

A component adds data or behavior to an entity.

Current component examples:

- `TransformComponent`
- `RectTransformComponent`
- `SpriteComponent`
- `AnimationComponent`
- `CollisionComponent`
- `ScriptComponent`
- `CanvasComponent`
- `UILabelComponent`
- `UIButtonComponent`
- `UIEditTextComponent`
- `UIPanelComponent`
- `PlayerCameraComponent`

Components can implement lifecycle methods:

```cpp
onStart()
onUpdate(float deltaTime)
onEnable(bool value)
onDestroy()
```

## Major Systems

### Rendering

Rendering is abstracted behind backend interfaces so the engine can support multiple rendering/platform implementations.

Current rendering features include:

- SFML backend
- SDL3 backend
- backend-neutral drawing API
- sprite rendering
- animation rendering
- UI rendering
- camera component support
- render scale support
- debug drawing
- editor viewport support

### Input

Input is split into raw input, virtual input, and configurable input actions.

```text
SDL/SFML events
  -> RawInputSystem
  -> VirtualInputSystem
  -> Lua / gameplay systems
```

`RawInputSystem` stores keyboard, mouse, touch, and text input from the active window backend.

`VirtualInputSystem` maps raw input into engine actions such as:

- `Move`
- `Fire`
- `Fire2`
- `Pause`

Input action definitions are loaded from:

```text
Assets/Input/actions.inputactions.xml
```

Input bindings are loaded from:

```text
Assets/Input/default.input.xml
```

Movement now uses an axis action:

```xml
<binding action="Move" key="W" axisX="0" axisY="-1"/>
```

Lua can read virtual input through:

```lua
local move = Input.getAxis2D("Move")
```

### Physics

Physics is handled through Box2D.

Current physics features include:

- collision components
- dynamic/static body types
- sensor support
- collision callbacks
- sensor callbacks
- Lua collision events
- raycast support
- linear velocity control
- fixed rotation support
- collider gizmo editing

### Lua Scripting

Lua scripts can drive runtime behavior.

Script support includes:

- entity access
- component access
- transform access
- collision access
- animation access
- script properties
- entity references by guid
- prefab spawning
- Lua event calls
- UI button callbacks
- edit text submit callbacks

### Assets

The asset system uses polymorphic asset classes and an asset manager.

Supported or planned asset categories include:

- textures
- sounds
- music
- scripts
- prefabs
- levels
- animations
- tilesets
- input action definitions
- input bindings

### Levels And Prefabs

The engine uses custom serialized formats:

```text
.ilevel   level files
.iprefab  prefab files
.ianim    animation files
.itile    tileset metadata files
```

Levels and prefabs are serialized through component serializers. This keeps save/load behavior closer to component ownership and reduces hardcoded level serialization logic.

### Editor

The editor is built with ImGui.

Current editor features include:

- top toolbar/menu
- entity hierarchy
- entity inspector
- component drawers
- add/remove component workflow
- prefab panel
- sprite palette
- animation editor
- tileset creator
- file explorer
- level manager
- transform gizmo
- collider gizmo
- rect transform gizmo
- camera preview
- play/pause/stop controls
- level save/load
- prefab save/load

## Current Features

- ECS-inspired entity/component architecture
- Lua scripting with entity/component bindings
- SFML and SDL3 backend support
- Box2D physics integration
- Custom asset manager
- Custom level format
- Custom prefab format
- Custom animation format
- Runtime input action registry
- Axis-based movement input
- UI canvas, panels, labels, buttons, and edit text
- Editor component drawers
- Editor gizmos
- Camera component support
- Desktop build support
- Experimental WebAssembly build support
- Experimental iOS build support
- Experimental Android build support

## Build

Configure and build desktop:

```sh
cmake -S . -B build
cmake --build build
```

Run desktop:

```sh
./run.sh
```

Build WebAssembly:

```sh
./build_web.sh
```

Run WebAssembly locally:

```sh
./run_web.sh
```

## Roadmap / TODO

### Runtime Framework

- Add generic game state management
- Add scene/level transition system
- Add reusable spawn/despawn system
- Add generic health/damage component support
- Add collision filtering with layers and masks
- Add timer/cooldown utilities
- Add reusable finite state machine support
- Add behavior tree or steering behavior support
- Add event bus or message system between entities/components
- Add save/load support for runtime game state
- Add deterministic fixed update loop for simulation-heavy systems

### Editor

- Add input action editor
- Add collision layer/mask editor
- Improve prefab editing workflow
- Improve UI component editing
- Add project settings panel
- Add undo/redo
- Add scene validation tools
- Add better asset browser workflows
- Add editor layout persistence

### Rendering

- Improve sprite batching
- Add explicit render layers
- Improve animation runtime/preview consistency
- Improve camera preview tools
- Add render-to-texture workflows
- Add optional OpenGL/3D rendering path
- Add shader/material abstraction

### Assets And Serialization

- Expand component serializer coverage
- Add asset dependency validation
- Add missing asset reporting
- Add asset import pipeline
- Add prefab variant support
- Add project packaging/export pipeline

### Multiplayer

- Add `NetworkIdentityComponent`
- Add `NetworkAuthorityComponent`
- Add `NetworkTransformComponent`
- Add `InputComponent`
- Add generic `InputCommand`
- Add fixed network tick
- Add host/client session flow
- Send client input commands to host
- Host simulates authoritative physics/gameplay
- Host broadcasts world snapshots
- Client applies snapshots
- Add interpolation
- Add client-side prediction
- Investigate rollback after host-authoritative networking works

### Platform Support

- Stabilize WebAssembly build
- Improve Android packaging
- Improve iOS packaging
- Add mobile touch UI workflows
- Add platform-specific input bindings
- Add platform-specific build validation

## Project Status

iEngineV2 is experimental and under active development. APIs, file formats, runtime systems, and editor workflows are expected to change as the engine evolves.
