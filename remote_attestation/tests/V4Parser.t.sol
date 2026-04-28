// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

import {Test} from "forge-std/Test.sol";
import {V4Parser} from "../contracts/tdx/QuoteV4Auth/V4Parser.sol";
import {V4Struct} from "../contracts/tdx/QuoteV4Auth/V4Struct.sol";

contract V4ParserTest is Test {
    function test_parseInput_acceptsZeroPaddedQuote() external view {
        string memory path = string.concat(vm.projectRoot(), "/apps/data/phala_tdx_quote");
        bytes memory quote = vm.parseBytes(string.concat("0x", vm.readFile(path)));

        (bool success, V4Struct.ParsedV4Quote memory parsedQuote) = V4Parser.parseInput(quote);

        assertTrue(success);
        assertEq(parsedQuote.header.version, bytes2(0x0400));
        assertEq(parsedQuote.header.teeType, bytes4(0x81000000));
        assertEq(parsedQuote.quoteSignature.length, 64);
        assertEq(parsedQuote.attestationKey.length, 64);
        assertEq(parsedQuote.rawQeReport.length, 384);
        assertEq(parsedQuote.qeReportCertificationData.certification.decodedCertDataArray.length, 3);
    }

    function test_validateParsedInput_acceptsValidParsedQuote() external view {
        string memory path = string.concat(vm.projectRoot(), "/apps/data/phala_tdx_quote");
        bytes memory quote = vm.parseBytes(string.concat("0x", vm.readFile(path)));

        (bool success, V4Struct.ParsedV4Quote memory parsedQuote) = V4Parser.parseInput(quote);

        assertTrue(success);
        V4Parser.validateParsedInput(parsedQuote);
    }

    function test_validateParsedInput_benchmark() external {
        vm.pauseGasMetering();
        string memory path = string.concat(vm.projectRoot(), "/apps/data/phala_tdx_quote");
        bytes memory quote = vm.parseBytes(string.concat("0x", vm.readFile(path)));
        (bool success, V4Struct.ParsedV4Quote memory parsedQuote) = V4Parser.parseInput(quote);
        vm.resumeGasMetering();

        assertTrue(success);
        V4Parser.validateParsedInput(parsedQuote);
    }
}
