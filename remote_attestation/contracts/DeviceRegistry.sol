// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import {IRiscZeroVerifier} from "risc0/IRiscZeroVerifier.sol";
import {ImageID} from "./ImageID.sol"; // auto-generated contract after running `cargo build`.

contract DeviceRegistry {
    // @notice RISC Zero verifier contract address.
    IRiscZeroVerifier public immutable verifier;
    bytes32 public constant imageId = ImageID.VERIFY_AR_ID;

    struct Device {
        bool authorized;
        string public_ip;
        string msg_broker_ip;
        bytes public_key;
    }

    mapping(address => Device) public devices;
    uint256 public number;

    constructor(IRiscZeroVerifier _verifier) {
        verifier = _verifier;
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

    //test function
    function set(uint256 x, bytes calldata seal) public {
        // Construct the expected journal data. Verify will fail if journal does not match.
        bytes memory journal = abi.encode(x);
        verifier.verify(seal, imageId, sha256(journal));
        number = x;
    }

    function registerDevice(
        uint256 x,
        bytes calldata seal,
        address _address,
        string memory _public_ip,
        string memory _msg_broker_ip,
        bytes memory _public_key
    ) public {
        // construct the journal data and add the device to the registry
        bytes memory journal = abi.encode(x);
        verifier.verify(seal, imageId, sha256(journal));
        devices[_address] = Device(
            true,
            _public_ip,
            _msg_broker_ip,
            _public_key
        );
    }

    function runProof(bytes calldata seal, uint256 x) public {
        bytes memory journal = abi.encode(x);
        verifier.verify(seal, imageId, sha256(journal));
    }

    function registerDeviceWithoutProof(
        address _address,
        string memory _public_ip,
        string memory _msg_broker_ip,
        bytes memory _public_key
    ) public {
        devices[_address] = Device(
            true,
            _public_ip,
            _msg_broker_ip,
            _public_key
        );
    }
}
