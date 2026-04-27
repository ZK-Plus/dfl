// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

import {Script} from "forge-std/Script.sol";
import {console2} from "forge-std/console2.sol";
import {AutomataDcapTdxV4Attestation} from "../contracts/AutomataDcapTdxV4Attestation.sol";

contract DeployTDXV4Attestation is Script {
    address internal constant DEFAULT_P256_VERIFIER = 0xc2b78104907F722DABAc4C69f826a522B2754De4;

    function run() external {
        uint256 deployerKey = uint256(vm.envBytes32("ETH_WALLET_PRIVATE_KEY"));

        address enclaveIdDaoAddr = vm.envAddress("ENCLAVE_ID_DAO");
        address pckHelperAddr = vm.envAddress("X509_HELPER");
        address tcbDaoAddr = vm.envAddress("FMSPC_TCB_DAO");
        address crlHelperAddr = vm.envAddress("X509_CRL_HELPER");
        address pcsDaoAddr = vm.envAddress("PCS_DAO");
        address p256VerifierAddr = _envOrAddress("P256_VERIFIER_ADDRESS", DEFAULT_P256_VERIFIER);

        vm.startBroadcast(deployerKey);
        AutomataDcapTdxV4Attestation attestation = new AutomataDcapTdxV4Attestation(
            enclaveIdDaoAddr,
            pckHelperAddr,
            tcbDaoAddr,
            crlHelperAddr,
            pcsDaoAddr,
            p256VerifierAddr
        );
        vm.stopBroadcast();

        console2.log("AutomataDcapTdxV4Attestation:", address(attestation));
    }

    function _envOrAddress(string memory key, address defaultValue) private view returns (address value) {
        try vm.envAddress(key) returns (address configured) {
            return configured;
        } catch {
            return defaultValue;
        }
    }
}
