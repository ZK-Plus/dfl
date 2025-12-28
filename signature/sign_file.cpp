#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

static void printOpenSSLError()
{
    unsigned long errCode = ERR_get_error();
    char errMsg[256];
    ERR_error_string_n(errCode, errMsg, sizeof(errMsg));
    std::cerr << "OpenSSL Error: " << errMsg << std::endl;
}

static bool hexToBytes(const std::string &hexIn, std::vector<unsigned char> &out)
{
    std::string hex = hexIn;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0)
        hex = hex.substr(2);

    std::string cleaned;
    cleaned.reserve(hex.size());
    for (char c : hex)
    {
        if (!std::isspace((unsigned char)c))
            cleaned.push_back(c);
    }

    if (cleaned.empty() || (cleaned.size() % 2 != 0))
        return false;

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F')
            return 10 + (c - 'A');
        return -1;
    };

    out.clear();
    out.reserve(cleaned.size() / 2);
    for (size_t i = 0; i < cleaned.size(); i += 2)
    {
        const int hi = nibble(cleaned[i]);
        const int lo = nibble(cleaned[i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out.push_back((unsigned char)((hi << 4) | lo));
    }
    return true;
}

static EVP_PKEY *loadPrivateKey(const char *privateKeyPath)
{
    FILE *privKeyFile = fopen(privateKeyPath, "r");
    if (!privKeyFile)
    {
        std::cerr << "Error: Unable to open private key file" << std::endl;
        return nullptr;
    }

    EVP_PKEY *privKey = PEM_read_PrivateKey(privKeyFile, NULL, NULL, NULL);
    fclose(privKeyFile);

    if (!privKey)
    {
        std::cerr << "Error: Unable to load private key" << std::endl;
        printOpenSSLError();
        return nullptr;
    }

    return privKey;
}

static EVP_PKEY *loadPublicKeyFromDerHex(const std::string &derHex)
{
    std::vector<unsigned char> der;
    if (!hexToBytes(derHex, der))
    {
        std::cerr << "Error: invalid DER-hex public key" << std::endl;
        return nullptr;
    }

    const unsigned char *p = der.data();
    EVP_PKEY *pub = d2i_PUBKEY(nullptr, &p, (long)der.size());
    if (!pub)
    {
        std::cerr << "Error: d2i_PUBKEY failed" << std::endl;
        printOpenSSLError();
        return nullptr;
    }
    return pub;
}

static bool signBytesWithKey(EVP_PKEY *privateKey, const unsigned char *data, size_t len, std::string &signatureOut)
{
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        std::cerr << "Error: Failed to create EVP_MD_CTX" << std::endl;
        return false;
    }

    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, privateKey) != 1)
    {
        std::cerr << "Error: EVP_DigestSignInit failed" << std::endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    if (EVP_DigestSignUpdate(mdctx, data, len) != 1)
    {
        std::cerr << "Error: EVP_DigestSignUpdate failed" << std::endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    size_t sigLen = 0;
    if (EVP_DigestSignFinal(mdctx, NULL, &sigLen) != 1)
    {
        std::cerr << "Error: EVP_DigestSignFinal (size) failed" << std::endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    signatureOut.assign(sigLen, '\0');
    if (EVP_DigestSignFinal(mdctx, reinterpret_cast<unsigned char *>(signatureOut.data()), &sigLen) != 1)
    {
        std::cerr << "Error: EVP_DigestSignFinal (sign) failed" << std::endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        signatureOut.clear();
        return false;
    }
    signatureOut.resize(sigLen);
    EVP_MD_CTX_free(mdctx);
    return true;
}

static bool verifyBytesWithKey(EVP_PKEY *publicKey, const unsigned char *data, size_t len, const unsigned char *sig, size_t sigLen)
{
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        std::cerr << "Error: Failed to create EVP_MD_CTX" << std::endl;
        return false;
    }

    if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, publicKey) != 1)
    {
        std::cerr << "Error: EVP_DigestVerifyInit failed" << std::endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    if (EVP_DigestVerifyUpdate(mdctx, data, len) != 1)
    {
        std::cerr << "Error: EVP_DigestVerifyUpdate failed" << std::endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    const int rc = EVP_DigestVerifyFinal(mdctx, sig, sigLen);
    EVP_MD_CTX_free(mdctx);
    if (rc == 1)
        return true;
    if (rc == 0)
        return false;

    std::cerr << "Error: EVP_DigestVerifyFinal failed" << std::endl;
    printOpenSSLError();
    return false;
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " <filePath> <privateKeyPemPath> <sigOutPath> <publicKeyDerHex>\n";
        return 2;
    }

    const std::string filePath = argv[1];
    const std::string privateKeyPemPath = argv[2];
    const std::string sigOutPath = argv[3];
    const std::string publicKeyDerHex = argv[4];

    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open())
    {
        std::cerr << "Error: Could not open file to sign: " << filePath << std::endl;
        return 1;
    }
    std::string fileBytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (fileBytes.empty())
    {
        std::cerr << "Error: File is empty, refusing to sign: " << filePath << std::endl;
        return 1;
    }

    EVP_PKEY *priv = loadPrivateKey(privateKeyPemPath.c_str());
    if (!priv)
        return 1;

    std::string signature;
    const bool signOk = signBytesWithKey(priv,
                                         reinterpret_cast<const unsigned char *>(fileBytes.data()),
                                         fileBytes.size(),
                                         signature);
    EVP_PKEY_free(priv);

    if (!signOk)
        return 1;

    std::ofstream out(sigOutPath, std::ios::binary);
    if (!out.is_open())
    {
        std::cerr << "Error: Could not open signature output: " << sigOutPath << std::endl;
        return 1;
    }
    out.write(signature.data(), (std::streamsize)signature.size());
    out.close();

    std::cout << "Signature written: " << sigOutPath << " (bytes=" << signature.size() << ")" << std::endl;

    EVP_PKEY *pub = loadPublicKeyFromDerHex(publicKeyDerHex);
    if (!pub)
        return 1;

    const bool verifyOk = verifyBytesWithKey(pub,
                                             reinterpret_cast<const unsigned char *>(fileBytes.data()),
                                             fileBytes.size(),
                                             reinterpret_cast<const unsigned char *>(signature.data()),
                                             signature.size());
    EVP_PKEY_free(pub);

    std::cout << "Verify: " << (verifyOk ? "OK" : "FAIL") << std::endl;
    return verifyOk ? 0 : 1;
}
