#!/usr/bin/env python3
"""
Export the federated model stored in aggregated.bin into a PyTorch checkpoint
and optionally an ONNX model for downstream zk-inference pipelines such as EZKL.

The binary layout matches neural_network/src/functions.cpp::save():
W1, B1, W2, B2, W3, B3 as float64 values, written column-major.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

import numpy as np

try:
    import torch
    import torch.nn as nn
except ImportError:  # pragma: no cover - handled at runtime
    torch = None
    nn = None


INPUT_SIZE = 784
L1_SIZE = 200
L2_SIZE = 50
OUTPUT_SIZE = 10


@dataclass(frozen=True)
class MatrixSpec:
    name: str
    rows: int
    cols: int


MODEL_LAYOUT: Tuple[MatrixSpec, ...] = (
    MatrixSpec("W1", L1_SIZE, INPUT_SIZE),
    MatrixSpec("B1", L1_SIZE, 1),
    MatrixSpec("W2", L2_SIZE, L1_SIZE),
    MatrixSpec("B2", L2_SIZE, 1),
    MatrixSpec("W3", OUTPUT_SIZE, L2_SIZE),
    MatrixSpec("B3", OUTPUT_SIZE, 1),
)


def ensure_torch() -> None:
    if torch is None or nn is None:
        raise RuntimeError(
            "PyTorch is required for this script. Install it first, for example:\n"
            "  pip install torch"
        )


def read_cpp_matrix(blob: bytes, offset: int, rows: int, cols: int) -> Tuple[np.ndarray, int]:
    element_count = rows * cols
    byte_count = element_count * 8
    end = offset + byte_count
    if end > len(blob):
        raise ValueError("Unexpected end of file while reading matrix data")

    flat = np.frombuffer(blob[offset:end], dtype="<f8", count=element_count)
    matrix = flat.reshape((cols, rows)).T.copy()
    return matrix, end


def load_aggregated_bin(model_path: Path) -> Dict[str, np.ndarray]:
    blob = model_path.read_bytes()
    matrices: Dict[str, np.ndarray] = {}
    offset = 0

    for spec in MODEL_LAYOUT:
        matrix, offset = read_cpp_matrix(blob, offset, spec.rows, spec.cols)
        matrices[spec.name] = matrix

    if offset != len(blob):
        trailing = len(blob) - offset
        raise ValueError(
            f"Model file contains {trailing} trailing bytes. "
            "This usually means the layout assumptions are wrong."
        )

    return matrices


def cpp_forward_numpy(inputs: np.ndarray, weights: Dict[str, np.ndarray]) -> Dict[str, np.ndarray]:
    if inputs.ndim != 2 or inputs.shape[0] != INPUT_SIZE:
        raise ValueError(f"Expected input matrix shape ({INPUT_SIZE}, batch), got {inputs.shape}")

    z1 = weights["W1"] @ inputs + weights["B1"]
    a1 = np.tanh(z1)
    z2 = weights["W2"] @ a1 + weights["B2"]
    a2 = np.tanh(z2)
    z3 = weights["W3"] @ a2 + weights["B3"]

    max_per_col = np.max(z3, axis=0, keepdims=True)
    exp_shifted = np.exp(z3 - max_per_col)
    a3 = exp_shifted / np.sum(exp_shifted, axis=0, keepdims=True)

    return {
        "Z1": z1,
        "A1": a1,
        "Z2": z2,
        "A2": a2,
        "Z3": z3,
        "A3": a3,
    }


class FederatedMLP(nn.Module):
    def __init__(self, apply_softmax: bool = True) -> None:
        super().__init__()
        self.apply_softmax = apply_softmax
        self.fc1 = nn.Linear(INPUT_SIZE, L1_SIZE)
        self.fc2 = nn.Linear(L1_SIZE, L2_SIZE)
        self.fc3 = nn.Linear(L2_SIZE, OUTPUT_SIZE)

    def forward(self, x: "torch.Tensor") -> "torch.Tensor":
        x = torch.tanh(self.fc1(x))
        x = torch.tanh(self.fc2(x))
        logits = self.fc3(x)
        if self.apply_softmax:
            return torch.softmax(logits, dim=1)
        return logits


class LogitsOnlyMLP(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.model = FederatedMLP(apply_softmax=False)

    def forward(self, x: "torch.Tensor") -> "torch.Tensor":
        return self.model(x)


def load_state_dict_into_model(model: "nn.Module", weights: Dict[str, np.ndarray]) -> "nn.Module":
    state_dict = {
        "fc1.weight": torch.from_numpy(weights["W1"]).to(torch.float32),
        "fc1.bias": torch.from_numpy(weights["B1"].reshape(-1)).to(torch.float32),
        "fc2.weight": torch.from_numpy(weights["W2"]).to(torch.float32),
        "fc2.bias": torch.from_numpy(weights["B2"].reshape(-1)).to(torch.float32),
        "fc3.weight": torch.from_numpy(weights["W3"]).to(torch.float32),
        "fc3.bias": torch.from_numpy(weights["B3"].reshape(-1)).to(torch.float32),
    }
    model.load_state_dict(state_dict)
    model.eval()
    return model


def build_torch_model(weights: Dict[str, np.ndarray]) -> "FederatedMLP":
    ensure_torch()
    model = FederatedMLP(apply_softmax=True)
    return load_state_dict_into_model(model, weights)


def build_logits_model(weights: Dict[str, np.ndarray]) -> "LogitsOnlyMLP":
    ensure_torch()
    model = LogitsOnlyMLP()
    nested_state_dict = {
        f"model.{key}": value
        for key, value in load_state_dict_into_model(FederatedMLP(apply_softmax=False), weights).state_dict().items()
    }
    model.load_state_dict(nested_state_dict)
    model.eval()
    return model


def generate_sample_inputs(batch_size: int, seed: int) -> np.ndarray:
    rng = np.random.default_rng(seed)
    return rng.uniform(-1.0, 1.0, size=(INPUT_SIZE, batch_size)).astype(np.float64)


def check_torch_parity(weights: Dict[str, np.ndarray], batch_size: int, seed: int) -> Dict[str, float]:
    ensure_torch()
    numpy_inputs = generate_sample_inputs(batch_size=batch_size, seed=seed)
    numpy_result = cpp_forward_numpy(numpy_inputs, weights)["A3"]

    model = build_torch_model(weights)
    torch_inputs = torch.from_numpy(numpy_inputs.T).to(torch.float32)
    with torch.no_grad():
        torch_result = model(torch_inputs).cpu().numpy().T

    abs_diff = np.abs(numpy_result - torch_result)
    return {
        "batch_size": batch_size,
        "seed": seed,
        "max_abs_diff": float(abs_diff.max()),
        "mean_abs_diff": float(abs_diff.mean()),
    }


def write_manifest(
    manifest_path: Path,
    model_path: Path,
    export_dir: Path,
    weights: Dict[str, np.ndarray],
    parity: Dict[str, float] | None,
    onnx_path: Path | None,
) -> None:
    manifest = {
        "source_model": str(model_path),
        "export_dir": str(export_dir),
        "layout": [
            {"name": spec.name, "rows": spec.rows, "cols": spec.cols}
            for spec in MODEL_LAYOUT
        ],
        "files": {
            "weights_npz": "weights_fp64.npz",
            "torch_state_dict": "model_state_dict.pt",
            "onnx_model": onnx_path.name if onnx_path else None,
            "onnx_logits_model": "model_logits.onnx" if onnx_path else None,
        },
        "activation": {
            "hidden_layers": "tanh",
            "output": "softmax",
        },
        "notes": [
            "aggregated.bin is parsed as float64 values written in Eigen column-major order",
            "the exported PyTorch/ONNX model uses float32 parameters for interoperability",
            "for EZKL, proving logits or argmax is usually easier than proving softmax probabilities",
        ],
        "weight_shapes": {name: list(array.shape) for name, array in weights.items()},
        "parity_check": parity,
    }
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def export_artifacts(
    model_path: Path,
    output_dir: Path,
    export_onnx: bool,
    batch_size: int,
    export_batch_size: int,
    seed: int,
) -> None:
    ensure_torch()
    weights = load_aggregated_bin(model_path)
    output_dir.mkdir(parents=True, exist_ok=True)

    np.savez(output_dir / "weights_fp64.npz", **weights)

    model = build_torch_model(weights)
    logits_model = build_logits_model(weights)
    torch.save(model.state_dict(), output_dir / "model_state_dict.pt")

    parity = check_torch_parity(weights, batch_size=batch_size, seed=seed)

    onnx_path: Path | None = None
    if export_onnx:
        onnx_path = output_dir / "model.onnx"
        logits_onnx_path = output_dir / "model_logits.onnx"
        # EZKL is happier with a static input shape than with symbolic batch axes.
        dummy_input = torch.randn(export_batch_size, INPUT_SIZE, dtype=torch.float32)
        torch.onnx.export(
            model,
            dummy_input,
            str(onnx_path),
            input_names=["input"],
            output_names=["probabilities"],
            opset_version=18,
            dynamo=False,
        )
        torch.onnx.export(
            logits_model,
            dummy_input,
            str(logits_onnx_path),
            input_names=["input"],
            output_names=["logits"],
            opset_version=18,
            dynamo=False,
        )

    write_manifest(
        manifest_path=output_dir / "export_manifest.json",
        model_path=model_path,
        export_dir=output_dir,
        weights=weights,
        parity=parity,
        onnx_path=onnx_path,
    )

    print(f"Loaded model: {model_path}")
    print(f"Export directory: {output_dir}")
    print("Saved:")
    print(f"  - {output_dir / 'weights_fp64.npz'}")
    print(f"  - {output_dir / 'model_state_dict.pt'}")
    if onnx_path:
        print(f"  - {onnx_path}")
        print(f"  - {output_dir / 'model_logits.onnx'}")
    print(f"  - {output_dir / 'export_manifest.json'}")
    print("Parity check:")
    print(json.dumps(parity, indent=2))


def inspect_model(model_path: Path) -> None:
    weights = load_aggregated_bin(model_path)
    summary = {
        name: {
            "shape": list(matrix.shape),
            "min": float(matrix.min()),
            "max": float(matrix.max()),
            "mean": float(matrix.mean()),
        }
        for name, matrix in weights.items()
    }
    print(json.dumps(summary, indent=2))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Export the federated aggregated.bin model into PyTorch and ONNX artifacts."
    )
    parser.add_argument(
        "--model",
        type=Path,
        default=Path("IPFS output/2026-04-28T17-17-32-699Z-aggregated.bin"),
        help="Path to the aggregated.bin model file",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=Path("zk_inference/out"),
        help="Directory for exported artifacts",
    )
    parser.add_argument(
        "--no-onnx",
        action="store_true",
        help="Skip ONNX export and only write NumPy/PyTorch artifacts",
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=4,
        help="Batch size used for the parity test",
    )
    parser.add_argument(
        "--export-batch-size",
        type=int,
        default=1,
        help="Fixed batch size used for ONNX export; EZKL usually prefers static batch=1",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=7,
        help="Random seed for parity-test sample inputs",
    )
    parser.add_argument(
        "--inspect",
        action="store_true",
        help="Only inspect the binary model and print matrix statistics",
    )
    return parser


def main(argv: Iterable[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(list(argv) if argv is not None else None)

    try:
        if args.inspect:
            inspect_model(args.model)
            return 0

        export_artifacts(
            model_path=args.model,
            output_dir=args.out,
            export_onnx=not args.no_onnx,
            batch_size=args.batch_size,
            export_batch_size=args.export_batch_size,
            seed=args.seed,
        )
        return 0
    except Exception as exc:  # pragma: no cover - CLI reporting
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
