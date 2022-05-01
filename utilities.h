#ifndef EX1_UTILITIES_H
#include <string.h>
#include <ctype.h>
#define EX1_UTILITIES_H

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

const int NOT_SPECIAL_COMMAND = -1;
const int SPECIAL_CHAR_REDIRECT = 0;
const int SPECIAL_CHAR_REDIRECT_APPEND = 1;
const int SPECIAL_PIPE_STDOUT = 2;
const int SPECIAL_PIPE_STDERR = 3;

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

bool isNumber(const string& arg){
    bool isNumber = true;
    for(char c: arg){
        isNumber &= isdigit(c);
    }
    return isNumber;
}

int isSpecialCommand(char **args, int *pos){
    string special_chars[] = {">", ">>", "|", "|&"};
    int index = 0;
    while(args[index] != nullptr){
        if(args[index] == special_chars[0]){
            *pos = index;
            return SPECIAL_CHAR_REDIRECT;
        }
        if(args[index] == special_chars[1]){
            *pos = index;
            return SPECIAL_CHAR_REDIRECT_APPEND;
        }
        if(args[index] == special_chars[2]){
            *pos = index;
            return SPECIAL_PIPE_STDOUT;
        }
        if(args[index] == special_chars[3]){
            *pos = index;
            return SPECIAL_PIPE_STDERR;
        }
        index++;
    }
    return NOT_SPECIAL_COMMAND;
}

int specialCharStringPosition(const string& cmdline){
    string special_chars[] = {">", ">>", "|", "|&"};
    int index = (int) cmdline.find(special_chars[0]);
    if(index != -1){
        return index;
    }
    index = (int) cmdline.find(special_chars[1]);
    if(index != -1){
        return index;
    }
    index = (int) cmdline.find(special_chars[2]);
    if(index != -1){
        return index;
    }
    index = (int) cmdline.find(special_chars[3]);
    return index;
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundCommand(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

#endif //EX1_UTILITIES_H
