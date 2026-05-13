// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.0;

import "../utils/DeploymentConfig.sol";
import "../utils/Multichain.sol";

import {AutomataDaoStorage} from "../../src/automata_pccs/shared/AutomataDaoStorage.sol";
import {AutomataFmspcTcbDao} from "../../src/automata_pccs/AutomataFmspcTcbDao.sol";
import {AutomataEnclaveIdentityDao} from "../../src/automata_pccs/AutomataEnclaveIdentityDao.sol";
import {AutomataPcsDao} from "../../src/automata_pccs/AutomataPcsDao.sol";
import {AutomataPckDao} from "../../src/automata_pccs/AutomataPckDao.sol";

contract ConfigAutomataDao is DeploymentConfig, Multichain {
    uint256 deployerKey = uint256(vm.envBytes32("ETH_WALLET_PRIVATE_KEY"));
    address owner = vm.addr(deployerKey);

    address pccsStorageAddr = readContractAddress("AutomataDaoStorage", true);

    function grantDao(address dao) public {
        vm.broadcast(deployerKey);

        AutomataDaoStorage pccsStorage = AutomataDaoStorage(pccsStorageAddr);
        pccsStorage.grantDao(dao);
    }

    function revokeDao(address dao) public {
        vm.broadcast(deployerKey);

        AutomataDaoStorage(pccsStorageAddr).revokeDao(dao);
    }

    function setAuthorizedCaller(address caller, bool authorized) public multichain {
        AutomataDaoStorage pccsStorage = AutomataDaoStorage(pccsStorageAddr);
        bool authorizedCaller = pccsStorage.isAuthorizedCaller(caller);

        if (authorized != authorizedCaller) {
            vm.broadcast(deployerKey);
            pccsStorage.setCallerAuthorization(caller, authorized);
        } else {
            console.log("Skip setAuthorizedCaller()");
        }
    }
}
