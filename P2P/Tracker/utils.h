#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
using namespace std;

class Utils {
private:
    string delimiter;

public:
    
    Utils(const string &delim = " ");
    vector<string> tokenize_line(const char *line, const char *delimiter);
    vector<string> get_tokens_for_commands(char buffer[1024], const char *delimiter);
    vector<string> get_tokens(const char *filename, const char *delimiter);
};

#endif // UTILS_H
