#!/usr/bin/env python3

import argparse
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path


def copy_directory_contents(source: Path, destination: Path) -> None:
    if not source.exists() or not source.is_dir():
        return

    destination.mkdir(parents=True, exist_ok=True)
    for item in source.iterdir():
        target = destination / item.name
        if item.is_dir():
            copy_directory_contents(item, target)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(item, target)


def level_file_name(path_text: str) -> str:
    return Path(path_text).name


def collect_release_level_files(levels_config: Path) -> set[str]:
    level_files = {"levels.xml"}
    if not levels_config.exists():
        return level_files

    tree = ET.parse(levels_config)
    root = tree.getroot()

    initial_level = root.find("initialLevel")
    if initial_level is not None:
        initial_path = initial_level.get("path", "")
        if initial_path:
            level_files.add(level_file_name(initial_path))
    else:
        level_files.add("current.ilevel")

    for level in root.findall("level"):
        if level.get("active", "false").lower() not in {"true", "1"}:
            continue

        file_name = level.get("fileName", "")
        if file_name:
            level_files.add(level_file_name(file_name))

    return level_files


def copy_release_levels(source: Path, destination: Path) -> None:
    if destination.exists():
        shutil.rmtree(destination)

    if not source.exists() or not source.is_dir():
        return

    destination.mkdir(parents=True, exist_ok=True)
    level_files = collect_release_level_files(source / "levels.xml")

    for file_name in sorted(level_files):
        source_file = source / file_name
        if source_file.exists() and source_file.is_file():
            shutil.copy2(source_file, destination / file_name)


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare runtime assets for packaging.")
    parser.add_argument("source_assets", type=Path)
    parser.add_argument("output_assets", type=Path)
    parser.add_argument("--exclude-scripts", action="store_true")
    parser.add_argument("--overlay", action="append", default=[])
    parser.add_argument("--level-overlay", action="append", default=[])
    args = parser.parse_args()

    source_assets = args.source_assets.resolve()
    output_assets = args.output_assets.resolve()

    if output_assets.exists():
        shutil.rmtree(output_assets)

    copy_directory_contents(source_assets, output_assets)

    if args.exclude_scripts:
        scripts_path = output_assets / "Scripts"
        if scripts_path.exists():
            shutil.rmtree(scripts_path)

    for overlay in args.overlay:
        source_text, separator, destination_text = overlay.partition(":")
        if separator == "":
            raise ValueError(f"Overlay must use source:destination format: {overlay}")

        overlay_source = Path(source_text).resolve()
        overlay_destination = output_assets / destination_text
        copy_directory_contents(overlay_source, overlay_destination)

    for overlay in args.level_overlay:
        source_text, separator, destination_text = overlay.partition(":")
        if separator == "":
            raise ValueError(f"Level overlay must use source:destination format: {overlay}")

        overlay_source = Path(source_text).resolve()
        overlay_destination = output_assets / destination_text
        copy_release_levels(overlay_source, overlay_destination)

    print(f"Prepared runtime assets: {output_assets}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
