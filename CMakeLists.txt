cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(iEngineV2 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Do not update FetchContent dependencies during normal builds" FORCE)

set(IMGUI_GIT_TAG v1.91.9b CACHE STRING "Dear ImGui git tag, branch, or commit to fetch" FORCE)
set(LUA_VERSION 5.4.8 CACHE STRING "Lua version to embed" FORCE)
set(SOL2_GIT_TAG v3.5.0 CACHE STRING "sol2 git tag, branch, or commit to fetch" FORCE)

set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Build Box2D documentation" FORCE)
set(BOX2D_BUILD_SAMPLES OFF CACHE BOOL "Build Box2D samples" FORCE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Build Box2D unit tests" FORCE)
set(SOL2_ENABLE_INSTALL OFF CACHE BOOL "Disable sol2 install rules" FORCE)
set(SOL2_BUILD_LUA OFF CACHE BOOL "Use engine-provided Lua instead of sol2-built Lua" FORCE)
set(SOL2_LUA_VERSION ${LUA_VERSION} CACHE STRING "Lua version for sol2 integration" FORCE)

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

set(IENGINE_WITH_EDITOR_DEFAULT ON)
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR EMSCRIPTEN OR IENGINE_TARGET_IOS OR IENGINE_TARGET_ANDROID)
    set(IENGINE_WITH_EDITOR_DEFAULT OFF)
endif()
option(IENGINE_WITH_EDITOR "Build iEngine editor UI and tooling" ${IENGINE_WITH_EDITOR_DEFAULT})

set(IENGINE_EMBED_LUA_SCRIPTS_DEFAULT OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(IENGINE_EMBED_LUA_SCRIPTS_DEFAULT ON)
endif()
option(IENGINE_EMBED_LUA_SCRIPTS "Embed Assets/Scripts Lua files into the executable" ${IENGINE_EMBED_LUA_SCRIPTS_DEFAULT})

if(IENGINE_SDL_ONLY)
    set(IENGINE_IMGUI_USER_CONFIG "\"IEngineImGuiConfig.h\"")
else()
    set(IENGINE_IMGUI_USER_CONFIG "\"IEngineImGuiSfmlConfig.h\"")
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

FetchContent_Declare(lua
    URL https://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz
    URL_HASH SHA256=4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG ${SOL2_GIT_TAG}
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
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

        set(IMGUI_SFML_USE_DEFAULT_CONFIG OFF CACHE BOOL "Use default imconfig-SFML.h" FORCE)
        set(IMGUI_SFML_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "Path to ImGui-SFML user ImGui config" FORCE)
        set(IMGUI_SFML_CONFIG_NAME "IEngineImGuiSfmlConfig.h" CACHE STRING "ImGui-SFML user ImGui config header" FORCE)
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
use_existing_fetchcontent_source(lua lua)
use_existing_fetchcontent_source(sol2 sol2)
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
FetchContent_MakeAvailable(lua)
FetchContent_MakeAvailable(sol2)

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

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM LUA_SOURCES
    "${lua_SOURCE_DIR}/src/lua.c"
    "${lua_SOURCE_DIR}/src/luac.c"
)

add_library(iEngineLua STATIC ${LUA_SOURCES})
target_include_directories(iEngineLua PUBLIC "${lua_SOURCE_DIR}/src")

if(IENGINE_TARGET_IOS)
    target_compile_definitions(iEngineLua PRIVATE LUA_USE_IOS)
endif()

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
    src/components/ScriptComponent.cpp
    src/components/SpriteComponent.cpp
    src/components/TransformComponent.cpp
    src/components/PlayerController.cpp
    src/math/Vector2F.cpp
    src/misc/Asset.cpp
    src/misc/Animation.cpp
    src/misc/AnimationLoader.cpp
    src/misc/Camera2D.cpp
    src/misc/Guid.cpp
    src/misc/Level.cpp
    src/misc/LevelAsset.cpp
    src/misc/LevelManager.cpp
    src/misc/PrefabAsset.cpp
    src/misc/SdlTextureResource.cpp
    src/misc/Sprite.cpp
    src/serialization/ComponentSerializationHelpers.cpp
    src/serialization/ComponentSerializer.cpp
    src/serialization/ComponentSerializerRegistry.cpp
    src/serialization/components/AnimationComponentSerializer.cpp
    src/serialization/components/CollisionComponentSerializer.cpp
    src/serialization/components/GameComponentSerializers.cpp
    src/serialization/components/ScriptComponentSerializer.cpp
    src/serialization/components/SpriteComponentSerializer.cpp
    src/serialization/components/TransformComponentSerializer.cpp
    src/serialization/PrefabSerializer.cpp
    src/misc/TextureAsset.cpp
    src/system/AssetManager.cpp
    src/system/EngineState.cpp
    src/system/InputListener.cpp
    src/system/InputSystem.cpp
    src/system/PhysicsSystem.cpp
    src/system/ProjectManager.cpp
    src/system/RawInputSystem.cpp
    src/system/Renderer.cpp
    src/system/ScriptSystem.cpp
    src/system/sdl/SdlGuiBackend.cpp
    src/system/sdl/SdlRenderBackend.cpp
    src/system/sdl/SdlWindowBackend.cpp
    src/system/TileLayerRenderer.cpp
    src/system/TouchControlSystem.cpp
    src/system/VirtualInputSystem.cpp
)

if(IENGINE_WITH_EDITOR)
    list(APPEND IENGINE_SOURCES
        src/editor/BuildSystem.cpp
        src/editor/ColliderGizmo.cpp
        src/editor/EditorCamera.cpp
        src/editor/EditorCollectionViews.cpp
        src/editor/EntityInspectorComponentActions.cpp
        src/editor/EntityInspectorComponentEditors.cpp
        src/editor/EntityInspectorPanel.cpp
        src/editor/EntityInspectorPrefabActions.cpp
        src/editor/LevelEditor.cpp
        src/editor/LevelEditorFileExplorer.cpp
        src/editor/LevelEditorProject.cpp
        src/editor/LevelEditorViewport.cpp
        src/editor/LevelManagerPanel.cpp
        src/editor/PrefabPanel.cpp
        src/editor/ScriptPropertyInspector.cpp
        src/editor/ScriptPropertyParser.cpp
        src/editor/SpritePalettePanel.cpp
        src/editor/AnimationEditorPanel.cpp
        src/editor/SpritePickerWidget.cpp
        src/editor/TilesetCreator.cpp
        src/editor/TransformGizmo.cpp
        src/editor/ViewportGrid.cpp
    )
endif()

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

set(IENGINE_RUNTIME_ASSETS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Assets")

if(IENGINE_EMBED_LUA_SCRIPTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(IENGINE_EMBEDDED_SCRIPTS_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(IENGINE_EMBEDDED_SCRIPTS_CPP "${IENGINE_EMBEDDED_SCRIPTS_DIR}/EmbeddedScripts.cpp")
    set(IENGINE_EMBEDDED_SCRIPTS_H "${IENGINE_EMBEDDED_SCRIPTS_DIR}/EmbeddedScripts.h")

    file(GLOB_RECURSE IENGINE_LUA_SCRIPT_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Scripts/*.lua"
    )

    add_custom_command(
        OUTPUT
            "${IENGINE_EMBEDDED_SCRIPTS_CPP}"
            "${IENGINE_EMBEDDED_SCRIPTS_H}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IENGINE_EMBEDDED_SCRIPTS_DIR}"
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_lua_scripts.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Scripts"
            "${IENGINE_EMBEDDED_SCRIPTS_DIR}"
        DEPENDS
            ${IENGINE_LUA_SCRIPT_FILES}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_lua_scripts.py"
        VERBATIM
    )

    add_custom_target(iEngineGenerateEmbeddedScripts
        DEPENDS
            "${IENGINE_EMBEDDED_SCRIPTS_CPP}"
            "${IENGINE_EMBEDDED_SCRIPTS_H}"
    )

    list(APPEND IENGINE_SOURCES "${IENGINE_EMBEDDED_SCRIPTS_CPP}")

    set(IENGINE_FILTERED_RUNTIME_ASSETS_DIR "${CMAKE_CURRENT_BINARY_DIR}/runtime_assets/Assets")
    set(IENGINE_FILTERED_RUNTIME_ASSETS_STAMP "${CMAKE_CURRENT_BINARY_DIR}/runtime_assets/assets.stamp")

    file(GLOB_RECURSE IENGINE_RUNTIME_ASSET_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*"
    )
    file(GLOB_RECURSE IENGINE_EDITOR_RUNTIME_LEVEL_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/Assets/Levels/*"
    )
    file(GLOB_RECURSE IENGINE_EDITOR_RUNTIME_PREFAB_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/Assets/Prefabs/*"
    )

    add_custom_command(
        OUTPUT "${IENGINE_FILTERED_RUNTIME_ASSETS_STAMP}"
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_runtime_assets.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "${IENGINE_FILTERED_RUNTIME_ASSETS_DIR}"
            "--exclude-scripts"
            "--level-overlay"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/Assets/Levels:Levels"
            "--overlay"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/Assets/Prefabs:Prefabs"
        COMMAND ${CMAKE_COMMAND} -E touch "${IENGINE_FILTERED_RUNTIME_ASSETS_STAMP}"
        DEPENDS
            ${IENGINE_RUNTIME_ASSET_FILES}
            ${IENGINE_EDITOR_RUNTIME_LEVEL_FILES}
            ${IENGINE_EDITOR_RUNTIME_PREFAB_FILES}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_runtime_assets.py"
        VERBATIM
    )

    add_custom_target(iEnginePrepareRuntimeAssets
        DEPENDS "${IENGINE_FILTERED_RUNTIME_ASSETS_STAMP}"
    )

    set(IENGINE_RUNTIME_ASSETS_SOURCE "${IENGINE_FILTERED_RUNTIME_ASSETS_DIR}")
endif()

if(IENGINE_TARGET_ANDROID)
    add_library(iEngineV2 SHARED ${IENGINE_SOURCES})
    set_target_properties(iEngineV2 PROPERTIES OUTPUT_NAME main)
else()
    add_executable(iEngineV2 ${IENGINE_SOURCES})
endif()

target_compile_features(iEngineV2 PRIVATE cxx_std_17)
target_compile_definitions(iEngineV2 PRIVATE IMGUI_USER_CONFIG=${IENGINE_IMGUI_USER_CONFIG})

if(IENGINE_WITH_EDITOR)
    target_compile_definitions(iEngineV2 PRIVATE IENGINE_WITH_EDITOR)
endif()

if(IENGINE_EMBED_LUA_SCRIPTS)
    add_dependencies(iEngineV2 iEngineGenerateEmbeddedScripts)
    add_dependencies(iEngineV2 iEnginePrepareRuntimeAssets)
    target_compile_definitions(iEngineV2 PRIVATE IENGINE_EMBED_LUA_SCRIPTS)
endif()

target_include_directories(iEngineV2 PRIVATE
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${stb_SOURCE_DIR}
)

if(IENGINE_EMBED_LUA_SCRIPTS)
    target_include_directories(iEngineV2 PRIVATE "${IENGINE_EMBEDDED_SCRIPTS_DIR}")
endif()

target_link_libraries(iEngineV2 PRIVATE
    box2d::box2d
    iEngineLua
    sol2::sol2
    tinyxml2::tinyxml2
)

if(NOT EMSCRIPTEN AND NOT IENGINE_TARGET_IOS AND NOT IENGINE_TARGET_ANDROID AND NOT IENGINE_EMBED_LUA_SCRIPTS)
    add_custom_target(iEngineCopyRuntimeScripts ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:iEngineV2>/Assets/Scripts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Scripts"
            "$<TARGET_FILE_DIR:iEngineV2>/Assets/Scripts"
        COMMENT "Copying Lua scripts to runtime Assets/Scripts"
        VERBATIM
    )
    add_dependencies(iEngineCopyRuntimeScripts iEngineV2)
endif()

if(EMSCRIPTEN)
    target_compile_definitions(iEngineV2 PRIVATE IENGINE_WEB IENGINE_SDL_ONLY)
    target_compile_options(iEngineV2 PRIVATE "-sUSE_SDL=3")
    target_link_options(iEngineV2 PRIVATE
        "-sUSE_SDL=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "--preload-file=${IENGINE_RUNTIME_ASSETS_SOURCE}@/Assets"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/config.xml@/config.xml"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/project.iengine@/project.iengine"
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

    if(IENGINE_EMBED_LUA_SCRIPTS)
        add_custom_command(TARGET iEngineV2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.xml"
                "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/config.xml"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/project.iengine"
                "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/project.iengine"
            COMMAND ${CMAKE_COMMAND} -E rm -rf
                "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/Assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${IENGINE_RUNTIME_ASSETS_SOURCE}"
                "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/Assets"
        )
    else()
        add_custom_command(TARGET iEngineV2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.xml"
                "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/config.xml"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/project.iengine"
                "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/project.iengine"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${IENGINE_RUNTIME_ASSETS_SOURCE}"
                "$<TARGET_BUNDLE_CONTENT_DIR:iEngineV2>/Assets"
        )
    endif()
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

    if(IENGINE_EMBED_LUA_SCRIPTS)
        add_custom_command(TARGET iEngineV2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.xml"
                "$<TARGET_FILE_DIR:iEngineV2>/config.xml"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/project.iengine"
                "$<TARGET_FILE_DIR:iEngineV2>/project.iengine"
            COMMAND ${CMAKE_COMMAND} -E rm -rf
                "$<TARGET_FILE_DIR:iEngineV2>/Assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${IENGINE_RUNTIME_ASSETS_SOURCE}"
                "$<TARGET_FILE_DIR:iEngineV2>/Assets"
        )
    else()
        add_custom_command(TARGET iEngineV2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.xml"
                "$<TARGET_FILE_DIR:iEngineV2>/config.xml"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/project.iengine"
                "$<TARGET_FILE_DIR:iEngineV2>/project.iengine"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${IENGINE_RUNTIME_ASSETS_SOURCE}"
                "$<TARGET_FILE_DIR:iEngineV2>/Assets"
        )
    endif()
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT iEngineV2)
