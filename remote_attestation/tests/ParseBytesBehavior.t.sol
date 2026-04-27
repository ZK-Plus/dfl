// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

import {Test} from "forge-std/Test.sol";

contract ParseBytesBehaviorTest is Test {
    function test_parseBytes_decodesHexString() external view {
        bytes memory parsed = vm.parseBytes("0x3082");
        assertEq(parsed, hex"3082");
    }
}
