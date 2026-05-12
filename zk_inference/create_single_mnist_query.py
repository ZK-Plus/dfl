#!/usr/bin/env python3
"""
Create a one-image MNIST query dataset and EZKL input for a single inference.

By default the script searches for the first correctly classified MNIST test
image for the selected model. It writes a one-sample IDX image/label pair,
an EZKL input.json, a portable PGM preview, and prediction metadata.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

try:
    import torch
except ImportError:  # pragma: no cover - handled at runtime
    torch = None


REPO_ROOT = Path(__file__).resolve().parents[1]
PYTHON_WORKER_DIR = REPO_ROOT / "python_worker"
if str(PYTHON_WORKER_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_WORKER_DIR))
if str(REPO_ROOT / "zk_inference") not in sys.path:
    sys.path.insert(0, str(REPO_ROOT / "zk_inference"))

from thesis_pytorch_compat.cli import INPUT_SIZE, read_idx_images, read_idx_labels  # type: ignore
from export_model import default_model_path, load_worker_model  # type: ignore


IMAGE_ROWS = 28
IMAGE_COLS = 28


def ensure_torch() -> None:
    if torch is None:
        raise RuntimeError("PyTorch is required. Install it with: pip install torch")


def read_raw_image_bytes(images_path: Path, index: int) -> bytes:
    blob = images_path.read_bytes()
    start = 16 + index * INPUT_SIZE
    end = start + INPUT_SIZE
    if end > len(blob):
        raise ValueError(f"Image index {index} exceeds {images_path}")
    return blob[start:end]


def write_single_idx_files(raw_image: bytes, label: int, out_dir: Path) -> tuple[Path, Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    image_path = out_dir / "single-image.idx3-ubyte"
    label_path = out_dir / "single-label.idx1-ubyte"
    image_path.write_bytes(struct.pack(">IIII", 2051, 1, IMAGE_ROWS, IMAGE_COLS) + raw_image)
    label_path.write_bytes(struct.pack(">II", 2049, 1) + bytes([label]))
    return image_path, label_path


def write_pgm_preview(raw_image: bytes, out_dir: Path) -> Path:
    preview_path = out_dir / "single-image.pgm"
    header = f"P5\n{IMAGE_COLS} {IMAGE_ROWS}\n255\n".encode("ascii")
    preview_path.write_bytes(header + raw_image)
    return preview_path


def predict(model_path: Path, normalized_image: "torch.Tensor") -> tuple[int, list[float], list[float]]:
    model = load_worker_model(model_path).eval()
    with torch.no_grad():
        logits = model(normalized_image.reshape(1, INPUT_SIZE)).reshape(-1)
        probabilities = torch.softmax(logits, dim=0)
    return (
        int(logits.argmax().item()),
        [float(value) for value in logits.tolist()],
        [float(value) for value in probabilities.tolist()],
    )


def select_index(
    model_path: Path,
    images_path: Path,
    labels_path: Path,
    preferred_index: int | None,
    search_limit: int,
) -> tuple[int, int, int, list[float], list[float]]:
    ensure_torch()
    if preferred_index is not None:
        images = read_idx_images(images_path, limit=preferred_index + 1)
        labels = read_idx_labels(labels_path, limit=preferred_index + 1)
        label = int(labels[preferred_index].item())
        predicted, logits, probabilities = predict(model_path, images[preferred_index])
        return preferred_index, label, predicted, logits, probabilities

    limit = min(search_limit, labels_path.stat().st_size - 8)
    images = read_idx_images(images_path, limit=limit)
    labels = read_idx_labels(labels_path, limit=limit)
    model = load_worker_model(model_path).eval()
    with torch.no_grad():
        logits_batch = model(images)
        predictions = logits_batch.argmax(dim=1)

    for index in range(limit):
        label = int(labels[index].item())
        predicted = int(predictions[index].item())
        if predicted == label:
            logits = logits_batch[index]
            probabilities = torch.softmax(logits, dim=0)
            return (
                index,
                label,
                predicted,
                [float(value) for value in logits.tolist()],
                [float(value) for value in probabilities.tolist()],
            )

    raise ValueError(f"No correctly classified image found in first {limit} samples")


def write_query_input(normalized_image: "torch.Tensor", out_path: Path) -> None:
    payload = {"input_data": [[float(value) for value in normalized_image.tolist()]]}
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a single-image MNIST inference query for EZKL.")
    parser.add_argument("--model", type=Path, default=default_model_path(), help="Model to use for prediction")
    parser.add_argument("--images", type=Path, default=Path("neural_network/data/t10k-images.idx3-ubyte"))
    parser.add_argument("--labels", type=Path, default=Path("neural_network/data/t10k-labels.idx1-ubyte"))
    parser.add_argument("--index", type=int, default=None, help="Use a specific MNIST test index")
    parser.add_argument("--search-limit", type=int, default=1000, help="Search range when --index is omitted")
    parser.add_argument("--out-dir", type=Path, default=Path("zk_inference/single_query"))
    parser.add_argument("--input-json", type=Path, default=Path("zk_inference/out/input.json"))
    args = parser.parse_args()

    index, label, predicted, logits, probabilities = select_index(
        model_path=args.model,
        images_path=args.images,
        labels_path=args.labels,
        preferred_index=args.index,
        search_limit=args.search_limit,
    )
    raw_image = read_raw_image_bytes(args.images, index)
    normalized_image = read_idx_images(args.images, limit=index + 1)[index]

    image_path, label_path = write_single_idx_files(raw_image, label, args.out_dir)
    preview_path = write_pgm_preview(raw_image, args.out_dir)
    write_query_input(normalized_image, args.input_json)

    metadata = {
        "source_model": str(args.model),
        "source_images": str(args.images),
        "source_labels": str(args.labels),
        "source_index": index,
        "true_label": label,
        "predicted_label": predicted,
        "correct": predicted == label,
        "logits": logits,
        "probabilities": probabilities,
        "files": {
            "single_image_idx": str(image_path),
            "single_label_idx": str(label_path),
            "preview_pgm": str(preview_path),
            "ezkl_input_json": str(args.input_json),
        },
    }
    metadata_path = args.out_dir / "prediction.json"
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    print(f"Selected MNIST test index: {index}")
    print(f"True label: {label}")
    print(f"Predicted label: {predicted}")
    print(f"Correct: {predicted == label}")
    print(f"Wrote {image_path}")
    print(f"Wrote {label_path}")
    print(f"Wrote {preview_path}")
    print(f"Wrote {args.input_json}")
    print(f"Wrote {metadata_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
