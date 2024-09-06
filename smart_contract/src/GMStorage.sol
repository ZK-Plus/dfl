// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

interface IDeviceRegistry {
    function isAuthorized(address _address) external view returns (bool);
}

interface IAggregatorSelection {
    function isAggregator(address _address) external view returns (bool);
}

contract GMStorage {
    string public globalModel;
    string public backupGlobalModel;
    address public device_registry_address;
    address public aggregator_selection_address;
    mapping(address => uint256) public contributions;
    address[] private contributors;

    constructor(
        address _device_registry_address,
        address _aggregator_selection_address
    ) {
        globalModel = "QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8";
        backupGlobalModel = "QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8";
        device_registry_address = _device_registry_address;
        aggregator_selection_address = _aggregator_selection_address;
    }

    function setGlobalModel(string memory _newGlobalModel) external {
        // check if caller is the aggregator
        require(
            IAggregatorSelection(aggregator_selection_address).isAggregator(
                msg.sender
            ),
            "Caller is not an aggregator"
        );
        backupGlobalModel = globalModel;
        globalModel = _newGlobalModel;
    }

    function incrementContribution(address[] memory _addresses) external {
        for (uint256 i = 0; i < _addresses.length; i++) {
            if (
                contributions[_addresses[i]] == 0 &&
                !isContributor(_addresses[i])
            ) {
                contributors.push(_addresses[i]);
            }
            contributions[_addresses[i]]++;
        }
    }

    function decrementContribution(address[] memory _addresses) external {
        for (uint256 i = 0; i < _addresses.length; i++) {
            contributions[_addresses[i]]--;
        }
    }

    function isContributor(address _address) internal view returns (bool) {
        for (uint256 i = 0; i < contributors.length; i++) {
            if (contributors[i] == _address) {
                return true;
            }
        }
        return false;
    }

    function getGlobalModel() external view returns (string memory) {
        return globalModel;
    }

    function getBackupGlobalModel() external view returns (string memory) {
        return backupGlobalModel;
    }

    function getContribution(address _address) external view returns (uint256) {
        return contributions[_address];
    }

    function getTopContributor() external view returns (address) {
        address topContributor;
        uint256 topScore = 0;
        for (uint256 i = 0; i < contributors.length; i++) {
            if (contributions[contributors[i]] > topScore) {
                topScore = contributions[contributors[i]];
                topContributor = contributors[i];
            }
        }
        return topContributor;
    }
}
