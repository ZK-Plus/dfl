// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

import {Script} from "forge-std/Script.sol";
import "forge-std/StdJson.sol";
import "solady/utils/JSONParserLib.sol";
import "solady/utils/LibString.sol";

import {
    EnclaveIdentityJsonObj,
    EnclaveIdentityHelper,
    IdentityObj
} from "@automata-network/on-chain-pccs/helpers/EnclaveIdentityHelper.sol";
import {TcbInfoJsonObj} from "@automata-network/on-chain-pccs/helpers/FmspcTcbHelper.sol";
import {CA} from "@automata-network/on-chain-pccs/Common.sol";

import {AutomataPcsDao} from "@automata-network/on-chain-pccs/automata_pccs/AutomataPcsDao.sol";
import {AutomataEnclaveIdentityDao} from "@automata-network/on-chain-pccs/automata_pccs/AutomataEnclaveIdentityDao.sol";
import {AutomataFmspcTcbDao} from "@automata-network/on-chain-pccs/automata_pccs/AutomataFmspcTcbDao.sol";

contract UploadPccsCollaterals is Script {
    using JSONParserLib for JSONParserLib.Item;
    using LibString for string;

    function run() external {
        uint256 deployerKey = uint256(vm.envBytes32("ETH_WALLET_PRIVATE_KEY"));

        address pcsDaoAddr = vm.envAddress("PCS_DAO");
        address enclaveIdDaoAddr = vm.envAddress("ENCLAVE_ID_DAO");
        address fmspcTcbDaoAddr = vm.envAddress("FMSPC_TCB_DAO");
        address enclaveIdHelperAddr = vm.envAddress("ENCLAVE_IDENTITY_HELPER");

        string memory identityJsonStr = vm.envString("PCCS_IDENTITY_JSON");
        EnclaveIdentityJsonObj memory identityJson = _parseIdentityJson(identityJsonStr);

        string memory tcbInfoJsonStr = vm.envString("PCCS_TCBINFO_JSON");
        TcbInfoJsonObj memory tcbInfoJson = _parseTcbInfoJson(tcbInfoJsonStr);

        bytes memory rootCaDer = _loadBinary("PCCS_ROOT_CA_PATH", "PCCS_ROOT_CA_DER");
        bytes memory tcbSigningDer = _loadBinary("PCCS_TCB_SIGNING_PATH", "PCCS_TCB_SIGNING_DER");
        bytes memory platformCaDer = _loadBinary("PCCS_PLATFORM_CA_PATH", "PCCS_PLATFORM_CA_DER");
        bytes memory platformCrlDer = _loadBinary("PCCS_PLATFORM_CRL_PATH", "PCCS_PLATFORM_CRL_DER");
        bytes memory rootCrlDer = _loadBinary("PCCS_ROOT_CRL_PATH", "PCCS_ROOT_CRL_DER");
        uint256 quoteVersion = vm.envOr("PCCS_QUOTE_VERSION", uint256(4));

        vm.startBroadcast(deployerKey);

        AutomataPcsDao pcsDao = AutomataPcsDao(pcsDaoAddr);
        pcsDao.upsertPcsCertificates(CA.ROOT, rootCaDer);
        pcsDao.upsertPcsCertificates(CA.SIGNING, tcbSigningDer);
        pcsDao.upsertPcsCertificates(CA.PLATFORM, platformCaDer);
        pcsDao.upsertPckCrl(CA.PLATFORM, platformCrlDer);
        pcsDao.upsertRootCACrl(rootCrlDer);

        IdentityObj memory identity = EnclaveIdentityHelper(enclaveIdHelperAddr).parseIdentityString(
            identityJson.identityStr
        );

        AutomataEnclaveIdentityDao(enclaveIdDaoAddr).upsertEnclaveIdentity(
            uint256(identity.id),
            quoteVersion,
            identityJson
        );

        AutomataFmspcTcbDao(fmspcTcbDaoAddr).upsertFmspcTcb(tcbInfoJson);

        vm.stopBroadcast();
    }

    function _parseIdentityJson(string memory identityJsonStr)
        private
        pure
        returns (EnclaveIdentityJsonObj memory identityJson)
    {
        JSONParserLib.Item memory root = JSONParserLib.parse(identityJsonStr);
        JSONParserLib.Item[] memory idObj = root.children();
        for (uint256 i = 0; i < root.size(); i++) {
            JSONParserLib.Item memory current = idObj[i];
            string memory decodedKey = JSONParserLib.decodeString(current.key());
            if (decodedKey.eq("enclaveIdentity")) {
                identityJson.identityStr = current.value();
            }
        }

        identityJson.signature = stdJson.readBytes(identityJsonStr, ".signature");
    }

    function _parseTcbInfoJson(string memory tcbInfoJsonStr)
        private
        pure
        returns (TcbInfoJsonObj memory tcbInfoJson)
    {
        JSONParserLib.Item memory root = JSONParserLib.parse(tcbInfoJsonStr);
        JSONParserLib.Item[] memory tcbInfoObj = root.children();
        for (uint256 i = 0; i < root.size(); i++) {
            JSONParserLib.Item memory current = tcbInfoObj[i];
            string memory decodedKey = JSONParserLib.decodeString(current.key());
            if (decodedKey.eq("tcbInfo")) {
                tcbInfoJson.tcbInfoStr = current.value();
            }
        }

        tcbInfoJson.signature = stdJson.readBytes(tcbInfoJsonStr, ".signature");
    }

    function _loadBinary(string memory pathEnvName, string memory hexEnvName) private view returns (bytes memory data) {
        hexEnvName;
        string memory pathValue = vm.envString(pathEnvName);
        string memory fullPath =
            bytes(pathValue)[0] == "/" ? pathValue : string.concat(vm.projectRoot(), "/", pathValue);
        return vm.readFileBinary(fullPath);
    }
}
