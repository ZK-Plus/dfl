// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {Test, console} from "forge-std/Test.sol";
import {GMStorage} from "../src/GMStorage.sol";

contract GMStorageTest is Test {
    GMStorage public gmStorage;

    function setUp() public {
        gmStorage = new GMStorage(
            address(0xBA874f9c3C3a68F1d196f29DFE14847590559E12),
            address(0x7F228C96C9136311DE79dC88026FD4fA4697d638)
        );
    }

    function test_printAddress() public {
        console.log("Address: ", gmStorage.aggregator_selection_address());
    }

    function test_setGlobalModel() public {
        gmStorage.setGlobalModel(
            "QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8"
        );
        assertEq(
            gmStorage.globalModel(),
            "QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8"
        );
    }

    // function test_Increment() public {
    //     counter.increment();
    //     assertEq(counter.number(), 1);
    // }

    // function testFuzz_SetNumber(uint256 x) public {
    //     counter.setNumber(x);
    //     assertEq(counter.number(), x);
    // }
}
