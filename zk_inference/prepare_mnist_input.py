#!/usr/bin/env python3
"""
Create an EZKL-friendly input.json from an MNIST IDX image file using the same
normalization as the active neural-network worker implementation.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from neural_network.cli import INPUT_SIZE, read_idx_images  # type: ignore


def load_image(image_path: Path, index: int) -> list[float]:
    images = read_idx_images(image_path, limit=index + 1)
    return [float(value) for value in images[index].tolist()]


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate input.json for EZKL from MNIST IDX images.")
    parser.add_argument(
        "--images",
        type=Path,
        default=Path("mnist/data/t10k-images.idx3-ubyte"),
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
    print(f"Input length: {len(image)} (expected {INPUT_SIZE})")
    print(f"Value range: [{min(image):.6f}, {max(image):.6f}]")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
