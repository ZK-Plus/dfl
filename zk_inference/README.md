# zk_inference

Dieses Verzeichnis exportiert das aktuell vom Python-Worker erzeugte federated-learning Modell in einen EZKL-kompatiblen Inferenzpfad.

Der neue Pfad verwendet `python_worker/thesis_pytorch_compat/cli.py` als Quelle der Wahrheit fuer Modellklasse, Modelllayout und MNIST-Normalisierung. Die frueheren Annahmen aus dem C++-Code werden hier nicht mehr nachgebaut.

## Ziel

Der Trainings- und Aggregationspfad liegt jetzt im Python-Worker:

- Training, Transfer, Aggregation und Signatur in `python_worker/thesis_pytorch_compat/cli.py`
- Orchestrierung und Blockchain/IPFS-Anbindung weiterhin ueber `node_server`
- Zero-Knowledge-Inference in `zk_inference`

Der Exportpfad ist:

```text
python_worker aggregated.bin -> PyTorch state_dict -> ONNX -> EZKL
```

## Voraussetzungen

```bash
pip install -r zk_inference/requirements.txt
```

Falls der Python-Worker noch nicht installiert ist:

```bash
pip install -e python_worker
```

Das ist optional, wenn die Skripte aus dem Repository-Root gestartet werden, weil `zk_inference` den lokalen `python_worker` automatisch in den Importpfad aufnimmt.

## Export

Modell inspizieren:

```bash
python3 zk_inference/export_model.py \
  --model neural_network/data/results_iid/aggregated.bin \
  --inspect
```

PyTorch- und ONNX-Artefakte erzeugen:

```bash
python3 zk_inference/export_model.py \
  --model neural_network/data/results_iid/aggregated.bin \
  --out zk_inference/out
```

Wenn `node_server/data/results_iid/aggregated.bin` existiert, wird dieser Pfad automatisch als Default verwendet. Andernfalls nimmt der Exporter automatisch das neueste `*-aggregated.bin` aus `IPFS output`. `neural_network/data/results_iid/aggregated.bin` bleibt nur noch ein Legacy-Fallback.

Fuer EZKL wird das ONNX-Modell standardmaessig mit festem `batch=1` exportiert. Das vermeidet symbolische Batch-Dimensionen, die bei `gen_settings` haeufig zu Fehlern fuehren.

## Ergebnisdateien

Im Zielordner entstehen:

- `model_state_dict_fp64.pt`: PyTorch-Checkpoint mit den Float64-Parametern aus dem Python-Worker
- `model_state_dict.pt`: PyTorch-Checkpoint mit Float32-Parametern fuer ONNX/EZKL
- `model.onnx`: ONNX-Modell mit Softmax-Output
- `model_logits.onnx`: ONNX-Modell mit Logits-Output, bevorzugt fuer EZKL
- `export_manifest.json`: Modellquelle, Layout aus `python_worker`, Parameterstatistiken und Paritaetscheck

## EZKL Input Vorbereiten

Ein einzelnes identifizierbares MNIST-Testbild mit derselben Normalisierung wie im Python-Worker:

```bash
.venv/bin/python zk_inference/create_single_mnist_query.py \
  --images neural_network/data/t10k-images.idx3-ubyte \
  --labels neural_network/data/t10k-labels.idx1-ubyte \
  --out-dir zk_inference/single_query \
  --input-json zk_inference/out/input.json
```

Das erzeugt ein Ein-Bild-Dataset, ein PGM-Preview, Prediction-Metadaten und das EZKL-`input.json`:

- `zk_inference/single_query/single-image.idx3-ubyte`
- `zk_inference/single_query/single-label.idx1-ubyte`
- `zk_inference/single_query/single-image.pgm`
- `zk_inference/single_query/prediction.json`
- `zk_inference/out/input.json`

Das `input.json` hat das Format:

```json
{
  "input_data": [[... 784 normalisierte Werte ...]]
}
```

## EZKL Lauf

Wenn `pip install ezkl` zwar das Python-Modul installiert, aber kein `ezkl`-Binary in `.venv/bin` anlegt, nutzt den Python-Runner:

```bash
.venv/bin/python zk_inference/run_ezkl.py \
  --workdir zk_inference/out \
  --model model_logits.onnx \
  --data input.json
```

Fuer einen schnelleren lokalen Debug-Lauf ohne Kalibrierung:

```bash
.venv/bin/python zk_inference/run_ezkl.py \
  --workdir zk_inference/out \
  --model model_logits.onnx \
  --data input.json \
  --skip-calibration
```

Der Runner fuehrt diese Schritte ueber die Python-API aus:

1. `gen_settings`
2. `calibrate_settings`
3. `compile_circuit`
4. `get_srs` oder lokaler `gen_srs`-Fallback
5. `setup`
6. `gen_witness`
7. `prove`
8. `verify`

## Hinweise

- Das native Modell des Python-Workers gibt Logits zurueck.
- `model_logits.onnx` ist deshalb der sauberste EZKL-Einstieg.
- `model.onnx` existiert nur als zusaetzliche Softmax-Variante fuer Wahrscheinlichkeitsausgaben.
