// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {Test, console} from "forge-std/Test.sol";
import {AggregatorSelection} from "../src/AggregatorSelection.sol";

contract AggregatorTest is Test {
    AggregatorSelection public aggregatorSelection;

    function setUp() public {
        aggregatorSelection = new AggregatorSelection();
    }

    function test_isAggregator() public {
        assert(
            aggregatorSelection.isAggregator(
                //address(0x927507b066B40C5a6b0fd327eFE1c9d33edeE759)
                address(0x6004D395731f85938177d5B07aCfEc2D5D7ECcB0)
            )
        );
    }

    function test_isNotAggregator() public {
        assert(
            aggregatorSelection.isAggregator(
                address(0x117264201970AFb089c8238e0320DdAA8E202b3B)
            )
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
