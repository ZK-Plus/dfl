#!/usr/bin/env python3
"""
Create an EZKL-friendly input.json from an MNIST IDX image file using the same
normalization as the C++ code in neural_network/src/functions.cpp.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


IMAGE_START = 16
IMAGE_SIZE = 28 * 28


def normalize(byte_value: int) -> float:
    return (byte_value - 127.5) / 127.5


def load_image(image_path: Path, index: int) -> list[float]:
    raw = image_path.read_bytes()
    start = IMAGE_START + index * IMAGE_SIZE
    end = start + IMAGE_SIZE
    if end > len(raw):
        raise ValueError(f"Image index {index} exceeds file length")

    return [normalize(value) for value in raw[start:end]]


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate input.json for EZKL from MNIST IDX images.")
    parser.add_argument(
        "--images",
        type=Path,
        default=Path("neural_network/data/t10k-images.idx3-ubyte"),
        help="Path to the MNIST IDX image file",
    )
    parser.add_argument(
        "--index",
        type=int,
        default=0,
        help="Zero-based image index inside the IDX file",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=Path("zk_inference/out/input.json"),
        help="Destination JSON file",
    )
    args = parser.parse_args()

    image = load_image(args.images, args.index)
    payload = {"input_data": [image]}

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    print(f"Wrote {args.out}")
    print(f"Image index: {args.index}")
    print(f"Input length: {len(image)}")
    print(f"Value range: [{min(image):.6f}, {max(image):.6f}]")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
