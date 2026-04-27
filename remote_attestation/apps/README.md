# Usage

Run the Anvil Blockchain Emulation with:

```
ocker compose -f docker/compose_master.yml pull anvil
docker compose -f docker/compose_master.yml build --no-cache smart-contracts
docker compose -f docker/compose_master.yml down
ANVIL_HARDFORK=osaka \
P256_MODE=native \
PCCS_FMSPC=20A06F000000 \
PCCS_FETCH=1 \
PCCS_TEE=tdx \
DEPLOY_TDX_V4_DCAP=1 \
VERIFY_TDX_QUOTE_ONCHAIN=1 \
KEEP_ALIVE=1 \
docker compose -f docker/compose_master.yml up --force-recreate
```
Extract the results from the logs:

```
docker logs smart-contracts | grep -E "P256 configured verifier|P256 configured probe|P256 native probe|P256 fallback probe|P256 effective route|TDX V4 verification succeeded|PCCS Collaterals uploaded successfully"
```