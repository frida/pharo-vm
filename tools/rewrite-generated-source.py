#!/usr/bin/env python3
import argparse
import re
import sys


RELOCATABLE_REGIONS = ["codeZone", "stack"]


def tolerate_relocated_regions(source):
    for region in RELOCATABLE_REGIONS:
        source = re.sub(
            r'logError\("Could not allocate ' + region
            + r' in the expected place([^\n]*)\n\s*error\("Error allocating"\);\n',
            r'logDebug("Could not allocate ' + region
            + r' in the expected place\1\n',
            source)
    return source


def confine_pthread_to_apple(source):
    return re.sub(
        r"(?:#if __APPLE__\n)?#include <pthread\.h>\n(?:#endif\n)?",
        "#if __APPLE__\n#include <pthread.h>\n#endif\n",
        source)


def rename_config_header(source):
    return re.sub(r'#include\s*"config\.h"', '#include "pharovmConfig.h"',
                  source)


RULES = {
    "tolerate-relocated-regions": tolerate_relocated_regions,
    "confine-pthread-to-apple": confine_pthread_to_apple,
    "rename-config-header": rename_config_header,
}


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--rule", action="append", default=[],
                        choices=sorted(RULES.keys()), required=True)
    args = parser.parse_args(argv[1:])

    with open(args.input, "r", encoding="utf-8", errors="surrogateescape") as f:
        source = f.read()

    for rule in args.rule:
        source = RULES[rule](source)

    with open(args.output, "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(source)


if __name__ == "__main__":
    main(sys.argv)
