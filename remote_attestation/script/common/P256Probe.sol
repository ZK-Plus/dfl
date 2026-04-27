// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

abstract contract P256Probe {
    address internal constant NATIVE_P256_VERIFIER = 0x0000000000000000000000000000000000000100;
    address internal constant FALLBACK_P256_VERIFIER = 0xc2b78104907F722DABAc4C69f826a522B2754De4;

    uint8 internal constant ROUTE_UNAVAILABLE = 0;
    uint8 internal constant ROUTE_NATIVE = 1;
    uint8 internal constant ROUTE_FALLBACK = 2;
    uint8 internal constant ROUTE_CONFIGURED = 3;

    bytes32 internal constant PROBE_HASH = 0xbb5a52f42f9c9261ed4361f59422a1e30036e7c32b270c8807a419feca605023;
    uint256 internal constant PROBE_R = 19738613187745101558623338726804762177711919211234071563652772152683725073944;
    uint256 internal constant PROBE_S = 34753961278895633991577816754222591531863837041401341770838584739693604822390;
    uint256 internal constant PROBE_X = 18614955573315897657680976650685450080931919913269223958732452353593824192568;
    uint256 internal constant PROBE_Y = 90223116347859880166570198725387569567414254547569925327988539833150573990206;

    function _probeVerifier(address verifier) internal view returns (bool supported, bool valid) {
        bytes memory input = abi.encodePacked(PROBE_HASH, PROBE_R, PROBE_S, PROBE_X, PROBE_Y);
        (bool success, bytes memory ret) = verifier.staticcall(input);
        if (!success || ret.length != 32) {
            return (false, false);
        }

        supported = true;
        valid = abi.decode(ret, (uint256)) == 1;
    }

    function _resolveP256Route(address configuredVerifier)
        internal
        view
        returns (uint8 route, bool nativeSupported, bool fallbackSupported, bool configuredSupported)
    {
        bool nativeValid;
        bool fallbackValid;
        bool configuredValid;

        (nativeSupported, nativeValid) = _probeVerifier(NATIVE_P256_VERIFIER);
        (fallbackSupported, fallbackValid) = _probeVerifier(FALLBACK_P256_VERIFIER);
        (configuredSupported, configuredValid) = _probeVerifier(configuredVerifier);

        if (configuredVerifier == NATIVE_P256_VERIFIER) {
            if (nativeSupported && nativeValid) return (ROUTE_NATIVE, nativeSupported, fallbackSupported, configuredSupported);
            if (fallbackSupported && fallbackValid) return (ROUTE_FALLBACK, nativeSupported, fallbackSupported, configuredSupported);
            return (ROUTE_UNAVAILABLE, nativeSupported, fallbackSupported, configuredSupported);
        }

        if (configuredVerifier == FALLBACK_P256_VERIFIER) {
            if (fallbackSupported && fallbackValid) return (ROUTE_FALLBACK, nativeSupported, fallbackSupported, configuredSupported);
            if (nativeSupported && nativeValid) return (ROUTE_NATIVE, nativeSupported, fallbackSupported, configuredSupported);
            return (ROUTE_UNAVAILABLE, nativeSupported, fallbackSupported, configuredSupported);
        }

        if (configuredSupported && configuredValid) {
            return (ROUTE_CONFIGURED, nativeSupported, fallbackSupported, configuredSupported);
        }
        if (fallbackSupported && fallbackValid) {
            return (ROUTE_FALLBACK, nativeSupported, fallbackSupported, configuredSupported);
        }
        if (nativeSupported && nativeValid) {
            return (ROUTE_NATIVE, nativeSupported, fallbackSupported, configuredSupported);
        }
        return (ROUTE_UNAVAILABLE, nativeSupported, fallbackSupported, configuredSupported);
    }

    function _routeLabel(uint8 route) internal pure returns (string memory) {
        if (route == ROUTE_NATIVE) return "native-precompile";
        if (route == ROUTE_FALLBACK) return "fallback-contract";
        if (route == ROUTE_CONFIGURED) return "configured-address";
        return "unavailable";
    }

    function _boolLabel(bool value) internal pure returns (string memory) {
        return value ? "supported" : "unsupported";
    }
}
