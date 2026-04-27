#!/bin/bash

# Script to prepare environment variables for UploadPccsCollaterals.s.sol
# Usage: ./prepare_pccs_env.sh > pccs_env_vars.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASSETS_DIR="/Users/sonak/uni/masterthesis/Master-Thesis/remote_attestation/lib/automata-dcap-v3-attestation/assets/0624"

# Check if assets directory exists
if [ ! -d "$ASSETS_DIR" ]; then
    echo "ERROR: Assets directory not found: $ASSETS_DIR" >&2
    exit 1
fi

# Function to convert file to a single-line 0x-prefixed hex string
convert_to_hex() {
    local file=$1

    if [ ! -f "$file" ]; then
        return 1
    fi

    if command -v xxd >/dev/null 2>&1; then
        printf '0x'
        xxd -p -c 999999 "$file" | tr -d '\n'
    elif command -v od >/dev/null 2>&1; then
        printf '0x'
        od -An -v -tx1 "$file" | tr -d ' \n'
    else
        echo "ERROR: neither xxd nor od is available for hex encoding" >&2
        return 1
    fi
}

emit_file_as_base64_export() {
    local file=$1
    local var_name=$2

    if ! encoded=$(convert_to_hex "$file"); then
        echo "WARNING: File not found: $file" >&2
        echo "export $var_name=''"
        return
    fi

    echo "export $var_name='$encoded'"
}

# Identity JSON
if [ -f "$ASSETS_DIR/identity.json" ]; then
    identity_json=$(cat "$ASSETS_DIR/identity.json")
    echo "export PCCS_IDENTITY_JSON='$identity_json'"
else
    echo "WARNING: identity.json not found" >&2
    echo "export PCCS_IDENTITY_JSON='{}'"
fi

# TCB Info JSON
if [ -f "$ASSETS_DIR/tcbinfo.json" ]; then
    tcbinfo_json=$(cat "$ASSETS_DIR/tcbinfo.json")
    echo "export PCCS_TCBINFO_JSON='$tcbinfo_json'"
else
    echo "WARNING: tcbinfo.json not found" >&2
    echo "export PCCS_TCBINFO_JSON='{}'"
fi

emit_file_as_base64_export "$ASSETS_DIR/root_ca.der" "PCCS_ROOT_CA_DER"
emit_file_as_base64_export "$ASSETS_DIR/tcb_signing.der" "PCCS_TCB_SIGNING_DER"
emit_file_as_base64_export "$ASSETS_DIR/platform_ca.der" "PCCS_PLATFORM_CA_DER"
emit_file_as_base64_export "$ASSETS_DIR/pckcrl.der" "PCCS_PLATFORM_CRL_DER"
emit_file_as_base64_export "$ASSETS_DIR/rootcacrl.der" "PCCS_ROOT_CRL_DER"
echo "export PCCS_ROOT_CA_PATH='$ASSETS_DIR/root_ca.der'"
echo "export PCCS_TCB_SIGNING_PATH='$ASSETS_DIR/tcb_signing.der'"
echo "export PCCS_PLATFORM_CA_PATH='$ASSETS_DIR/platform_ca.der'"
echo "export PCCS_PLATFORM_CRL_PATH='$ASSETS_DIR/pckcrl.der'"
echo "export PCCS_ROOT_CRL_PATH='$ASSETS_DIR/rootcacrl.der'"
