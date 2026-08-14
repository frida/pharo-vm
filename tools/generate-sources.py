#!/usr/bin/env python3
import argparse
import io
import pathlib
import platform
import stat
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

HOST = "https://github.com/frida/pharo-vm/releases/download/slang-1"
VM_VERSION = "PharoVM-10.3.2-b8793dd2"
DARWIN_VM_VERSION = "PharoVM-v12.0.5-beta-0.7884d2853"
WINDOWS_ARM64_VM_VERSION = "PharoVM-10.0.9-de76067"
MACHINE_NAMES = {"AMD64": "x86_64"}
IMAGE = "47ac9a9f7ec7"

PHAROS_IMAGE = "https://files.pharo.org/image/130/Pharo13.0-SNAPSHOT.build.732.sha.e84a2d15c7.arch.64bit.zip"
ICEBERG_REMOTE = "httpsUrl"


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkout", required=True)
    parser.add_argument("--flavour", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--cache", required=True)
    parser.add_argument("--prepare-image")
    arguments = parser.parse_args(argv[1:])

    checkout = pathlib.Path(arguments.checkout)
    output = pathlib.Path(arguments.output)
    cache = pathlib.Path(arguments.cache)

    if arguments.prepare_image is not None:
        prepare_image(checkout, cache, pathlib.Path(arguments.prepare_image))
        return

    if not (output / "generated").is_dir():
        generate(arguments.flavour, output, cache)

    print(output / "generated")


def generate(flavour, output, cache):
    vm = fetch_vm(cache)
    image = fetch_image(cache)

    output.mkdir(parents=True, exist_ok=True)
    run(vm, "--headless", image, "--no-default-preferences",
        "perform", "PharoVMMaker", "generate:outputDirectory:",
        flavour, str(output))


def fetch_vm(cache):
    machine = "%s-%s" % (platform.system(),
                         MACHINE_NAMES.get(platform.machine(), platform.machine()))
    directory = cache / "vm"
    binary = directory / "pharo"
    archive = "%s-%s-bin.zip" % (VM_VERSION, machine)

    if machine.startswith("Darwin"):
        binary = directory / "Pharo.app/Contents/MacOS/Pharo"
        archive = "%s-%s-bin.zip" % (DARWIN_VM_VERSION, machine)
    elif machine.startswith("Windows"):
        binary = directory / "PharoConsole.exe"
        if platform.machine() == "ARM64":
            archive = ("%s-Windows-ARM64-stockReplacement-bin.zip"
                       % WINDOWS_ARM64_VM_VERSION)

    if not binary.exists():
        unzip(fetch("%s/%s" % (HOST, archive)), directory)

    return str(binary)


def fetch_image(cache):
    directory = cache / "image"
    image = directory / "VMMaker.image"

    if not image.exists():
        directory.mkdir(parents=True, exist_ok=True)
        with tarfile.open(fileobj=fetch(
                "%s/vmmaker-image-%s.tar.xz" % (HOST, IMAGE))) as tar:
            tar.extractall(directory)

    return str(image)


def prepare_image(checkout, cache, destination):
    directory = cache / "prepare"
    vm = fetch_vm(cache)

    unzip(fetch(PHAROS_IMAGE), directory)
    run(vm, "--headless", str(next(directory.glob("Pharo*.image"))),
        "--no-default-preferences", "save", "VMMaker")

    image = directory / "VMMaker.image"
    run(vm, "--headless", str(image), "--no-default-preferences",
        "--save", "--quit", str(checkout / "scripts/installVMMaker.st"),
        str(checkout), ICEBERG_REMOTE)

    with tarfile.open(destination, "w:xz") as tar:
        for entry in sorted(directory.iterdir()):
            if entry.name.startswith("Pharo") and entry.suffix in [".image",
                                                                   ".changes"]:
                continue
            tar.add(entry, arcname=entry.name)


def fetch(url):
    with urllib.request.urlopen(url) as response:
        return io.BytesIO(response.read())


def unzip(archive, destination):
    destination.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as contents:
        for entry in contents.infolist():
            extracted = pathlib.Path(contents.extract(entry, destination))
            unix_mode = entry.external_attr >> 16
            if unix_mode != 0:
                extracted.chmod(stat.S_IMODE(unix_mode))


def run(*command):
    result = subprocess.run(command, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if result.returncode != 0:
        sys.stderr.write(result.stdout.decode("utf-8", "replace"))
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main(sys.argv)
