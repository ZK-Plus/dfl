// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

import {Test} from "forge-std/Test.sol";
import {AutomataDcapTdxV4Attestation} from "../contracts/AutomataDcapTdxV4Attestation.sol";
import {V4Parser} from "../contracts/tdx/QuoteV4Auth/V4Parser.sol";
import {V4Struct} from "../contracts/tdx/QuoteV4Auth/V4Struct.sol";

contract TDXV4GasTest is Test {
    AutomataDcapTdxV4Attestation internal verifier;

    function setUp() external {
        vm.createSelectFork(vm.envString("RPC_URL"));
        verifier = AutomataDcapTdxV4Attestation(vm.envAddress("DCAP_TDX_V4_ADDRESS"));
    }

    function test_gas_verifyAndAttestOnChain_rawQuote() external {
        vm.pauseGasMetering();
        string memory path = string.concat(vm.projectRoot(), "/apps/data/phala_tdx_quote");
        bytes memory quote = vm.parseBytes(string.concat("0x", vm.readFile(path)));
        vm.resumeGasMetering();

        bytes memory output = verifier.verifyAndAttestOnChain(quote);
        assertGt(output.length, 0);
    }

    function test_gas_verifyParsedQuoteAndAttestOnChain() external {
        vm.pauseGasMetering();
        string memory path = string.concat(vm.projectRoot(), "/apps/data/phala_tdx_quote");
        bytes memory quote = vm.parseBytes(string.concat("0x", vm.readFile(path)));
        (bool success, V4Struct.ParsedV4Quote memory parsedQuote) = V4Parser.parseInput(quote);
        vm.resumeGasMetering();

        assertTrue(success);

        bytes memory output = verifier.verifyParsedQuoteAndAttestOnChain(parsedQuote);
        assertGt(output.length, 0);
    }
}
