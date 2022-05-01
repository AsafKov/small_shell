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
#include "utilities.h"
#include <time.h>
#include <utime.h>

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
    prompt = "smash";
    jobList = new JobsList();
    isRunning = true;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line, int *specialType, string &specialArg, int *commandType,
                                   string *targetCommandCmdline) {
    string cmd_s = _trim(string(cmd_line));
    string firstArg = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    char **args = new char *[COMMAND_MAX_ARGS];

    _parseCommandLine(cmd_line, args);
    int splitAt = specialCharStringPosition(string(cmd_line));
    int specialCharIndex;
    *specialType = isSpecialCommand(args, &specialCharIndex);
    if (*specialType != NOT_SPECIAL_COMMAND) {
        specialArg = args[specialCharIndex + 1];
    }
    string cmdline_beforeChar = cmd_line;
    if (splitAt != -1) {
        int length = *specialType == SPECIAL_CHAR_REDIRECT || *specialType == SPECIAL_PIPE_STDOUT ? 1 : 2;
        *targetCommandCmdline = cmdline_beforeChar.substr(splitAt + length, string::npos);
        cmdline_beforeChar = cmdline_beforeChar.substr(0, splitAt - 1);
    }

    Command *command = nullptr;
    if (firstArg == "chprompt") {
        command = new ChangePromptCommand(cmd_line, args, 0, specialCharIndex);
    }
    if (firstArg == "showpid") {
        command = new ShowPidCommand(cmd_line, args, 0, specialCharIndex);
    }
    if (firstArg == "pwd") {
        command = new GetCurrDirCommand(cmd_line, args, 0, specialCharIndex);
    }
    if (firstArg == "cd") {
        command = new ChangeDirCommand(cmd_line, args, 0, specialCharIndex);
    }
    if(firstArg == "touch"){
        command = new TouchCommand(cmd_line, args, 0, specialCharIndex);
    }
    if (firstArg == "jobs") {
        command = new JobsCommand(cmd_line);
        *commandType = COMMAND_TYPE_BLOCKING;
    }
    if (firstArg == "kill") {
        command = new KillCommand(cmd_line, args);
        *commandType = COMMAND_TYPE_BLOCKING;
    }
    if (firstArg == "fg") {
        command = new ForegroundCommand(cmd_line, args);
        *commandType = COMMAND_TYPE_BLOCKING;
    }
    if (firstArg == "quit") {
        command = new QuitCommand(cmd_line, args);
        *commandType = COMMAND_TYPE_BLOCKING;
    }
    if (firstArg == "bg") {
        command = new BackgroundCommand(cmd_line, args);
        *commandType = COMMAND_TYPE_BLOCKING; // TODO: redirection?
    }
    if (firstArg == "tail") {
        command = new TailCommand(cmd_line, args);
    }
    if (command == nullptr) {
        command = new ExternalCommand(cmdline_beforeChar.c_str(), *specialType, specialArg);
        *commandType = COMMAND_TYPE_EXTERNAL;
    }

    delete[] args;
    return command;
}

void SmallShell::executeCommand(const char *cmd_line) {
    this->jobList->removeFinishedJobs();
    string specialArg, targetCommandCmdline;
    int specialChar, commandType = COMMAND_TYPE_BUILT_IN;
    Command *command = CreateCommand(cmd_line, &specialChar, specialArg, &commandType, &targetCommandCmdline);
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
            if(pipe(pipeline) == -1){
                perror("smash error: pipe failed");
            }
            int outChannelFd = specialChar == SPECIAL_PIPE_STDOUT ? 1 : 2, firstCmdType = commandType, srcPid = fork();
            if(srcPid == -1){
                perror("smash error: fork failed");
            }
            if (srcPid == 0) {
                setpgrp();
                dup2(pipeline[1], outChannelFd);
                if(close(pipeline[0]) == -1){
                    perror("smash error: close failed");
                }
                if(close(pipeline[1]) == -1){
                    perror("smash error: close failed");
                }
                if(firstCmdType == COMMAND_TYPE_EXTERNAL){
                    string src_command = command->getCmdLine();
                    bool isBackground = _isBackgroundCommand(command->getCmdLine().c_str());
                    int position = (int) src_command.find_last_of('&');
                    if (isBackground) {
                        src_command = src_command.substr(0, position);
                    }
                    char *args[] = {(char *) "/bin/bash", (char *) "-c", (char *) src_command.c_str(), nullptr};
                    if (execv(args[0], args) != -1) {
                        perror("smash error: execv failed");
                    }
                } else {
                    command->execute();
                    exit(0);
                }
            } else {
                int status;
                waitpid(srcPid, &status, WUNTRACED);
                close(pipeline[1]);
            }

            Command *targetCommand = CreateCommand(targetCommandCmdline.c_str(), &specialChar, specialArg, &commandType,
                                                   &targetCommandCmdline);
            int targetPid = fork();
            if (targetPid == -1) {
                perror("smash error: fork failed");
            } else {
                if (targetPid == 0) {
                    setpgrp();
                    dup2(pipeline[0], 0);
                    if(close(pipeline[0]) == -1){
                        perror("smash error: close failed");
                    }
                    if (commandType == COMMAND_TYPE_EXTERNAL) {
                        string target_command = targetCommand->getCmdLine();
                        bool isBackground = _isBackgroundCommand(targetCommand->getCmdLine().c_str());
                        int position = (int) target_command.find_last_of('&');
                        if (isBackground) {
                            target_command = target_command.substr(0, position);
                        }
                        char *args[] = {(char *) "/bin/bash", (char *) "-c", (char *) target_command.c_str(),
                                        nullptr};
                        if (execv(args[0], args) != -1) {
                            perror("smash error: execv failed");
                        }
                    } else {
                        targetCommand->execute();
                        exit(0);
                    }

                } else {
                    int status;
                    waitpid(targetPid, &status, WUNTRACED);
                }
            }
        }
    }
}

void SmallShell::redirectStdout(Command *command, const string &specialArg, int redirectionType) {
    string mode = redirectionType == SPECIAL_CHAR_REDIRECT ? "w" : "a";
    int stdout_dup_fd = dup(1);
    FILE *outputFile = fopen(specialArg.c_str(), mode.c_str());
    if (outputFile == nullptr) {
        perror("smash error: fopen failed");
    } else {
        dup2(fileno(outputFile), 1);
        command->execute();
        fclose(outputFile);
        dup2(stdout_dup_fd, 1);
        close(stdout_dup_fd);
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
        setpgrp();
        bool isBackground = _isBackgroundCommand(this->getCmdLine().c_str());
        if (childPid == 0) {
            string cmd_line = this->getCmdLine();
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
        setpgrp();
        bool isBackground = _isBackgroundCommand(this->getCmdLine().c_str());
        if (childPid == 0) {
            string cmd_line = this->getCmdLine();
            int position = cmd_line.find_last_of('&');
            if (isBackground) {
                cmd_line = cmd_line.substr(0, position);
            }
            char *args[] = {(char *) "/bin/bash", (char *) "-c", (char *) cmd_line.c_str(), nullptr};
            string mode = commandType == SPECIAL_CHAR_REDIRECT ? "w" : "a";
            int stdout_dup_fd = dup(1);
            FILE *outputFile = fopen(arg.c_str(), mode.c_str());
            if (outputFile == nullptr) {
                perror("smash error: fopen failed");
            } else {
                dup2(fileno(outputFile), 1);
                execv(args[0], args);
                fclose(outputFile);
                dup2(stdout_dup_fd, 1);
                close(stdout_dup_fd);
            }
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
            errorMessage = "smash error: kill: too many arguments\n";
        } else { // *chef's kiss*
            bool isProperFormat = true;
            string firstArg = args[1];
            isProperFormat &= firstArg.at(0) == '-';
            firstArg = firstArg.substr(1, firstArg.length() - 1);
            isProperFormat &= isNumber(firstArg);
            isProperFormat &= isNumber(args[2]);
            if (isProperFormat) {
                sigNum = stoi(firstArg);
                jobId = stoi(args[2]);
            } else {
                errorMessage = "smash error: kill: invalid arguments\n";
            }
        }
    }
}

ShowPidCommand::ShowPidCommand(const char *cmd_line, char **args, int position, int specialCharPosition)
        : BuiltInCommand(cmd_line) {
    if (args[position + 1] != nullptr && position + 1 != specialCharPosition) {
        errorMessage = "smash error: showpid: too many arguments\n";
    } else {
        output = "smash pid is " + to_string(getpid()) + "\n";
    }
}

void ShowPidCommand::execute() {
    if (errorMessage.empty()) {
        cout << output;
    } else {
        cerr << errorMessage;
    }
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line, char **args, int position, int specialCharPosition)
        : BuiltInCommand(cmd_line) {
    if (args[position + 1] != nullptr && position + 1 != specialCharPosition) {
        errorMessage = "smash error: pwd: too many arguments\n";
    } else {
        char buffer[FILENAME_MAX];
        getcwd(buffer, FILENAME_MAX);
        output = std::string(buffer) + "\n";
    }
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
                    perror("smash error: cdhdir failed");
                }
            }
        } else {
            if (chdir(newDir.c_str()) == 0) {
                smash.setPrevDirectory(currDir);
            } else {
                perror("smash error: cdhdir failed");
            }
        }
    }
}

void JobsList::addJob(ExternalCommand *cmd, bool isStopped) {
    if(foregroundJob != nullptr && cmd->getPid() == foregroundJob->getCommand()->getPid()){
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
        if (jobs.at(i)->getStatus() == STATUS_KILLED || ( result > 0 && (WIFEXITED(status) || WIFSIGNALED(status)))) {
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
    cout << "sending SIGKILL signal to " << jobs.size() << " jobs:\n";
    for (unsigned int i = 0; i < jobs.size(); i++) {
        if (jobs.at(i)->getStatus() == STATUS_ACTIVE || jobs.at(i)->getStatus() == STATUS_STOPPED) {
            cout << "" << jobs.at(i)->getId() << " " << jobs.at(i)->getCommand()->getCmdLine() << "\n";
            kill(jobs.at(i)->getCommand()->getPid(), SIGKILL);
        }
    }
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    ExternalCommand *cmd = nullptr;
    for (JobEntry *entry: jobs) {
        if(entry == foregroundJob) continue;
        cmd = (ExternalCommand *) entry->getCommand();
        cout << "[" << entry->getId() << "] " << cmd->getCmdLine() << ": " << cmd->getPid() << " "
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
                }
            }
        }
    }
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {
    if (args[2] != nullptr) {
        errorMessage = "smash error: fg: too many arguments\n";
    } else {
        if (args[1] != nullptr) {
            if (!isNumber(args[1])) {
                // TODO: is jobId < 0 invalid argument?
                errorMessage = "smash error: fg: invalid arguments\n";
            } else {
                jobId = stoi(args[1]);
                if (jobId < 1) {
                    errorMessage = "smash error: fg: job " + string(args[1]) + " does not exist\n";
                }
            }
        } else {
            jobId = LATEST_JOB;
        }
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
                cerr << "smash error: fg: job " << jobId << " does not exist\n";
            }
        } else {
            cout << cmd->getCmdLine() << ": " << cmd->getPid() << "\n";
            kill(cmd->getPid(), SIGCONT);
            waitpid(cmd->getPid(), &status, WUNTRACED);
            smash.clearForegroundJob();
        }
    }
}


BackgroundCommand::BackgroundCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {
    if (args[2] != nullptr) {
        errorMessage = "smash error: bg:  invalid arguments\n";
    } else {
        if (args[1] != nullptr) {
            if (!isNumber(args[1])) {
                // TODO: is jobId < 0 invalid argument?
                errorMessage = "smash error: fg: invalid arguments\n";
            } else {
                jobId = stoi(args[1]);
                if (jobId < 1) {
                    errorMessage = "smash error: bg: job " + string(args[1]) + " does not exist\n";
                }
            }
        } else {
            jobId = LATEST_JOB;
        }
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
                cerr << "smash error: bg: jobs list is empty\n";
            } else {
                cerr << "smash error: bg: job " << jobId << " does not exist\n";
            }
        } else {
            cout << cmd->getCmdLine() << "\n";
            kill(cmd->getPid(), SIGCONT);
        }
    }
}

QuitCommand::QuitCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {
    if (args[1]!=nullptr) {
        if (string(args[1]) == "kill") {
            isKill = true;
        }
    } else {
        isKill=false;
    }
}

void QuitCommand::execute() {
    SmallShell::getInstance().removeFinishedJobs();
    if (isKill) {
        SmallShell::getInstance().killAll();
    }
    else {
        SmallShell::getInstance().setIsRunning();
    }
}

TailCommand::TailCommand(const char *cmd_line, char **args) : BuiltInCommand(cmd_line) {
    if (args[1]== nullptr){
        errorMessage = "smash error: tail: invalid arguments\n";
    }
    else{
        if (isNumber(args[1])){
            numLines = stoi(args[1]);
            if (args[2]== nullptr){
                errorMessage = "smash error: tail: invalid arguments\n";
            }
            else {
                fileName = args[2];
            }
        }
        else{
            fileName = args[1];
            numLines = 10;
        }
    }
}

void TailCommand::execute() {
    if(!errorMessage.empty()){
        cout << errorMessage;
    } else {
        char buffer[1];
        //char* bufferOut[numLines];
        int resultOpen = open(fileName.c_str(), O_RDONLY);
        if(resultOpen == -1){
            perror("smash error: open failed");
        } else {
            int counter=1;
            int resultRead=2;
            int isEmpty=999;
            while ( resultRead != 0)
            {
                resultRead = read(resultOpen, buffer,1);
                if(resultRead == -1){
                    perror("smash error: read failed");}
                if (isEmpty == 999 && buffer[0] == '\0'){
                    exit(0);
                }
                isEmpty=0;
                if (buffer[0]=='\n') {
                    counter++;
                }
            }
            if (buffer[0]=='\n')
            {
                counter-=2;
            }

            if (counter<numLines && counter>0){
                numLines=counter;
            }
            if (counter==0){
                numLines=1;
            }

            int resultOpen2 = open(fileName.c_str(), O_RDONLY);
            if(resultOpen2 == -1) {
                perror("smash error: open failed");
            }
            int resultRead2=2;
            int resultOpen3 = open(fileName.c_str(), O_RDONLY);
            if(resultOpen3 == -1){
                perror("smash error: open failed");}
            int resultRead3=2;

            int countLines=0, countChars=0, countRestChars=0, countTotalLines=0;
            if (counter==0)
            {

                while (resultRead2!=0)
                {
                    resultRead2 = read(resultOpen2, buffer,1);
                    if(resultRead2 == -1){
                        perror("smash error: read failed");}
                    countChars++;
                }
                countChars--;
            }
            else {
                while ((countLines < numLines) && (countTotalLines <= counter)) {
                    resultRead2 = read(resultOpen2, buffer, 1);
                    if(resultRead2 == -1){
                        perror("smash error: read failed");}
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
            char* bufferOut[countChars];
            if (buffer[0]!='\n')
            {
                if (countRestChars>0){
                    char* bufferGarbage[countRestChars];
                    resultRead3 = read(resultOpen3, bufferGarbage,countRestChars);
                    if(resultRead3 == -1){
                        perror("smash error: read failed");}
                }
                countChars--;
                resultRead3 = read(resultOpen3, bufferOut,countChars);
                if(resultRead3 == -1){
                    perror("smash error: read failed");}
                int writeRes = write(STDOUT_FILENO, bufferOut, countChars);
                if(writeRes == -1){
                    perror("smash error: write failed");}
                char space[1];
                space[0]='\n';
                int writeSpace=write(STDOUT_FILENO, space, 1);
                if(writeSpace== -1){
                    perror("smash error: write failed");}
            }
            else{
                if (countRestChars>0){
                    char* bufferGarbage[countRestChars];
                    resultRead3 = read(resultOpen3, bufferGarbage,countRestChars);
                    if(resultRead3 == -1){
                        perror("smash error: read failed");}
                }
                resultRead3 = read(resultOpen3, bufferOut,countChars);
                if(resultRead3 == -1){
                    perror("smash error: read failed");}
                int writeRes = write(STDOUT_FILENO, bufferOut, countChars);
                if(writeRes == -1){
                    perror("smash error: write failed");}
            }

        }

    }
}
TouchCommand::TouchCommand(const char* cmd_line, char** args, int position, int specialCharPosition) : BuiltInCommand(cmd_line) {
    if(args[position + 3] != nullptr && position + 3 != specialCharPosition){
        errorMessage = "smash error: touch: invalid arguments\n";
    } else {
        if(args[1] == nullptr || args[2] == nullptr){
            errorMessage = "smash error: touch: invalid arguments\n";
        } else {
            fileName = args[1];
            timeString = args[2];
        }
    }
}

void TouchCommand::execute() {
    if(errorMessage.empty()){
        struct std::tm time{};
        strptime(timeString.c_str(), "%S:%M:%H:%d:%m:%Y", &time);
        struct utimbuf timeBuffer{};
        std::time_t timestamp = mktime(&time);
        timeBuffer.modtime = timestamp;
        if(utime(fileName.c_str(), &timeBuffer) == -1){
            perror("smash error: touch failed");
        }
    } else {
        cerr << errorMessage;
    }
}