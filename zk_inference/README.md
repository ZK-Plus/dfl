# zk_inference

Dieses Verzeichnis ist der Einstiegspunkt, um das in eurem Federated-Learning-Flow erzeugte `aggregated.bin` in einen zk-freundlichen Inferenzpfad zu ueberfuehren.

## Ziel

Der aktuelle Trainings- und Verteilpfad bleibt wie gehabt:

- Training und Aggregation in `neural_network/`
- Upload des globalen Modells nach IPFS in `node_server/`
- Referenzierung ueber den Smart Contract

Neu dazu kommt hier ein Exportpfad:

`aggregated.bin -> PyTorch state_dict -> ONNX -> EZKL`

## Warum der Zwischenschritt noetig ist

Euer Modell liegt momentan als roher Binary-Stream aus sechs Matrizen vor:

1. `W1` mit Form `200 x 784`
2. `B1` mit Form `200 x 1`
3. `W2` mit Form `50 x 200`
4. `B2` mit Form `50 x 1`
5. `W3` mit Form `10 x 50`
6. `B3` mit Form `10 x 1`

Die Werte werden in `neural_network/src/functions.cpp` durch `save()` spaltenweise als `double` geschrieben. Genau dieses Layout liest `export_model.py` wieder ein.

## Voraussetzungen

Mindestens:

```bash
pip install torch numpy
```

Fuer ONNX-Export zusaetzlich:

```bash
pip install onnx onnxscript
```

## Nutzung

Modell inspizieren:

```bash
python3 zk_inference/export_model.py --model "IPFS output/2026-04-28T17-17-32-699Z-aggregated.bin" --inspect
```

PyTorch- und ONNX-Artefakte erzeugen:

```bash
python3 zk_inference/export_model.py \
  --model "IPFS output/2026-04-28T17-17-32-699Z-aggregated.bin" \
  --out zk_inference/out
```

Fuer EZKL wird das ONNX-Modell standardmaessig mit festem `batch=1` exportiert. Das ist absichtlich so, weil symbolische Batch-Dimensionen bei `gen_settings` haeufig zu Fehlern wie `Undetermined symbol in expression` fuehren.

Nur PyTorch exportieren:

```bash
python3 zk_inference/export_model.py \
  --model "IPFS output/2026-04-28T17-17-32-699Z-aggregated.bin" \
  --out zk_inference/out \
  --no-onnx
```

## Ergebnisdateien

Im Zielordner entstehen:

- `weights_fp64.npz`: Gewichte im originalen Float64-Format
- `model_state_dict.pt`: PyTorch-Checkpoint
- `model.onnx`: ONNX-Modell mit Softmax-Output
- `model_logits.onnx`: ONNX-Modell mit Logits-Output, meist besser fuer EZKL
- `export_manifest.json`: Layout, Formen und Paritaetscheck

## Wichtige fachliche Hinweise fuer EZKL

- Hidden Activations sind aktuell `tanh`.
- Der Output nutzt `softmax`.
- Fuer ZK-Proofs ist es oft einfacher, `logits` oder `argmax` statt kompletter `softmax`-Wahrscheinlichkeiten zu beweisen.
- Wenn ihr spaeter auf bessere Proving-Performance gehen wollt, ist ein Re-Training mit `ReLU` meist der naechste sinnvolle Schritt.

## EZKL Input vorbereiten

Ein einzelnes MNIST-Testbild im exakt gleichen Normalisierungsformat wie euer C++-Code laesst sich so in `input.json` umwandeln:

```bash
python3 zk_inference/prepare_mnist_input.py \
  --images neural_network/data/t10k-images.idx3-ubyte \
  --index 0 \
  --out zk_inference/out/input.json
```

Die Ausgabe hat das Format:

```json
{
  "input_data": [[... 784 normalisierte Werte ...]]
}
```

## Nächste sinnvolle Reihenfolge

1. `model_logits.onnx` fuer EZKL verwenden.
2. Mit `prepare_mnist_input.py` ein `input.json` erzeugen.
3. Danach die EZKL-Setup-Artefakte aufbauen: `settings.json`, `network.ezkl`, `vk.key`, `pk.key`, `kzg.srs`.

## Naechster Schritt

Sobald der Export stabil laeuft, wuerde ich als naechstes ein kleines EZKL-Setup-Script ergaenzen:

1. `gen-settings`
2. `calibrate-settings`
3. `compile-model`
4. `setup`
5. `gen-witness`
6. `prove`
7. `verify`

Danach kann man den Verifier-Contract an euren bestehenden Blockchain-Koordinator anbinden.

## EZKL ohne Shell-CLI

Wenn `pip install ezkl` zwar das Python-Modul installiert, aber kein `ezkl`-Binary in `.venv/bin` anlegt, nutzt ihr einfach den Python-Runner:

```bash
.venv/bin/python zk_inference/run_ezkl.py \
  --workdir zk_inference/out \
  --model model_logits.onnx \
  --data input.json
```

Fuer einen ersten schnelleren Debug-Lauf ohne Kalibrierung:

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
4. `get_srs`
5. `setup`
6. `gen_witness`
7. `prove`
8. `verify`
