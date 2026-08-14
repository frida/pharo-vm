import argparse
import pathlib
import re

PRIMITIVE_PATTERN = re.compile(
    r"^Primitive(?:WithDepth)?\(\s*(\w+)\s*(?:,\s*(-?\d+)\s*)?\)", re.MULTILINE)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("sources", type=pathlib.Path, nargs="+")
    args = parser.parse_args()

    primitives = []
    for source in args.sources:
        for name, depth in PRIMITIVE_PATTERN.findall(source.read_text()):
            primitives.append((name, int(depth) if depth else 0))
    primitives.sort()

    args.output.write_text(render(primitives))


def render(primitives):
    lines = [
        '#include "pharovm/pharo.h"',
        "",
    ]
    for name, _ in primitives:
        lines += [f"extern sqInt {name}(void);"]
    lines += [
        "",
        "void *ffi_exports[][3] =",
        "{",
    ]
    for name, depth in primitives:
        lines += [f'    {{ 0, "{name}\\000\\{depth & 0xff:03o}", (void *) {name} }},']
    lines += [
        "    { 0, 0, 0 }",
        "};",
        "",
    ]
    return "\n".join(lines)


main()
