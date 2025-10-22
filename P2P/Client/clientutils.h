#ifndef CLIENTUTILS_H
#define CLIENTUTILS_H

#include <string>
#include <vector>

#define SHA_DIGEST_LENGTH 20
#define CHUNK_SIZE 524288 // 512 KB

enum class ChunkStatus { Missing, InProgress, Done };

class ClientUtils {
public:
    std::vector<std::string> get_tokens_for_command(const char* command, const char* delimiter);
    std::string get_file_sha1(const char* filename);
    std::vector<std::string> hash_chunks(const char* filename);
    std::string sha1_of_buffer(const char* buffer, size_t length);
    int shaforcompletefile(const char* filename, unsigned char* out);
};

#endif // CLIENTUTILS_H