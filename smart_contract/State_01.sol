//SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract AggregatorState {
    address public owner;
    string public aggregator_ip;
    mapping(address => string) public ip_table;
    enum FL_State {
        learning,
        aggregation,
        selection,
        error
    }
    FL_State current_state = FL_State.learning;

    constructor() {
        owner = msg.sender;
    }

    function selectAggregator(string memory ip) public {
        require(
            msg.sender == owner,
            "Only contract owner can set first aggregator"
        );
        aggregator_ip = ip;
    }

    function registerAccount(string memory ip) public {
        ip_table[address(msg.sender)] = ip;
    }

    function getAggregatorIP() public view returns (string memory) {
        return aggregator_ip;
    }
}
