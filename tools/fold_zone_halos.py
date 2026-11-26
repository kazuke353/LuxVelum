import argparse
import sys
from pathlib import Path

SECTIONS = ("Zones", "SunHalo", "MoonHalo", "Density")


def read_zone_file(path):
    header = []
    sections = {name: {} for name in SECTIONS}
    current = None
    for raw in Path(path).read_text(encoding="utf-8-sig").splitlines():
        line = raw.strip()
        if not line:
            continue
        if line.startswith(";"):
            if current is None:
                header.append(raw.rstrip())
            continue
        if line.startswith("[") and line.endswith("]"):
            current = line[1:-1]
            sections.setdefault(current, {})
            continue
        if current is None or "=" not in line:
            raise ValueError(f"{path}: unexpected line {raw!r}")
        key, value = line.split("=", 1)
        sections[current][key.strip()] = value.strip()
    return header, sections


def zone_order(shipped, own):
    order = list(shipped["Zones"])
    for section in SECTIONS:
        for zone in list(shipped[section]) + list(own[section]):
            if zone not in order:
                order.append(zone)
    return order


def folded_zone_text(shipped_path, own_path):
    header, shipped = read_zone_file(shipped_path)
    _, own = read_zone_file(own_path)
    lines = list(header)
    order = zone_order(shipped, own)
    for section in SECTIONS:
        lines.append(f"[{section}]")
        for zone in order:
            value = own[section].get(zone, shipped[section].get(zone))
            if value is not None:
                lines.append(f"{zone}={value}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(
        description="Fold the owner's WTF\\Luxvelum-zones.ini into the shipped data/zones.ini; his values win.")
    parser.add_argument("own", help="the owner's Luxvelum-zones.ini")
    parser.add_argument("--shipped", default=str(Path(__file__).resolve().parent.parent / "data" / "zones.ini"))
    args = parser.parse_args()
    text = folded_zone_text(args.shipped, args.own)
    Path(args.shipped).write_text(text, encoding="utf-8")
    _, folded = read_zone_file(args.shipped)
    print(f"{len(folded['Zones'])} zones in {args.shipped}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
