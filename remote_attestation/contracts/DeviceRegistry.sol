// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

interface ITdxV4Attestation {
    function verifyAndAttestOnChain(bytes calldata input) external view returns (bytes memory output);
}

contract DeviceRegistry {
    struct Device {
        bool authorized;
        string public_ip;
        string msg_broker_ip;
        bytes public_key;
    }

    address public owner;
    ITdxV4Attestation public tdxV4Attestation;
    mapping(address => Device) public devices;
    uint256 public number;

    constructor() {
        owner = msg.sender;
        // Add some initial authorized addresses for demonstration
        devices[msg.sender] = Device(true, "test_ip", "", "");
    }

    modifier onlyOwner() {
        require(msg.sender == owner, "not owner");
        _;
    }

    function setTdxV4Attestation(address _tdxV4Attestation) public onlyOwner {
        require(_tdxV4Attestation != address(0), "invalid attestation address");
        tdxV4Attestation = ITdxV4Attestation(_tdxV4Attestation);
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

    function getDevice(
        address _address
    ) external view returns (bool, string memory, string memory, bytes memory) {
        return (
            devices[_address].authorized,
            devices[_address].public_ip,
            devices[_address].msg_broker_ip,
            devices[_address].public_key
        );
    }

    function registerDevice(
        bytes calldata quote,
        address _address,
        string memory _public_ip,
        string memory _msg_broker_ip,
        bytes memory _public_key
    ) public {
        require(address(tdxV4Attestation) != address(0), "tdx attestation not configured");
        tdxV4Attestation.verifyAndAttestOnChain(quote);
        devices[_address] = Device(
            true,
            _public_ip,
            _msg_broker_ip,
            _public_key
        );
    }
}
