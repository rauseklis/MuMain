"""Builds the xShop script folder shipped with the client from Webzen's original Season 6 script.

Keeps only the packages whose item the server can deliver (the names listed in the server's
cashshop-items.csv), together with their products and the categories that still have packages.
Lines are filtered as raw bytes, so the original encoding is preserved.

Usage: python generate-xshop-script.py <original script folder> <cashshop-items.csv> <output folder>
"""
import re
import sys
from pathlib import Path


def normalize(name: bytes) -> str:
    text = name.decode("latin1").strip()
    while text.startswith("["):
        end = text.find("]")
        rest = text[end + 1:].strip()
        text = rest if rest else text[1:end]
    text = re.sub(r"_\d+ Day$", "", text)
    return text.strip().lower()


def read_lines(path: Path) -> list[bytes]:
    return [line.rstrip(b"\r\n") for line in path.read_bytes().splitlines() if line.strip()]


def main() -> None:
    source, csv_path, output = (Path(arg) for arg in sys.argv[1:4])
    supported = {
        line.split(",")[0].strip().lower()
        for line in csv_path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.startswith("#")
    }

    # Packages with more than one product (the "+GP" bundles) hand out more than one item and are not supported.
    packages = [line for line in read_lines(source / "IBSPackage.txt")
                if normalize(line.split(b"@")[3]) in supported
                and len([seq for seq in line.split(b"@")[19].split(b"|") if seq]) == 1]
    used_categories = {line.split(b"@")[0] for line in packages}
    used_products = {seq for line in packages for seq in line.split(b"@")[19].split(b"|") if seq}
    products = [line for line in read_lines(source / "IBSProduct.txt") if line.split(b"@")[0] in used_products]

    categories = []
    roots = {}
    for line in read_lines(source / "IBSCategory.txt"):
        fields = line.split(b"@")
        if fields[6] == b"1":
            roots[fields[4]] = line
        elif fields[0] in used_categories:
            categories.append(line)
    used_roots = {line.split(b"@")[4] for line in categories}
    categories = [roots[root] for root in roots if root in used_roots] + categories

    output.mkdir(parents=True, exist_ok=True)
    for name, lines in (("IBSCategory.txt", categories), ("IBSPackage.txt", packages), ("IBSProduct.txt", products)):
        (output / name).write_bytes(b"\r\n".join(lines) + b"\r\n")
    print(f"{len(categories)} categories, {len(packages)} packages, {len(products)} products")


if __name__ == "__main__":
    main()
