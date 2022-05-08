#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>

/**
 * Utilities
 */
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
using namespace std;

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
    unsigned long index = (int) cmdline.find(special_chars[0].c_str(), 0, 2);
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

/**
 * Commands
 */


// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
    prompt = "smash";
    jobList = new JobsList();
    isRunning = true;
    foregroundCommand = nullptr;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line, int *specialType, string &specialArg, int *commandType) {
    string string_cmdline = cmd_line;
    unsigned int splitAt = findSpecialChar(string_cmdline, specialType);
    string left_side = string_cmdline.substr(0, splitAt);
    string right_side = left_side;
    if (*specialType != NOT_SPECIAL_COMMAND) {
        int length = *specialType == SPECIAL_CHAR_REDIRECT || *specialType == SPECIAL_PIPE_STDOUT ? 1 : 2;
        right_side = string_cmdline.substr(splitAt + length, string_cmdline.length() - 1);
        right_side = _trim(right_side);
    }

    specialArg = right_side;
    string cmd_s = _trim(left_side);
    string firstArg = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    char **args = new char *[COMMAND_MAX_ARGS];

    _parseCommandLine(left_side.c_str(), args);
//    cout<< "\n command is: " << firstArg << "\n";

    Command *command = nullptr;
    if (firstArg == "chprompt") {
        command = new ChangePromptCommand(cmd_line, args);
    }
    if (firstArg == "showpid") {
        command = new ShowPidCommand(cmd_line, args);
    }
    if (firstArg == "pwd") {
        command = new GetCurrDirCommand(cmd_line, args);
    }
    if (firstArg == "cd") {
        command = new ChangeDirCommand(cmd_line, args);
    }
    if (firstArg == "touch") {
        command = new TouchCommand(cmd_line, args);
    }
    if (firstArg == "jobs") {
        command = new JobsCommand(cmd_line);
    }
    if (firstArg == "kill") {
        command = new KillCommand(cmd_line, args);
    }
    if (firstArg == "fg") {
        command = new ForegroundCommand(cmd_line, args);
    }
    if (firstArg == "quit") {
        command = new QuitCommand(cmd_line, args);
    }
    if (firstArg == "bg") {
        command = new BackgroundCommand(cmd_line, args);
    }
    if (firstArg == "tail") {
        command = new TailCommand(cmd_line, args);
    }
    if (command == nullptr) {
        command = new ExternalCommand(cmd_line, left_side, right_side, *specialType);
        *commandType = COMMAND_TYPE_EXTERNAL;
    }

    delete[] args;
    return command;
}

void SmallShell::executeCommand(const char *cmd_line) {
    this->jobList->removeFinishedJobs();
    string specialArg;
    int specialChar, commandType = COMMAND_TYPE_BUILT_IN;
    Command *command = CreateCommand(cmd_line, &specialChar, specialArg, &commandType);
    if (specialChar == NOT_SPECIAL_COMMAND) {
        command->execute();
    } else {
        if (specialChar == SPECIAL_CHAR_REDIRECT || specialChar == SPECIAL_CHAR_REDIRECT_APPEND) {
            if (commandType == COMMAND_TYPE_BUILT_IN) {
                redirectStdout(command, specialArg, specialChar);
            } else {
                if (commandType == COMMAND_TYPE_EXTERNAL) {
                    command->execute();
                }
            }
        } else {
            int pipeline[2];
            int outChannelFd =
                    specialChar == SPECIAL_PIPE_STDOUT ? STDOUT_FILENO : STDERR_FILENO, firstCmdType = commandType;
            if (pipe(pipeline) == -1) {
                perror("smash error: pipe failed");
                exit(0);
            }
            int srcPid = fork();

            if (srcPid == -1) {
                perror("smash error: fork failed");
                exit(0);
            }

            if (srcPid == 0) {
                if(setpgrp() == -1){
                    perror("smash error: setpgrp failed");
                }
                if(dup2(pipeline[1], outChannelFd) == -1){
                    perror("smash error: dup2 failed");
                }
                if (firstCmdType == COMMAND_TYPE_EXTERNAL) {
                    if (close(pipeline[0]) == -1) {
                        perror("smash error: close failed");
                        exit(0);
                    }
                    if (close(pipeline[1]) == -1) {
                        perror("smash error: close failed");
                        exit(0);
                    }
                    execExternal(command->getExecutableCommand());
                } else {
                    command->execute();
                    exit(0);
                }
            }

            commandType = COMMAND_TYPE_BUILT_IN;
            Command *targetCommand = CreateCommand(specialArg.c_str(), &specialChar, specialArg, &commandType);
            int targetPid = fork();
            if (targetPid == -1) {
                perror("smash error: fork failed");
                exit(0);
            } else {
                if (targetPid == 0) {
                    if(setpgrp() == -1){
                        perror("smash error: setpgrp failed");
                    }
                    if(dup2(pipeline[0], STDIN_FILENO) == -1){
                        perror("smash error: dup2 failed");
                    }
                    if (commandType == COMMAND_TYPE_EXTERNAL) {
                        if (close(pipeline[0]) == -1) {
                            perror("smash error: close failed");
                            exit(0);
                        }
                        if (close(pipeline[1]) == -1) {
                            perror("smash error: close failed");
                            exit(0);
                        }
                        execExternal(targetCommand->getExecutableCommand());
                    } else {
                        targetCommand->execute();
                        exit(0);
                    }
                }

                if (close(pipeline[0]) == -1) {
                    perror("smash error: close failed");
                    exit(0);
                }
                if (close(pipeline[1]) == -1) {
                    perror("smash error: close failed");
                    exit(0);
                }

                int status;
                waitpid(srcPid, &status, WUNTRACED);

                int statusTarget;
                waitpid(targetPid, &statusTarget, WUNTRACED);
            }
        }
    }
}

void SmallShell::execExternal(string command) {
    bool isBackground = _isBackgroundCommand(command.c_str());
    int position = (int) command.find_last_of('&');
    if (isBackground) {
        command = command.substr(0, position);
    }
    char *args[] = {(char *) "/bin/bash", (char *) "-c", (char *) command.c_str(),
                    nullptr};
    if (execv(args[0], args) != -1) {
        perror("smash error: execv failed");
    }
}

void SmallShell::redirectStdout(Command *command, const string &specialArg, int redirectionType) {
    int flags = O_CREAT | O_RDWR;
    flags |= redirectionType == SPECIAL_CHAR_REDIRECT ? O_TRUNC : O_APPEND;
    int fd = open(specialArg.c_str(), flags);
    chmod(specialArg.c_str(), 0655);
    int dup_out = dup(STDOUT_FILENO);
    if(dup_out == -1){
        perror("smash error: dup failed");
    }
    if (fd == -1) {
        perror("smash error: open failed");
    } else {
        if(dup2(fd, STDOUT_FILENO) == -1){
            perror("smash error: dup2 failed");
        }
        command->execute();
        if (close(fd) == -1) {
            perror("smash error: close failed");
        }
        if(dup2(dup_out, STDOUT_FILENO) == -1){
            perror("smash error: dup2 failed");
        }
        if(close(dup_out) == -1){
            perror("smash error: close failed");
        }
    }
}

void ChangePromptCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (isValid) {
        if (newPrompt.empty()) {
            smash.setPrompt();
        } else {
            smash.setPrompt(newPrompt);
        }
    } else {
        cerr << errorMessage;
    }
}

void JobsCommand::execute() {
    SmallShell::getInstance().printJobs();
}

void ExternalCommand::execute() {
    if (commandType != NOT_SPECIAL_COMMAND) {
        executeRedirection();
        return;
    }
    SmallShell &smash = SmallShell::getInstance();
    int childPid = fork();
    if (childPid == -1) {
        perror("smash error: fork failed");
        return;
    } else {
        bool isBackground = _isBackgroundCommand(getExecutableCommand().c_str());
        if (childPid == 0) {
            if(setpgrp() == -1){
                perror("smash error: setpgrp failed");
            }
            string cmd_line = getExecutableCommand();
            int position = cmd_line.find_last_of('&');
            if (isBackground) {
                cmd_line = cmd_line.substr(0, position);
            }
            char *args[] = {(char *) "/bin/bash", (char *) "-c", (char *) cmd_line.c_str(), nullptr};
            if (execv(args[0], args) != -1) {
                perror("smash error: execv failed");
            }
        } else {
            this->pid = childPid;
            int status;
            smash.setForegroundCommand(this);
            if (!isBackground) {
                waitpid(childPid, &status, WUNTRACED);
                smash.clearForegroundCommand();
            } else {
                smash.addJob(this);
            }
        }
    }
}

void ExternalCommand::executeRedirection() {
    SmallShell &smash = SmallShell::getInstance();
    int childPid = fork();
    if (childPid == -1) {
        perror("smash error: fork failed");
        return;
    } else {
        bool isBackground = _isBackgroundCommand(getExecutableCommand().c_str());
        if (childPid == 0) {
            if(setpgrp() == -1){
                perror("smash error: setpgrp failed");
            }
            string cmd_line = getExecutableCommand();
            int position = (int) cmd_line.find_last_of('&');
            if (isBackground) {
                cmd_line = cmd_line.substr(0, position);
            }
            char *args[] = {(char *) "/bin/bash", (char *) "-c", (char *) cmd_line.c_str(), nullptr};
            int flags = O_CREAT | O_RDWR;
            flags |= commandType == SPECIAL_CHAR_REDIRECT ? O_TRUNC : O_APPEND;
            int fd = open(out_file.c_str(), flags);
            chmod(out_file.c_str(), 0655);
            if (fd == -1) {
                perror("smash error: open failed");
            } else {
                if(dup2(fd, 1) == -1){
                    perror("smash error: dup2 failed");
                }
                if(execv(args[0], args) == -1){
                    perror("smash error: execv failed");
                }
                if (close(fd) == -1) {
                    perror("smash error: close failed");
                }
            }
            exit(0);
        } else {
            this->pid = childPid;
            int status;
            smash.setForegroundCommand(this);
            waitpid(childPid, &status, WUNTRACED);
            smash.clearForegroundCommand();
        }
    }
}

KillCommand::KillCommand(const char *cmdLine, char **args) : BuiltInCommand(cmdLine) {
    if (args[1] == nullptr || args[2] == nullptr) { // Not enough args
        errorMessage = "smash error: kill: invalid arguments\n";
    } else {
        if (args[3] != nullptr) { // Too many
            errorMessage = "smash error: kill: invalid arguments\n";
        } else { // *chef's kiss*
            bool isProperFormat = true;
            string firstArg = args[1];
            isProperFormat &= firstArg.at(0) == '-';
            firstArg = firstArg.substr(1, firstArg.length() - 1);
            isProperFormat &= isNumber(firstArg);
            isProperFormat &= isNumber(args[2]);
            if (isProperFormat) {
                sigNum = stoi(firstArg);
                if (sigNum > 64) {
                    errorMessage = "smash error: kill: invalid arguments\n";
                }
                jobId = stoi(args[2]);
            } else {
                errorMessage = "smash error: kill: invalid arguments\n";
            }
        }
    }
}

ShowPidCommand::ShowPidCommand(const char *cmd_line, char **args)
        : BuiltInCommand(cmd_line) {
    output = "smash pid is " + to_string(getpid()) + "\n";
}

void ShowPidCommand::execute() {
    if (errorMessage.empty()) {
        cout << output;
    } else {
        cerr << errorMessage;
    }
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line, char **args)
        : BuiltInCommand(cmd_line) {
    char buffer[FILENAME_MAX];
    getcwd(buffer, FILENAME_MAX);
    output = std::string(buffer) + "\n";
}

void GetCurrDirCommand::execute() {
    if (errorMessage.empty()) {
        cout << output;
    } else {
        cerr << errorMessage;
    }
}

void ChangeDirCommand::execute() {
    if (!errorMessage.empty()) {
        cerr << errorMessage;
    } else {
        SmallShell &smash = SmallShell::getInstance();
        char currDir[FILENAME_MAX];
        getcwd(currDir, FILENAME_MAX);
        if (newDir == "-") {
            if (smash.getPrevDirectory().empty()) {
                errorMessage = "smash error: cd: OLDPWD not set\n";
                cerr << errorMessage;
            } else {
                if (chdir(smash.getPrevDirectory().c_str()) == 0) {
                    smash.setPrevDirectory(currDir);
                } else {
                    perror("smash error: chdir failed");
                }
            }
        } else {
            if (chdir(newDir.c_str()) == 0) {
                smash.setPrevDirectory(currDir);
            } else {
                perror("smash error: chdir failed");
            }
        }
    }
}

void JobsList::addJob(ExternalCommand *cmd, bool isStopped) {
    if (foregroundJob != nullptr && cmd->getPid() == foregroundJob->getCommand()->getPid()) {
        foregroundJob->changeStatus(isStopped);
        foregroundJob->setInitTIme(time(nullptr));
        foregroundJob = nullptr;
    } else {
        removeFinishedJobs();
        currentMaxId += 1;
        auto *entry = new JobEntry(currentMaxId, cmd, isStopped);
        entry->setInitTIme(time(nullptr));
        jobs.push_back(entry);
    }
}

void JobsList::removeFinishedJobs() {
    int currentMax = 0;
    int status, result;
    for (unsigned int i = 0; i < jobs.size(); i++) {
        result = waitpid(jobs.at(i)->getCommand()->getPid(), &status, WNOHANG);
        if (jobs.at(i)->getStatus() == STATUS_KILLED || (result > 0 && (WIFEXITED(status) || WIFSIGNALED(status)))) {
            delete jobs.at(i)->getCommand();
            jobs.erase(jobs.begin() + i);
        } else {
            currentMax = currentMax < jobs.at(i)->getId() ? jobs.at(i)->getId() : currentMax;
        }
    }
    currentMaxId = currentMax;
}

void JobsList::killAll() {
    removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to " << jobs.size() << " jobs:\n";
    for (unsigned int i = 0; i < jobs.size(); i++) {
        if (jobs.at(i)->getStatus() == STATUS_ACTIVE || jobs.at(i)->getStatus() == STATUS_STOPPED) {
            cout << "" << jobs.at(i)->getCommand()->getPid() << ": " << jobs.at(i)->getCommand()->getCmdLine() << "\n";
            kill(jobs.at(i)->getCommand()->getPid(), SIGKILL);
        }
    }
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    ExternalCommand *cmd = nullptr;
    for (JobEntry *entry: jobs) {
        if (entry == foregroundJob) continue;
        cmd = (ExternalCommand *) entry->getCommand();
        cout << "[" << entry->getId() << "] " << cmd->getCmdLine() << " : " << cmd->getPid() << " "
             << difftime(time(nullptr), entry->getInitTime()) << " secs";
        if (entry->getStatus() == STATUS_STOPPED) {
            cout << " (stopped)\n";
        } else {
            cout << "\n";
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (JobEntry *entry: jobs) {
        if (entry->getId() == jobId) {
            return entry;
        }
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    int maxId = 0;
    JobEntry *lastJob = nullptr;
    for (JobEntry *entry: jobs) {
        if (entry->getId() > maxId) {
            lastJob = entry;
            maxId = entry->getId();
        }
    }
    *lastJobId = maxId;
    return lastJob;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int maxId = 0;
    JobEntry *lastJob = nullptr;
    for (JobEntry *entry: jobs) {
        if ((entry->getId() > maxId) && (entry->getStatus() == STATUS_STOPPED)) {
            lastJob = entry;
            maxId = entry->getId();
        }
    }
    *jobId = maxId;
    return lastJob;
}


void JobsList::removeJobById(int jobId) {
    for (unsigned int i = 0; i < jobs.size(); i++) {
        if (jobs.at(i)->getId() == jobId) {
            jobs.erase(jobs.begin() + i);
            break;
        }
    }
}

void KillCommand::execute() {
    if (!errorMessage.empty()) {
        cerr << errorMessage;
    } else {
        SmallShell &smash = SmallShell::getInstance();
        int jobPid = smash.getJobPid(jobId);
        if (jobPid == -1) {
            cerr << "smash error: kill: job-id " << jobId << " does not exist\n";
        } else {
            if (kill(jobPid, sigNum) == -1) {
                perror("smash error: kill failed");
            } else {
                cout << "signal number " << sigNum << " was sent to pid " << jobPid << "\n";
                switch (sigNum) {
                    case SIGTSTP: {
                        smash.changeJobStatus(jobId, JobsList::STATUS_STOPPED);
                        break;
                    }
                    case SIGTERM: {
                        smash.changeJobStatus(jobId, JobsList::STATUS_KILLED);
                        break;
                    }
                    case SIGCONT: {
                        smash.changeJobStatus(jobId, JobsList::STATUS_ACTIVE);
                        break;
                    }
                    case SIGKILL: {
                        smash.changeJobStatus(jobId, JobsList::STATUS_KILLED);
                        break;
                    }
                }
            }
        }
    }
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {
    if (args[1] != nullptr) {
        if (args[2] != nullptr) {
            errorMessage = "smash error: fg: invalid arguments\n";
        } else {
            if (!isNumber(args[1])) {
                // TODO: is jobId < 0 invalid argument?
                errorMessage = "smash error: fg: invalid arguments\n";
            } else {
                jobId = stoi(args[1]);
                if (jobId < 1) {
                    errorMessage = "smash error: fg: job-id " + string(args[1]) + " does not exist\n";
                }
            }
        }
    } else {
        jobId = LATEST_JOB;
    }
}

void ForegroundCommand::execute() {
    if (!errorMessage.empty()) {
        cerr << errorMessage;
    } else {
        SmallShell &smash = SmallShell::getInstance();
        int id = JobsList::NOT_FOUND, status;
        ExternalCommand *cmd = smash.pushToForeground(jobId, &id);
        if (id <= 0) {
            if (jobId == 0) {
                cerr << "smash error: fg: jobs list is empty\n";
            } else {
                cerr << "smash error: fg: job-id " << jobId << " does not exist\n";
            }
        } else {
            cout << cmd->getCmdLine() << " : " << cmd->getPid() << "\n";
            kill(cmd->getPid(), SIGCONT);
            waitpid(cmd->getPid(), &status, WUNTRACED);
            smash.clearForegroundJob();
        }
    }
}


BackgroundCommand::BackgroundCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {
    if (args[1] != nullptr) {
        if (args[2] != nullptr) {
            errorMessage = "smash error: bg: invalid arguments\n";
        } else {
            if (!isNumber(args[1]) && !isNumber(args[1] + 1)) {
                // TODO: is jobId < 0 invalid argument?
                errorMessage = "smash error: bg: invalid arguments\n";
            } else {
                jobId = stoi(args[1]);
                if (jobId < 1) {
                    errorMessage = "smash error: bg: job-id " + string(args[1]) + " does not exist\n";
                }
            }
        }
    } else {
        jobId = LATEST_JOB;
    }
}

void BackgroundCommand::execute() {
    if (!errorMessage.empty()) {
        cerr << errorMessage;
    } else {
        SmallShell &smash = SmallShell::getInstance();
        int id = JobsList::NOT_FOUND;
        ExternalCommand *cmd = smash.resumeStopped(jobId, &id);
        if (id <= 0) {
            if (jobId == 0) {
                cerr << "smash error: bg: there is no stopped jobs to resume\n";
            } else {
                cerr << "smash error: bg: job-id " << jobId << " does not exist\n";
            }
        } else {
            if (cmd != nullptr) {
                cout << cmd->getCmdLine() << " : " << cmd->getPid() << "\n";
                kill(cmd->getPid(), SIGCONT);
            }
        }
    }
}

QuitCommand::QuitCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {
    if (args[1] != nullptr) {
        if (string(args[1]) == "kill") {
            isKill = true;
        }
    } else {
        isKill = false;
    }
}

void QuitCommand::execute() {
    SmallShell::getInstance().removeFinishedJobs();
    if (isKill) {
        SmallShell::getInstance().killAll();
    } else {
        SmallShell::getInstance().setIsRunning();
    }
}

TailCommand::TailCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {

    if (args[1] == nullptr) {
        errorMessage = "smash error: tail: invalid arguments\n";
    }

    else {
        if (args[2]!=nullptr)
        {
            if (args[3]!=nullptr)
            {
                errorMessage = "smash error: tail: invalid arguments\n";
            }
        }
        string firstArg = args[1];
        string numS = firstArg.substr(1, firstArg.length() - 1);
        if (isNumber(numS))
        {
            if (firstArg.at(0) == '-')
            {
                numLines = stoi(numS);
                if (args[2] == nullptr) {
                    errorMessage = "smash error: tail: invalid arguments\n";
                } else {
                    fileName = args[2];
                }
            }
            else if (args[2]!=nullptr)
            {
                errorMessage = "smash error: tail: invalid arguments\n";
            }
        }
        else {
            if (args[2]!=nullptr)
            {
                errorMessage = "smash error: tail: invalid arguments\n";
            }
            else
            {
                fileName = args[1];
                numLines = 10;
            }

        }
    }
}

void TailCommand::execute() {
    if (!errorMessage.empty()) {
        cerr << errorMessage;
    } else {
        char buffer[1];
        //char* bufferOut[numLines];
        int resultOpen = open(fileName.c_str(), O_RDONLY);
        if (resultOpen == -1) {
            perror("smash error: open failed");
        } else if (numLines == 0) {}
        else {
            int counter = 1;
            int resultRead = 2;
            int isEmpty = 999;
            while (resultRead != 0) {
                resultRead = read(resultOpen, buffer, 1);
                if (resultRead == -1) {
                    perror("smash error: read failed");
                }
                if (isEmpty == 999 && buffer[0] == '\0') {
                    //exit(0);
                    isEmpty = 1;
                    break;
                }
                isEmpty=0;

                if (buffer[0] == '\n') {
                    counter++;
                }
            }
            if (!isEmpty) {
                    if (buffer[0] == '\n') {
                        counter -= 2;
                    }

                    if (counter < numLines && counter > 0) {
                        numLines = counter;
                    }
                    if (counter == 0) {
                        numLines = 1;
                    }

                    int resultOpen2 = open(fileName.c_str(), O_RDONLY);
                    if (resultOpen2 == -1) {
                        perror("smash error: open failed");
                    }
                    int resultRead2 = 2;
                    int resultOpen3 = open(fileName.c_str(), O_RDONLY);
                    if (resultOpen3 == -1) {
                        perror("smash error: open failed");
                    }
                    int resultRead3 = 2;

                    int countLines = 0, countChars = 0, countRestChars = 0, countTotalLines = 0;
                    if (counter == 0) {
                        if (isEmpty) {}
                        else {

                            while (resultRead2 != 0) {
                                resultRead2 = read(resultOpen2, buffer, 1);
                                if (resultRead2 == -1) {
                                    perror("smash error: read failed");
                                }
                                countChars++;
                            }
                            countChars--;
                        }
                    } else {
                        while ((countLines < numLines) && (countTotalLines <= counter)) {
                            resultRead2 = read(resultOpen2, buffer, 1);
                            if (resultRead2 == -1) {
                                perror("smash error: read failed");
                            }
                            if ((buffer[0] == '\n') || (resultRead2 == 0)) {
                                if (countTotalLines >= counter - numLines) {
                                    countLines++;
                                    //if (countLines>counter-numLines){
                                    //countChars++;
                                    //break;
                                    //}
                                }
                                countTotalLines++;
                                if (countTotalLines == counter - numLines) {
                                    countRestChars++;
                                    continue;
                                }
                            }
                            if (countTotalLines >= counter - numLines) {
                                countChars++;
                            } else {
                                countRestChars++;
                            }

                        }
                    }
                    char *bufferOut[countChars];
                    if (buffer[0] != '\n') {
                        if (countRestChars > 0) {
                            char *bufferGarbage[countRestChars];
                            resultRead3 = read(resultOpen3, bufferGarbage, countRestChars);
                            if (resultRead3 == -1) {
                                perror("smash error: read failed");
                            }
                        }
                        countChars--;
                        resultRead3 = read(resultOpen3, bufferOut, countChars);
                        if (resultRead3 == -1) {
                            perror("smash error: read failed");
                        }
                        int writeRes = write(STDOUT_FILENO, bufferOut, countChars);
                        if (writeRes == -1) {
                            perror("smash error: write failed");
                        }
                        if(close(resultOpen3) == -1){
                            perror("smash error: close failed");
                        }
                        if(close(resultOpen2) == -1){
                            perror("smash error: close failed");
                        }
                        if(close(resultOpen) == -1){
                            perror("smash error: close failed");
                        }
                    } else {
                        if (countRestChars > 0) {
                            char *bufferGarbage[countRestChars];
                            resultRead3 = read(resultOpen3, bufferGarbage, countRestChars);
                            if (resultRead3 == -1) {
                                perror("smash error: read failed");
                            }
                        }
                        resultRead3 = read(resultOpen3, bufferOut, countChars);
                        if (resultRead3 == -1) {
                            perror("smash error: read failed");
                        }
                        int writeRes = write(STDOUT_FILENO, bufferOut, countChars);
                        if (writeRes == -1) {
                            perror("smash error: write failed");
                        }
                        if(close(resultOpen3) == -1){
                            perror("smash error: close failed");
                        }
                        if(close(resultOpen2) == -1){
                            perror("smash error: close failed");
                        }
                        if(close(resultOpen) == -1){
                            perror("smash error: close failed");
                        }
                    }

                }

            }
        }
    }


TouchCommand::TouchCommand(const char *cmd_line, char **args) : BuiltInCommand(
        cmd_line) {
    if (args[3] != nullptr) {
        errorMessage = "smash error: touch: invalid arguments\n";
    } else {
        if (args[1] == nullptr || args[2] == nullptr) {
            errorMessage = "smash error: touch: invalid arguments\n";
        } else {
            fileName = args[1];
            timeString = args[2];
        }
    }
}

void TouchCommand::execute() {
    if (errorMessage.empty()) {
        struct std::tm time{};
        strptime(timeString.c_str(), "%S:%M:%H:%d:%m:%Y", &time);
        struct utimbuf timeBuffer{};
        std::time_t timestamp = mktime(&time);
        timeBuffer.actime = timestamp;
        timeBuffer.modtime = timestamp;
        if (utime(fileName.c_str(), &timeBuffer) == -1) {
            perror("smash error: utime failed");
        }
    } else {
        cerr << errorMessage;
    }
}