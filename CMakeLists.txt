cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(iEngineV2 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

set(IMGUI_GIT_TAG v1.91.9b CACHE STRING "Dear ImGui git tag, branch, or commit to fetch" FORCE)

set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Build Box2D documentation" FORCE)
set(BOX2D_BUILD_SAMPLES OFF CACHE BOOL "Build Box2D samples" FORCE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Build Box2D unit tests" FORCE)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${IMGUI_GIT_TAG}
    GIT_SHALLOW ON
)

FetchContent_Declare(imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG master
    GIT_SHALLOW ON
)

FetchContent_Declare(box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.1.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 11.0.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(SFML)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF)

FetchContent_MakeAvailable(imgui-sfml)
FetchContent_MakeAvailable(box2d)
FetchContent_MakeAvailable(tinyxml2)

add_executable(iEngineV2
    main.cpp
    src/Entity.cpp
    src/components/AnimationComponent.cpp
    src/components/CollisionComponent.cpp
    src/components/Component.cpp
    src/components/SpriteComponent.cpp
    src/components/TransformComponent.cpp
    src/components/PlayerController.cpp
    src/game/Brick.cpp
    src/game/Bullet.cpp
    src/math/Vector2F.cpp
    src/misc/Asset.cpp
    src/misc/Animation.cpp
    src/misc/Level.cpp
    src/misc/LevelAsset.cpp
    src/misc/MusicAsset.cpp
    src/misc/SoundAsset.cpp
    src/misc/Sprite.cpp
    src/misc/TextureAsset.cpp
    src/system/AssetManager.cpp
    src/system/InputListener.cpp
    src/system/InputSystem.cpp
    src/system/PhysicsSystem.cpp
    src/system/Renderer.cpp
    src/system/sfml/SfmlGuiBackend.cpp
    src/system/sfml/SfmlRenderBackend.cpp
    src/system/sfml/SfmlWindowBackend.cpp
    src/system/TileLayerRenderer.cpp
)

target_compile_features(iEngineV2 PRIVATE cxx_std_17)

target_include_directories(iEngineV2 PRIVATE src)

target_link_libraries(iEngineV2 PRIVATE
    SFML::Graphics
    SFML::Audio
    ImGui-SFML::ImGui-SFML
    box2d::box2d
    tinyxml2::tinyxml2
)

add_custom_command(TARGET iEngineV2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.xml"
        "$<TARGET_FILE_DIR:iEngineV2>/config.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:iEngineV2>/Assets"
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT iEngineV2)
