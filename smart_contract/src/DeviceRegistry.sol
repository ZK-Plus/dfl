// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

contract DeviceRegistry {
    struct Device {
        bool authorized;
        string public_ip;
        string msg_broker_ip;
        string public_key;
    }

    address public owner;
    mapping(address => Device) public devices;
    mapping(address => bool) private knownDevice;
    address[] private deviceAddresses;

    event DeviceRegistered(address indexed device);
    event DeviceAuthorized(address indexed device);
    event DeviceDeauthorized(address indexed device);
    event DeviceLeft(address indexed device);

    constructor() {
        owner = msg.sender;
        // Add some initial authorized addresses for demonstration
        devices[msg.sender] = Device(true, "test_ip", "", "");
        addKnownDevice(msg.sender);
    }

    modifier onlyOwner() {
        require(msg.sender == owner, "not owner");
        _;
    }

    function authorizeAddress(address _address) public onlyOwner {
        addKnownDevice(_address);
        devices[_address].authorized = true;
        emit DeviceAuthorized(_address);
    }

    function deauthorizeAddress(address _address) public onlyOwner {
        devices[_address].authorized = false;
        emit DeviceDeauthorized(_address);
    }

    function leaveNetwork() public {
        require(devices[msg.sender].authorized, "device not authorized");
        devices[msg.sender].authorized = false;
        emit DeviceLeft(msg.sender);
    }

    function isAuthorized(address _address) public view returns (bool) {
        if (devices[_address].authorized) {
            return true;
        } else {
            return false;
        }
    }

    function getAuthorizedDevices() external view returns (address[] memory) {
        uint256 count = 0;
        for (uint256 i = 0; i < deviceAddresses.length; i++) {
            if (devices[deviceAddresses[i]].authorized) {
                count++;
            }
        }

        address[] memory authorized = new address[](count);
        uint256 index = 0;
        for (uint256 i = 0; i < deviceAddresses.length; i++) {
            address device = deviceAddresses[i];
            if (devices[device].authorized) {
                authorized[index] = device;
                index++;
            }
        }
        return authorized;
    }

    function registerDevice(
        address _address,
        string memory _public_ip,
        string memory _msg_broker_ip,
        string memory _public_key
    ) public {
        addKnownDevice(_address);
        devices[_address] = Device(
            true,
            _public_ip,
            _msg_broker_ip,
            _public_key
        );
        emit DeviceRegistered(_address);
    }

    function addKnownDevice(address _address) internal {
        if (!knownDevice[_address]) {
            knownDevice[_address] = true;
            deviceAddresses.push(_address);
        }
    }
}
