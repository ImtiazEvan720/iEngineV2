# iEngineV2

## About The Project

iEngineV2 is an experimental C++ 2D game engine built around an entity component system architecture. It has backend-neutral rendering,Prefab Support, Lua scripting, Box2D physics and ImGui editor tooling.


## Feature List

- ECS-inspired `Entity` and `Component` architecture
- Entity names, tags, GUIDs, enabled state, destroyed state, parent/child hierarchy, and display order
- Backend abstraction for windowing, rendering, input, and ImGui integration
- SFML and SDL3 runtime backends
- Fixed internal resolution rendering with window scaling and letterboxing
- Box2D physics integration with static/dynamic bodies, sensors, raycasts, collision callbacks, and Lua collision events
- Raw input, virtual input, configurable input actions, and axis-based movement
- Lua scripting with entity/component bindings, script properties, prefab spawning, UI callbacks, and level loading APIs
- Custom runtime/editor file formats: `.ilevel`, `.iprefab`, `.ianim`, and `.itile`
- Component serializer registry for level and prefab save/load
- ImGui editor with level hierarchy, inspector, component drawers, sprite palette, animation editor, tileset creator, prefab panel, level manager, file explorer, and viewport gizmos
- Editor tools for transform, collider, and RectTransform editing
- UI canvas system with panels, labels, buttons, and edit text
- Desktop build support
- Experimental WebAssembly, iOS, and Android build support

## How To Build

Requirements:

- CMake 3.28 or newer
- C++17 compiler
- macOS, Windows, or Linux desktop toolchain
- Emscripten SDK for WebAssembly builds
- Xcode/iOS tools for iOS experiments
- Android SDK/NDK/CMake for Android experiments

Configure and build the normal desktop build:

```sh
cmake -S . -B build
cmake --build build
```

Run the normal desktop build:

```sh
./run.sh
```

Build the desktop release package:

```sh
./build_release.sh
```

Run the desktop release build:

```sh
./run_release.sh
```

Build WebAssembly:

```sh
./build_web.sh
```

Run WebAssembly locally:

```sh
./run_web.sh
```

Experimental platform scripts:

```sh
./build_ios.sh
./build_android.sh
```

## Future Roadmap

- Add editor undo/redo through command-based editing
- Improve project creation, project settings, and asset browser workflows
- Add collision layers, masks, and better physics debugging tools
- Improve runtime UI systems, navigation, focus handling, and mobile touch controls
- Improve sprite batching, render layers, camera tooling
- Add shader/material abstraction and optional 3D/OpenGL rendering experiments
- Stabilize WebAssembly, iOS, and Android packaging
- Add generic networking components such as network identity, authority, transform sync, and input commands
- Prototype host-authoritative multiplayer with client input, host simulation, snapshots, interpolation, and later prediction
