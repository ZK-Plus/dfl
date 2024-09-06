// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

contract DeviceRegistry {
    struct Device {
        bool authorized;
        string public_ip;
        string msg_broker_ip;
        string public_key;
    }

    mapping(address => Device) public devices;

    constructor() {
        // Add some initial authorized addresses for demonstration
        devices[msg.sender] = Device(true, "test_ip", "", "");
    }

    function authorizeAddress(address _address) public {
        devices[_address].authorized = true;
    }

    function deauthorizeAddress(address _address) public {
        devices[_address].authorized = false;
    }

    function isAuthorized(address _address) public view returns (bool) {
        if (devices[_address].authorized) {
            return true;
        } else {
            return false;
        }
    }

    function registerDevice(
        address _address,
        string memory _public_ip,
        string memory _msg_broker_ip,
        string memory _public_key
    ) public {
        devices[_address] = Device(
            true,
            _public_ip,
            _msg_broker_ip,
            _public_key
        );
    }
}
