#!/usr/bin/env python3

import argparse
import shutil
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


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare runtime assets for packaging.")
    parser.add_argument("source_assets", type=Path)
    parser.add_argument("output_assets", type=Path)
    parser.add_argument("--exclude-scripts", action="store_true")
    parser.add_argument("--overlay", action="append", default=[])
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

    print(f"Prepared runtime assets: {output_assets}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
