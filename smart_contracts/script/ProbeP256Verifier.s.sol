// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

import {Script} from "forge-std/Script.sol";
import {console2} from "forge-std/console2.sol";
import {P256Probe} from "./common/P256Probe.sol";

contract ProbeP256Verifier is Script, P256Probe {
    address internal constant DEFAULT_P256_VERIFIER = 0xc2b78104907F722DABAc4C69f826a522B2754De4;

    function run() external view {
        address configuredVerifier = _envOrAddress("P256_VERIFIER_ADDRESS", DEFAULT_P256_VERIFIER);
        (uint8 route, bool nativeSupported, bool fallbackSupported, bool configuredSupported) =
            _resolveP256Route(configuredVerifier);

        console2.log("P256 configured verifier:", configuredVerifier);
        console2.log("P256 configured probe:", _boolLabel(configuredSupported));
        console2.log("P256 native probe:", _boolLabel(nativeSupported));
        console2.log("P256 fallback probe:", _boolLabel(fallbackSupported));
        console2.log("P256 effective route:", _routeLabel(route));
    }

    function _envOrAddress(string memory key, address defaultValue) private view returns (address value) {
        try vm.envAddress(key) returns (address configured) {
            return configured;
        } catch {
            return defaultValue;
        }
    }
}
