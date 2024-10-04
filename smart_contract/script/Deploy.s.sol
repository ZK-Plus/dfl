// Copyright 2024 RISC Zero, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

pragma solidity ^0.8.20;

import {Script} from "forge-std/Script.sol";
import {console2} from "forge-std/console2.sol";
import {AggregatorSelection} from "../src/AggregatorSelection.sol";
import {GMStorage} from "../src/GMStorage.sol";

/// @notice Deployment script for the RISC Zero starter project.
/// @dev Use the following environment variable to control the deployment:
///     * ETH_WALLET_PRIVATE_KEY private key of the wallet to be used for deployment.
///

contract ContractDeploy is Script {
    function run() external {
        uint256 deployerKey = uint256(vm.envBytes32("ETH_WALLET_PRIVATE_KEY"));

        vm.startBroadcast(deployerKey);

        AggregatorSelection aggregatorSelection = new AggregatorSelection(
            address(0x607B2b2229eba34075fEEEF2E43AC96481133A58)
        );
        console2.log(
            "Deployed AggregatorSelection to",
            address(aggregatorSelection)
        );

        GMStorage gmStorage = new GMStorage(
            address(0xBA874f9c3C3a68F1d196f29DFE14847590559E12),
            address(aggregatorSelection)
        );
        console2.log("Deployed GMStorage to", address(gmStorage));

        vm.stopBroadcast();
    }
}
