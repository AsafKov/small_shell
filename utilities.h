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
const int SPECIAL_CHAR_REDIRECT_APPEND = 0;
const int SPECIAL_CHAR_REDIRECT = 1;
const int SPECIAL_PIPE_STDERR = 2;
const int SPECIAL_PIPE_STDOUT = 3;

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
    if(arg.empty()){
        return false;
    }
    bool isNumber = true;
    string temp = arg;
    if(arg.at(0) == '-'){
        temp = temp.substr(1);
    }
    for(char c : temp){
        isNumber &= isdigit(c);
    }
    return isNumber;
}

unsigned int findSpecialChar(const string& cmdline, int *specialType){
    string special_chars[] = {">>", ">", "|&", "|"};
    long index = (int) cmdline.find(special_chars[0].c_str(), 0, 2);
    if(index != string::npos){
        *specialType = SPECIAL_CHAR_REDIRECT_APPEND;
        return index;
    }
    index = (int) cmdline.find(special_chars[1].c_str(), 0, 1);
    if(index != string::npos){
        *specialType = SPECIAL_CHAR_REDIRECT;
        return index;
    }
    index = (int) cmdline.find(special_chars[2].c_str(), 0, 2);
    if(index != string::npos){
        *specialType = SPECIAL_PIPE_STDERR;
        return index;
    }
    index = (int) cmdline.find(special_chars[3].c_str(), 0, 1);
    if(index != string::npos){
        *specialType = SPECIAL_PIPE_STDOUT;
        return index;
    }

    *specialType = NOT_SPECIAL_COMMAND;
    return cmdline.length();
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
