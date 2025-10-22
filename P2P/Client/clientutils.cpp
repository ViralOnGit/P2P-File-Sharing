#include "clientutils.h"
#include <openssl/evp.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

using namespace std;

vector<string> ClientUtils::get_tokens_for_command(const char* command, const char* delimiter) {
    vector<string> tokens;
    char* token = strtok(const_cast<char*>(command), delimiter);
    while (token != nullptr) {
        tokens.push_back(string(token));
        token = strtok(nullptr, delimiter);
    }
    return tokens;
}

vector<string> ClientUtils::hash_chunks(const char* filename) {
    vector<string> chunkHashes;
    FILE* pf = fopen(filename, "rb");
    if (!pf) {
        cerr << "Cannot open file: " << filename << endl;
        return chunkHashes;
    }

    unsigned char buf[CHUNK_SIZE];
    size_t bytesRead;
    EVP_MD_CTX* chunkCtx = EVP_MD_CTX_new();
    if (!chunkCtx) {
        cerr << "Cannot create EVP_MD_CTX" << endl;
        fclose(pf);
        return chunkHashes;
    }

    while ((bytesRead = fread(buf, 1, CHUNK_SIZE, pf)) > 0) {
        unsigned char chunkDigest[SHA_DIGEST_LENGTH];
        char chunkHash[SHA_DIGEST_LENGTH * 2 + 1];
        EVP_DigestInit_ex(chunkCtx, EVP_sha1(), nullptr);
        EVP_DigestUpdate(chunkCtx, buf, bytesRead);
        EVP_DigestFinal_ex(chunkCtx, chunkDigest, nullptr);
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
            sprintf(&chunkHash[i * 2], "%02x", chunkDigest[i]);
        }
        chunkHash[SHA_DIGEST_LENGTH * 2] = '\0';
        chunkHashes.push_back(string(chunkHash));
    }

    EVP_MD_CTX_free(chunkCtx);
    fclose(pf);
    return chunkHashes;
}

int ClientUtils::shaforcompletefile(const char* filename, unsigned char* out) {
    FILE* pf = fopen(filename, "rb");
    if (!pf) {
        cerr << "Cannot open file: " << filename << endl;
        return -1;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        cerr << "Cannot create EVP_MD_CTX" << endl;
        fclose(pf);
        return -1;
    }

    EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
    unsigned char buf[CHUNK_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buf, 1, CHUNK_SIZE, pf)) > 0) {
        EVP_DigestUpdate(ctx, buf, bytesRead);
    }

    EVP_DigestFinal_ex(ctx, out, nullptr);
    EVP_MD_CTX_free(ctx);
    fclose(pf);
    return 0;
}

string ClientUtils::get_file_sha1(const char* filename) {
    unsigned char fileDigest[SHA_DIGEST_LENGTH];
    char fileSHA1[SHA_DIGEST_LENGTH * 2 + 1];
    if (shaforcompletefile(filename, fileDigest) != 0) {
        cerr << "Error computing file SHA1" << endl;
        return "";
    }
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&fileSHA1[i * 2], "%02x", fileDigest[i]);
    }
    fileSHA1[SHA_DIGEST_LENGTH * 2] = '\0';
    return string(fileSHA1);
}

string ClientUtils::sha1_of_buffer(const char* buffer, size_t length) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    char hash_str[SHA_DIGEST_LENGTH * 2 + 1];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
    EVP_DigestUpdate(ctx, buffer, length);
    EVP_DigestFinal_ex(ctx, hash, nullptr);
    EVP_MD_CTX_free(ctx);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&hash_str[i * 2], "%02x", hash[i]);
    }
    hash_str[SHA_DIGEST_LENGTH * 2] = '\0';
    return string(hash_str);
}