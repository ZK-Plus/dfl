#!/usr/bin/env python3
"""MCP server exposing the local zk_inference pipeline as tools."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

try:
    from mcp.server.fastmcp import FastMCP
except ImportError:  # pragma: no cover - used by deterministic local mode
    FastMCP = None


REPO_ROOT = Path(__file__).resolve().parents[1]
PYTHON = REPO_ROOT / ".venv" / "bin" / "python"
if not PYTHON.exists():
    PYTHON = Path(sys.executable)

class _NoMCP:
    def tool(self):
        return lambda function: function

    def run(self) -> None:
        raise RuntimeError("MCP server mode requires: pip install -r agent/requirements.txt")


mcp = FastMCP("master-thesis-zk-inference") if FastMCP is not None else _NoMCP()


def _repo_path(path: str | Path) -> Path:
    candidate = Path(path)
    if not candidate.is_absolute():
        candidate = REPO_ROOT / candidate
    return candidate.resolve()


def _run(args: list[str], cwd: Path = REPO_ROOT) -> dict[str, Any]:
    env = os.environ.copy()
    env["PYTHONPATH"] = f"{REPO_ROOT / 'python_worker'}:{REPO_ROOT / 'zk_inference'}:{env.get('PYTHONPATH', '')}"
    completed = subprocess.run(args, cwd=cwd, env=env, text=True, capture_output=True, check=False)
    return {
        "command": args,
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
        "ok": completed.returncode == 0,
    }


@mcp.tool()
def export_model(model_path: str, out_dir: str = "zk_inference/out") -> dict[str, Any]:
    """Export an aggregated .bin model into ONNX/EZKL artifacts."""

    model = _repo_path(model_path)
    out = _repo_path(out_dir)
    result = _run([str(PYTHON), "zk_inference/export_model.py", "--model", str(model), "--out", str(out)])
    result["artifacts"] = {
        "workdir": str(out),
        "model_logits": str(out / "model_logits.onnx"),
        "model_probabilities": str(out / "model.onnx"),
        "manifest": str(out / "export_manifest.json"),
    }
    return result


@mcp.tool()
def create_single_image_query(
    model_path: str,
    index: int | None = None,
    images: str = "neural_network/data/t10k-images.idx3-ubyte",
    labels: str = "neural_network/data/t10k-labels.idx1-ubyte",
    out_dir: str = "zk_inference/single_query",
    input_json: str = "zk_inference/out/input.json",
) -> dict[str, Any]:
    """Create a one-image MNIST query and EZKL input.json for the model."""

    args = [
        str(PYTHON),
        "zk_inference/create_single_mnist_query.py",
        "--model",
        str(_repo_path(model_path)),
        "--images",
        str(_repo_path(images)),
        "--labels",
        str(_repo_path(labels)),
        "--out-dir",
        str(_repo_path(out_dir)),
        "--input-json",
        str(_repo_path(input_json)),
    ]
    if index is not None:
        args.extend(["--index", str(index)])
    result = _run(args)
    metadata_path = _repo_path(out_dir) / "prediction.json"
    result["prediction_metadata"] = str(metadata_path)
    if metadata_path.exists():
        result["prediction"] = json.loads(metadata_path.read_text(encoding="utf-8"))
    return result


@mcp.tool()
def run_ezkl(
    workdir: str = "zk_inference/out",
    model: str = "model_logits.onnx",
    data: str = "input.json",
    skip_calibration: bool = True,
) -> dict[str, Any]:
    """Run EZKL setup, witness generation, proof generation, and verification."""

    args = [
        str(PYTHON),
        "zk_inference/run_ezkl.py",
        "--workdir",
        str(_repo_path(workdir)),
        "--model",
        model,
        "--data",
        data,
    ]
    if skip_calibration:
        args.append("--skip-calibration")
    result = _run(args)
    out = _repo_path(workdir)
    result["artifacts"] = {
        "proof": str(out / "proof.json"),
        "witness": str(out / "witness.json"),
        "settings": str(out / "settings.json"),
        "verification_key": str(out / "vk.key"),
    }
    return result


@mcp.tool()
def prove_single_image(
    model_path: str,
    signature_path: str,
    index: int | None = None,
    workdir: str = "zk_inference/out",
    query_dir: str = "zk_inference/single_query",
    skip_calibration: bool = True,
) -> dict[str, Any]:
    """Export a model, create one-image input, run EZKL, and return prediction/proof paths."""

    model = _repo_path(model_path)
    signature = _repo_path(signature_path)
    work = _repo_path(workdir)
    query = _repo_path(query_dir)
    work.mkdir(parents=True, exist_ok=True)
    shutil.copy2(signature, work / signature.name)

    export_result = export_model(str(model), str(work))
    if not export_result["ok"]:
        return {"ok": False, "stage": "export_model", "export": export_result}

    query_result = create_single_image_query(
        model_path=str(model),
        index=index,
        out_dir=str(query),
        input_json=str(work / "input.json"),
    )
    if not query_result["ok"]:
        return {"ok": False, "stage": "create_single_image_query", "export": export_result, "query": query_result}

    ezkl_result = run_ezkl(workdir=str(work), skip_calibration=skip_calibration)
    return {
        "ok": bool(ezkl_result["ok"]),
        "stage": "done" if ezkl_result["ok"] else "run_ezkl",
        "model_path": str(model),
        "signature_path": str(signature),
        "signature_copy": str(work / signature.name),
        "prediction": query_result.get("prediction"),
        "proof": ezkl_result.get("artifacts", {}).get("proof"),
        "export": export_result,
        "query": query_result,
        "ezkl": ezkl_result,
    }


if __name__ == "__main__":
    mcp.run()
