// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

interface IDeviceRegistry {
    function isAuthorized(address _address) external view returns (bool);
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

    constructor() {
        system_state = "TRAINING";
        current_aggregator = address(
            0x927507b066B40C5a6b0fd327eFE1c9d33edeE759
        );
        broker_endpoint = "test_endpoint";
        time_to_aggregate = 0;
        time_to_select = 0;
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

    function getBrokerEndpoint() external view returns (string memory) {
        return broker_endpoint;
    }

    function getTimeToAggregate() external view returns (uint256) {
        return time_to_aggregate;
    }

    function getTimeToSelect() external view returns (uint) {
        return time_to_select;
    }

    function setCurrentAggregator(address _aggregator) external {
        current_aggregator = _aggregator;
    }

    function setBrokerEndpoint(string memory _endpoint) external {
        broker_endpoint = _endpoint;
    }

    //change system state method
}
