#!/usr/bin/env python3

import argparse
from pathlib import Path


def cpp_string_literal(data: bytes) -> str:
    chunks = []
    current = ""

    for value in data:
        if value == 0x0A:
            token = "\\n"
        elif value == 0x0D:
            token = "\\r"
        elif value == 0x09:
            token = "\\t"
        elif value == 0x22:
            token = "\\\""
        elif value == 0x5C:
            token = "\\\\"
        elif 0x20 <= value <= 0x7E:
            token = chr(value)
        else:
            token = f"\\{value:03o}"

        if len(current) + len(token) > 100:
            chunks.append(current)
            current = ""

        current += token

    chunks.append(current)
    return "\n            ".join(f'"{chunk}"' for chunk in chunks)


def write_if_changed(path: Path, content: str) -> None:
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return

    path.write_text(content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Embed Lua scripts into generated C++ sources.")
    parser.add_argument("scripts_dir", type=Path)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()

    scripts_dir = args.scripts_dir.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    scripts = []
    if scripts_dir.exists():
        for path in sorted(scripts_dir.rglob("*.lua")):
            if path.is_file():
                relative_path = path.relative_to(scripts_dir).as_posix()
                scripts.append((relative_path, path.read_bytes()))

    header = """#ifndef IENGINEV2_EMBEDDEDSCRIPTS_H
#define IENGINEV2_EMBEDDEDSCRIPTS_H

#include <string>

class EmbeddedScripts {
public:
    static const char* get(const std::string& name);
};

#endif
"""

    entries = []
    for name, source in scripts:
        entries.append(
            "        {\n"
            f"            \"{name}\",\n"
            f"            {cpp_string_literal(source)}\n"
            "        }"
        )

    if entries:
        array_initializer = "{{\n" + ",\n".join(entries) + "\n    }}"
    else:
        array_initializer = "{}"

    source = f"""#include \"EmbeddedScripts.h\"

#include <array>

namespace {{
struct EmbeddedScriptEntry {{
    const char* name;
    const char* source;
}};

const std::array<EmbeddedScriptEntry, {len(scripts)}> EmbeddedScriptEntries = {array_initializer};
}}

const char* EmbeddedScripts::get(const std::string& name) {{
    for (const EmbeddedScriptEntry& script : EmbeddedScriptEntries) {{
        if (name == script.name) {{
            return script.source;
        }}
    }}

    return nullptr;
}}
"""

    write_if_changed(output_dir / "EmbeddedScripts.h", header)
    write_if_changed(output_dir / "EmbeddedScripts.cpp", source)
    print(f"Embedded {len(scripts)} Lua script(s) from {scripts_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
