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
    string public globalModelSignature;
    string public backupGlobalModelSignature;
    uint256 public round;
    address public lastRoundAggregator;
    address public device_registry_address;
    address public aggregator_selection_address;
    mapping(address => uint256) public contributions;
    address[] private contributors;

    constructor(
        address _device_registry_address,
        address _aggregator_selection_address,
        string memory _initial_GM_CID,
        string memory _initial_GM_SIG_CID,
        address _initial_GM_SIGNER_ADDRESS
    ) {
        globalModel = _initial_GM_CID;
        backupGlobalModel = _initial_GM_CID;
        globalModelSignature = _initial_GM_SIG_CID;
        backupGlobalModelSignature = _initial_GM_SIG_CID;
        device_registry_address = _device_registry_address;
        aggregator_selection_address = _aggregator_selection_address;
        round = 0;
        lastRoundAggregator = _initial_GM_SIGNER_ADDRESS;
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
        lastRoundAggregator = msg.sender;
    }

    function setGlobalModelSignature(string memory _newGlobalModelSignature)
        external
    {
        // check if caller is the aggregator
        require(
            IAggregatorSelection(aggregator_selection_address).isAggregator(
                msg.sender
            ),
            "Caller is not an aggregator"
        );
        backupGlobalModelSignature = globalModelSignature;
        globalModelSignature = _newGlobalModelSignature;
        lastRoundAggregator = msg.sender;
    }

    function setGlobalModelAndSignature(
        string memory _newGlobalModel,
        string memory _newGlobalModelSignature
    ) external {
        // check if caller is the aggregator
        require(
            IAggregatorSelection(aggregator_selection_address).isAggregator(
                msg.sender
            ),
            "Caller is not an aggregator"
        );
        backupGlobalModel = globalModel;
        backupGlobalModelSignature = globalModelSignature;
        globalModel = _newGlobalModel;
        globalModelSignature = _newGlobalModelSignature;
        lastRoundAggregator = msg.sender;
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

    function incrementRound() external {
        require(
            IAggregatorSelection(aggregator_selection_address).isAggregator(
                msg.sender
            ),
            "Caller is not an aggregator"
        );
        round++;
        lastRoundAggregator = msg.sender;
    }

    function setLastRoundAggregator() external {
        require(
            IAggregatorSelection(aggregator_selection_address).isAggregator(
                msg.sender
            ),
            "Caller is not an aggregator"
        );
        lastRoundAggregator = msg.sender;
    }

    function getRound() external view returns (uint256) {
        return round;
    }

    function getGlobalModel() external view returns (string memory) {
        return globalModel;
    }

    function getGlobalModelSignature() external view returns (string memory) {
        return globalModelSignature;
    }

    function getBackupGlobalModel() external view returns (string memory) {
        return backupGlobalModel;
    }

    function getBackupGlobalModelSignature()
        external
        view
        returns (string memory)
    {
        return backupGlobalModelSignature;
    }

    function getLastRoundsAggregator() external view returns (address) {
        return lastRoundAggregator;
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
