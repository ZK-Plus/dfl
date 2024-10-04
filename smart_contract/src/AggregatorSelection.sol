// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

interface IDeviceRegistry {
    function isAuthorized(address _address) external view returns (bool);
    function getDevice(
        address _address
    ) external view returns (bool, string memory, string memory, bytes memory);
}

interface IGMStorage {
    function getTopContributor() external view returns (address);
}

contract AggregatorSelection {
    enum SystemState {
        TRAINING,
        AGGREGATING,
        SELECTING
    }
    string public system_state;
    address public current_aggregator;
    string public broker_endpoint;
    uint256 public time_to_aggregate;
    uint public time_to_select;
    address public gm_storage_address;

    constructor(address _gm_storage_address) {
        system_state = "TRAINING";
        current_aggregator = address(
            0x927507b066B40C5a6b0fd327eFE1c9d33edeE759
        );
        broker_endpoint = "test_endpoint";
        time_to_aggregate = 0;
        time_to_select = 0;
        gm_storage_address = _gm_storage_address;
    }

    // view functions
    function getSystemState()
        external
        view
        returns (string memory, address, string memory, uint256, uint)
    {
        return (
            system_state,
            current_aggregator,
            broker_endpoint,
            time_to_aggregate,
            time_to_select
        );
    }

    function getCurrentAggregator() external view returns (address) {
        return current_aggregator;
    }

    function isAggregator(address _address) external view returns (bool) {
        return _address == current_aggregator;
    }

    function getBrokerEndpoint() external view returns (string memory) {
        return broker_endpoint;
    }

    function getTimeToAggregate() external view returns (uint256) {
        return time_to_aggregate;
    }

    function getTimeToSelect() external view returns (uint) {
        return time_to_select;
    }

    function setSystemState(string memory _state) external {
        // check if requester is current aggregator
        require(
            msg.sender == current_aggregator,
            "Caller is not the current aggregator"
        );
        system_state = _state;
    }

    function setCurrentAggregator(address _aggregator) external {
        current_aggregator = _aggregator;
    }

    function setBrokerEndpoint(string memory _endpoint) external {
        broker_endpoint = _endpoint;
    }

    function triggerAggregatorSelection() external {
        uint256 current_timestamp = block.timestamp;
        // check if requester is current aggregator
        require(
            msg.sender == current_aggregator ||
                current_timestamp >= time_to_aggregate,
            "Caller is not the current aggregator or the time to aggregate has not been reached"
        );
        // set system state to SELECTING
        system_state = "SELECTING";

        // get top contributor
        IGMStorage gm_storage = IGMStorage(gm_storage_address);
        address top_contributor = gm_storage.getTopContributor();

        // set new aggregator
        current_aggregator = top_contributor;
        broker_endpoint = "new_endpoint";
        system_state = "TRAINING";
    }
}
