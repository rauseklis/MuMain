"""Downloads the Real-ESRGAN ncnn build that generate.py needs and unpacks it into tools/ui_hd/.esrgan/.

  python tools/ui_hd/setup_esrgan.py

Windows only (the release is a Windows build; it needs a Vulkan-capable GPU). The download is the official release of
https://github.com/xinntao/Real-ESRGAN and is checked against a known SHA-256 before anything is unpacked.
The program is not part of the repository: tools/ui_hd/.esrgan/ is git-ignored.
"""
import hashlib
import io
import os
import sys
import urllib.request
import zipfile

RELEASE_URL = ("https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.5.0/"
               "realesrgan-ncnn-vulkan-20220424-windows.zip")
RELEASE_SHA256 = "abc02804e17982a3be33675e4d471e91ea374e65b70167abc09e31acb412802d"
TARGET_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".esrgan")
WANTED_PREFIXES = ("realesrgan-ncnn-vulkan.exe", "vcomp140.dll", "models/")


def download():
    print(f"downloading {RELEASE_URL} (about 45 MB)")
    with urllib.request.urlopen(RELEASE_URL) as response:
        return response.read()


def unpack(data):
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for name in archive.namelist():
            if not name.startswith(WANTED_PREFIXES) or name.endswith("/"):
                continue
            destination = os.path.join(TARGET_DIR, *name.split("/"))
            os.makedirs(os.path.dirname(destination), exist_ok=True)
            with archive.open(name) as source, open(destination, "wb") as target:
                target.write(source.read())


def main():
    if os.path.exists(os.path.join(TARGET_DIR, "realesrgan-ncnn-vulkan.exe")):
        print(f"already installed in {TARGET_DIR}")
        return 0
    data = download()
    digest = hashlib.sha256(data).hexdigest()
    if digest != RELEASE_SHA256:
        print(f"checksum mismatch ({digest}); not unpacking", file=sys.stderr)
        return 1
    unpack(data)
    print(f"installed in {TARGET_DIR}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
