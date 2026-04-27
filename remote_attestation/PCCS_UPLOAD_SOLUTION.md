# PCCS Collaterals Upload - Lösung für Zugriffsfehler

## Problem
Beim Versuch, eine TEe Quote auf der Blockchain zu verifizieren, trat ein Zugriffsfehler auf:
```
vm.readFileBinary: the path assets/0624/root_ca.der is not allowed to be accessed for read operations
```

## Lösung

### 1. Foundry-Konfiguration aktualisiert
Die `foundry.toml` wurde angepasst, um den Zugriff auf die Zertifikatsdateien zu ermöglichen:
```toml
allow_paths = [
    "../smart_contract/src", 
    "./tmp", 
    "./lib/automata-dcap-v3-attestation/assets",
    "./lib/automata-dcap-v3-attestation/assets/0624",
    "./lib",
    "assets",
    "assets/0624"
]
```

### 2. Skript angepasst
Das `UploadPccsCollaterals.s.sol` Skript wurde so angepasst, dass es Zertifikate **nicht mehr aus Dateien liest**, sondern **aus Environment-Variablen** erwartet.

### 3. Hilfs-Skript erstellt
Ein neues Skript `prepare_pccs_env.sh` wurde erstellt, das die Zertifikatsdateien in Base64-Strings konvertiert.

## Verwendung

### Schritt 1: Zertifikate vorbereiten
```bash
cd remote_attestation
chmod +x prepare_pccs_env.sh
./prepare_pccs_env.sh > pccs_env_vars.sh
```

### Schritt 2: Environment-Variablen setzen
```bash
source pccs_env_vars.sh
```

### Schritt 3: Upload-Skript ausführen
```bash
export ETH_WALLET_PRIVATE_KEY=0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
export PCS_DAO=0x0000000000000000000000000000000000000000
export ENCLAVE_ID_DAO=0x0000000000000000000000000000000000000000
export FMSPC_TCB_DAO=0x0000000000000000000000000000000000000000
export ENCLAVE_IDENTITY_HELPER=0x0000000000000000000000000000000000000000

forge script script/UploadPccsCollaterals.s.sol --rpc-url http://localhost:8545 --broadcast --ffi -vvv
```

## Alternative: Direkte Verwendung im Docker-Container

Wenn Sie den Container verwenden, können Sie die Environment-Variablen direkt setzen:

```bash
docker exec -it smart-contracts bash

# Inside container
export PCCS_IDENTITY_JSON=$(cat /dfl/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624/identity.json)
export PCCS_TCBINFO_JSON=$(cat /dfl/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624/tcbinfo.json)
export PCCS_ROOT_CA_DER=$(base64 -w 0 /dfl/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624/root_ca.der)
export PCCS_TCB_SIGNING_DER=$(base64 -w 0 /dfl/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624/tcb_signing.der)
export PCCS_PLATFORM_CA_DER=$(base64 -w 0 /dfl/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624/platform_ca.der)
export PCCS_PLATFORM_CRL_DER=$(base64 -w 0 /dfl/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624/pckcrl.der)
export PCCS_ROOT_CRL_DER=$(base64 -w 0 /dfl/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624/rootcacrl.der)

# Then run the script
forge script script/UploadPccsCollaterals.s.sol --rpc-url http://localhost:8545 --broadcast --ffi -vvv
```

## Wichtige Hinweise

1. **Base64-Format**: Die Zertifikate werden als Base64-Strings in Environment-Variablen übergeben
2. **FFI erforderlich**: Das `--ffi` Flag ist weiterhin erforderlich, um Environment-Variablen zu verwenden
3. **Sicherheitsvorteil**: Durch die Verwendung von Environment-Variablen wird der Dateizugriff vermieden, was sicherer ist

## Fehlerbehebung

### Wenn die Zertifikatsdateien nicht gefunden werden:
Stellen Sie sicher, dass das Verzeichnis `lib/automata-dcap-v3-attestation/assets/0624/` existiert und die folgenden Dateien enthält:
- `identity.json`
- `tcbinfo.json`
- `root_ca.der`
- `tcb_signing.der`
- `platform_ca.der`
- `pckcrl.der`
- `rootcacrl.der`

### Wenn Base64-Encoding Probleme auftreten:
Verwenden Sie auf macOS: `base64 -w 0`
Verwenden Sie auf Linux: `base64 -w 0` oder `base64 | tr -d '\n'`
