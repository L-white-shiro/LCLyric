#ifndef AES_H
#define AES_H
#include<windows.h>
#include<bcrypt.h>
#include<vector>
#include<string>
#include<stdexcept>

#pragma comment(lib, "bcrypt.lib")
//该段MD5+AES由AI编写
// 辅助函数：将字节数组转换为大写十六进制字符串
inline std::string BytesToHex(const std::vector<BYTE>& bytes) {
    const char* hexChars = "0123456789ABCDEF";
    std::string hexStr;
    hexStr.reserve(bytes.size() * 2);
    for (BYTE b : bytes) {
        hexStr.push_back(hexChars[b >> 4]);
        hexStr.push_back(hexChars[b & 0x0F]);
    }
    return hexStr;
}

// 使用 BCrypt 进行 AES-128-ECB 加密（无宏依赖版）
inline std::string AES128_ECB_Encrypt(const std::string& plaintext, const std::vector<BYTE>& key) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    NTSTATUS status = 0;
    std::vector<BYTE> ciphertext;
    DWORD cbCipherText = 0;
    // 1. 打开 AES 算法
    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (status < 0) throw std::runtime_error("BCrypt: OpenAlgorithm failed");
    // 2. 设置 ECB 模式
    status = BCryptSetProperty(
        hAlg,
        BCRYPT_CHAINING_MODE,
        (PUCHAR)BCRYPT_CHAIN_MODE_ECB,
        sizeof(BCRYPT_CHAIN_MODE_ECB),
        0
        );
    if (status < 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("BCrypt: SetProperty (ECB) failed");
    }
    // 3. 生成密钥
    status = BCryptGenerateSymmetricKey(
        hAlg,
        &hKey,
        NULL,
        0,
        (PUCHAR)key.data(),
        (ULONG)key.size(),
        0
        );
    if (status < 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("BCrypt: GenerateSymmetricKey failed");
    }
    // 4. 计算密文所需大小 (PKCS7 填充)
    ULONG cbInput = (ULONG)plaintext.size();
    status = BCryptEncrypt(
        hKey,
        (PUCHAR)plaintext.data(),
        cbInput,
        NULL,
        NULL,
        0,
        NULL,
        0,
        &cbCipherText,
        BCRYPT_BLOCK_PADDING
        );
    if (status < 0) {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("BCrypt: Get ciphertext size failed");
    }
    // 5. 执行加密
    ciphertext.resize(cbCipherText);
    status = BCryptEncrypt(
        hKey,
        (PUCHAR)plaintext.data(),
        cbInput,
        NULL,
        NULL,
        0,
        ciphertext.data(),
        cbCipherText,
        &cbCipherText,
        BCRYPT_BLOCK_PADDING
        );
    if (status < 0) {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("BCrypt: Encrypt failed");
    }
    // 6. 清理
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return BytesToHex(ciphertext);
}
#endif // AES_H
