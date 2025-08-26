#include "utils.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

Utils::Utils(const string &delim) : delimiter(delim) {}

vector<string> Utils::tokenize_line(const char *line, const char *delimiter)
{
    vector<string> tokens;
    char *mutableLine = strdup(line);
    if (mutableLine == nullptr) {
        perror("strdup failed");
        return tokens;
    }

    char *token = strtok(mutableLine, delimiter);
    while (token != nullptr) {
        tokens.push_back(string(token));
        token = strtok(nullptr, delimiter);
    }
    free(mutableLine);
    return tokens;
}

vector<string> Utils::get_tokens_for_commands(char buffer[1024], const char *delimiter)
{
    return tokenize_line(buffer, delimiter);
}

vector<string> Utils::get_tokens(const char *filename, const char *delimiter)
{
    vector<string> tokens;
    FILE *file = fopen(filename, "r");
    if (file == nullptr) {
        cerr << "Error: Unable to open file." << endl;
        return tokens;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        buffer[strcspn(buffer, "\n")] = '\0';  // remove newline
        vector<string> lineTokens = tokenize_line(buffer, delimiter);
        tokens.insert(tokens.end(), lineTokens.begin(), lineTokens.end());
    }
    fclose(file);
    return tokens;
}
