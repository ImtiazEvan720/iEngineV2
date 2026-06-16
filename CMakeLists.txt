cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(iEngineV2 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Do not update FetchContent dependencies during normal builds" FORCE)

set(IMGUI_GIT_TAG v1.91.9b CACHE STRING "Dear ImGui git tag, branch, or commit to fetch" FORCE)

set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Build Box2D documentation" FORCE)
set(BOX2D_BUILD_SAMPLES OFF CACHE BOOL "Build Box2D samples" FORCE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Build Box2D unit tests" FORCE)

set(IENGINE_TARGET_IOS OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IENGINE_TARGET_IOS ON)
endif()

set(IENGINE_TARGET_ANDROID OFF)
if(ANDROID)
    set(IENGINE_TARGET_ANDROID ON)
endif()

set(IENGINE_SDL_ONLY OFF)
if(EMSCRIPTEN OR IENGINE_TARGET_IOS OR IENGINE_TARGET_ANDROID)
    set(IENGINE_SDL_ONLY ON)
endif()

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${IMGUI_GIT_TAG}
    GIT_SHALLOW ON
)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
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

if(NOT EMSCRIPTEN)
    if(IENGINE_TARGET_ANDROID)
        set(SDL_SHARED ON CACHE BOOL "Build SDL shared library" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "Build SDL static library" FORCE)
    else()
        set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
        set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
    endif()
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
    set(SDL_AUDIO OFF CACHE BOOL "Build SDL audio subsystem" FORCE)
    set(SDL_CAMERA OFF CACHE BOOL "Build SDL camera subsystem" FORCE)
    set(SDL_DIALOG OFF CACHE BOOL "Build SDL dialog subsystem" FORCE)
    set(SDL_GPU OFF CACHE BOOL "Build SDL GPU subsystem" FORCE)
    set(SDL_HAPTIC OFF CACHE BOOL "Build SDL haptic subsystem" FORCE)
    set(SDL_HIDAPI OFF CACHE BOOL "Build SDL HIDAPI subsystem" FORCE)
    set(SDL_JOYSTICK OFF CACHE BOOL "Build SDL joystick subsystem" FORCE)
    set(SDL_POWER OFF CACHE BOOL "Build SDL power subsystem" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "Build SDL sensor subsystem" FORCE)
    set(SDL_TRAY OFF CACHE BOOL "Build SDL tray subsystem" FORCE)

    if(NOT IENGINE_SDL_ONLY)
        FetchContent_Declare(SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 3.1.0
            GIT_SHALLOW ON
            EXCLUDE_FROM_ALL
            SYSTEM
        )

        FetchContent_Declare(imgui-sfml
            GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
            GIT_TAG master
            GIT_SHALLOW ON
        )
    endif()

    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.10
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
    )
endif()

function(use_existing_fetchcontent_source dependency source_directory_name)
    set(existing_source_dir "${CMAKE_BINARY_DIR}/_deps/${source_directory_name}-src")
    if(EXISTS "${existing_source_dir}")
        string(TOUPPER "${dependency}" dependency_upper)
        set("FETCHCONTENT_SOURCE_DIR_${dependency_upper}" "${existing_source_dir}" CACHE PATH
            "Use already-populated ${dependency} source directory" FORCE
        )
    endif()
endfunction()

use_existing_fetchcontent_source(imgui imgui)
use_existing_fetchcontent_source(stb stb)
use_existing_fetchcontent_source(box2d box2d)
use_existing_fetchcontent_source(tinyxml2 tinyxml2)

if(NOT EMSCRIPTEN)
    if(NOT IENGINE_SDL_ONLY)
        use_existing_fetchcontent_source(SFML sfml)
        use_existing_fetchcontent_source(imgui-sfml imgui-sfml)
    endif()
    use_existing_fetchcontent_source(SDL3 sdl3)
endif()

FetchContent_MakeAvailable(imgui)
FetchContent_MakeAvailable(stb)

if(NOT EMSCRIPTEN)
    if(NOT IENGINE_SDL_ONLY)
        FetchContent_MakeAvailable(SFML)

        set(IMGUI_DIR ${imgui_SOURCE_DIR})
        set(IMGUI_SFML_FIND_SFML OFF)

        FetchContent_MakeAvailable(imgui-sfml)
    endif()
    FetchContent_MakeAvailable(SDL3)
endif()

FetchContent_MakeAvailable(box2d)
FetchContent_MakeAvailable(tinyxml2)

set(IENGINE_SOURCES
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    main.cpp
    src/Application.cpp
    src/Entity.cpp
    src/components/AnimationComponent.cpp
    src/components/CollisionComponent.cpp
    src/components/Component.cpp
    src/components/SpriteComponent.cpp
    src/components/TransformComponent.cpp
    src/components/PlayerController.cpp
    src/editor/LevelEditor.cpp
    src/game/Brick.cpp
    src/game/Bullet.cpp
    src/math/Vector2F.cpp
    src/misc/Asset.cpp
    src/misc/Animation.cpp
    src/misc/Level.cpp
    src/misc/LevelAsset.cpp
    src/misc/SdlTextureResource.cpp
    src/misc/Sprite.cpp
    src/misc/TextureAsset.cpp
    src/system/AssetManager.cpp
    src/system/InputListener.cpp
    src/system/InputSystem.cpp
    src/system/PhysicsSystem.cpp
    src/system/Renderer.cpp
    src/system/sdl/SdlGuiBackend.cpp
    src/system/sdl/SdlRenderBackend.cpp
    src/system/sdl/SdlWindowBackend.cpp
    src/system/TileLayerRenderer.cpp
)

if(NOT IENGINE_SDL_ONLY)
    list(APPEND IENGINE_SOURCES
        src/misc/MusicAsset.cpp
        src/misc/SfmlTextureResource.cpp
        src/misc/SoundAsset.cpp
        src/system/sfml/SfmlGuiBackend.cpp
        src/system/sfml/SfmlRenderBackend.cpp
        src/system/sfml/SfmlWindowBackend.cpp
    )
else()
    list(APPEND IENGINE_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
endif()

if(IENGINE_TARGET_ANDROID)
    add_library(iEngineV2 SHARED ${IENGINE_SOURCES})
    set_target_properties(iEngineV2 PROPERTIES OUTPUT_NAME main)
else()
    add_executable(iEngineV2 ${IENGINE_SOURCES})
endif()

target_compile_features(iEngineV2 PRIVATE cxx_std_17)

target_include_directories(iEngineV2 PRIVATE
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${stb_SOURCE_DIR}
)

target_link_libraries(iEngineV2 PRIVATE
    box2d::box2d
    tinyxml2::tinyxml2
)

if(EMSCRIPTEN)
    target_compile_definitions(iEngineV2 PRIVATE IENGINE_WEB IENGINE_SDL_ONLY)
    target_compile_options(iEngineV2 PRIVATE "-sUSE_SDL=3")
    target_link_options(iEngineV2 PRIVATE
        "-sUSE_SDL=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/Assets@/Assets"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/config.xml@/config.xml"
    )
    set_target_properties(iEngineV2 PROPERTIES SUFFIX ".html")
elseif(IENGINE_TARGET_IOS)
    target_compile_definitions(iEngineV2 PRIVATE IENGINE_IOS IENGINE_SDL_ONLY)
    target_link_libraries(iEngineV2 PRIVATE SDL3::SDL3)

    set_target_properties(iEngineV2 PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.imtiazevan.iEngineV2"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.imtiazevan.iEngineV2"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName "iEngineV2"
        XCODE_ATTRIBUTE_INFOPLIST_KEY_UILaunchScreen_Generation "YES"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    )

    add_custom_command(TARGET iEngineV2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xml"
            "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/config.xml"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/Assets"
    )
elseif(IENGINE_TARGET_ANDROID)
    target_compile_definitions(iEngineV2 PRIVATE IENGINE_ANDROID IENGINE_SDL_ONLY)
    target_link_libraries(iEngineV2 PRIVATE SDL3::SDL3)
else()
    target_link_libraries(iEngineV2 PRIVATE
        SFML::Graphics
        SFML::Audio
        ImGui-SFML::ImGui-SFML
        SDL3::SDL3
    )

    add_custom_command(TARGET iEngineV2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xml"
            "$<TARGET_FILE_DIR:iEngineV2>/config.xml"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:iEngineV2>/Assets"
    )
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT iEngineV2)
