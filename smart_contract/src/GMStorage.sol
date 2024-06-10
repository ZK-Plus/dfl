// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

interface IDeviceRegistry {
    function isAuthorized(address _address) external view returns (bool);
}

contract GMStorage {
    string public globalModel;
    string public backupGlobalModel;
    address private _device_registry_address =
        0x0aE52af46B2A860ED0EaeC2C41Aa8072C614E7fb;
    mapping(address => uint256) public contributions;
    address[] private contributors;

    constructor() {
        globalModel = "QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8";
        backupGlobalModel = "QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8";
    }

    function setGlobalModel(string memory _newGlobalModel) external {
        require(
            IDeviceRegistry(_device_registry_address).isAuthorized(msg.sender),
            "Caller is not authorized"
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
