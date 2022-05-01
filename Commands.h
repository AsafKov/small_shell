#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using namespace std;

const int COMMAND_TYPE_BUILT_IN = 0;
const int COMMAND_TYPE_EXTERNAL = 1;
const int COMMAND_TYPE_BLOCKING = 2;

class Command {
    const string cmd_line;
public:
    Command(const char* cmd_line): cmd_line(cmd_line){}
    const string getCmdLine() const { return cmd_line; }
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const char* cmd_line): Command(cmd_line) {}
    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
private:
    int pid;
    int commandType;
    string arg;
public:
    int getPid() const{ return pid; }
    explicit ExternalCommand(const char* cmd_line, int type, string specialArg) : Command(cmd_line), commandType(type), arg(specialArg){}
    virtual ~ExternalCommand() {}
    void execute() override;
    void executeRedirection();
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    bool isValid;
    string errorMessage;
public:
    explicit RedirectionCommand(const char* cmd_line, int position);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
    string newDir;
    string errorMessage;
public:
    ChangeDirCommand(const char *cmd_line, char** args, int position, int specialCharPosition): BuiltInCommand(cmd_line) {
        if (args[position + 1] == nullptr) {
            errorMessage = "smash error: cd: invalid arguments\n";
        } else {
            if(args[position + 2] != nullptr && position+2 != specialCharPosition){
                errorMessage = "smash error: cd: too many arguments\n";
            } else {
                newDir = args[1];
            }
        }
    }
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
    string errorMessage;
    string output;
public:
    explicit GetCurrDirCommand(const char* cmd_line, char** args, int position, int specialCharPosition);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
private:
    string newPrompt;
    string errorMessage;
    bool isValid;
public:
    explicit ChangePromptCommand(const char* cmd_line, char** args, int position, int specialCharPosition): BuiltInCommand(cmd_line) {
        isValid = true;
        if (args[position + 1] != nullptr) {
            newPrompt = args[1];
        }
    }
    ~ChangePromptCommand() override = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
    string errorMessage;
    string output;
    string fileName;
public:
    ShowPidCommand(const char* cmd_line, char** args, int position, int specialCharPosition);
    ~ShowPidCommand() override = default;
    void execute() override;
};

class JobsList;


class JobsList {
public:
    static const int STATUS_ACTIVE = 0;
    static const int STATUS_STOPPED = 1;
    static const int STATUS_KILLED = 2;

    static const int EMPTY_LIST = 0;
    static const int NOT_FOUND = -1;
    class JobEntry {
        const int jobId;
         time_t initTime;
        const ExternalCommand* command;
        int status = STATUS_ACTIVE; // 0-active, 1-stopped, 2-killed

    public:
        JobEntry(int id, ExternalCommand *issuedCommand, int cmdStatus): jobId(id), command(issuedCommand), status(cmdStatus){}
        const ExternalCommand *getCommand() const{ return command; }
        int getId() const{ return jobId; }
        time_t getInitTime() const{ return initTime; }
        void setInitTIme(time_t time){ this->initTime = time; }
        int getStatus() const{ return status; }
        void changeStatus(int newStatus){ status = newStatus; }
    };

    int currentMaxId = 0;
    vector<JobEntry *> jobs;
    JobEntry *foregroundJob = nullptr;

public:
    JobsList(){}
    ~JobsList() = default;
    void addJob(ExternalCommand* cmd, bool isStopped = false);
    void printJobsList();
    void killAll();
    void clearForegroundJob(){
        if(foregroundJob != nullptr){
            removeJobById(foregroundJob->getId());
            foregroundJob = nullptr;
        }
    }
    ExternalCommand* pushToForeground(int jobId, int *id){
        if(jobs.empty()){
            *id = EMPTY_LIST;
            return nullptr;
        }
        JobEntry *jobEntry;
        if(jobId == 0){
            jobEntry = getLastJob(id);
        } else {
            jobEntry = getJobById(jobId);
        }

        if(jobEntry != nullptr){
            *id = jobEntry->getId();
            foregroundJob = jobEntry;
        }
        return jobEntry != nullptr? (ExternalCommand*)jobEntry->getCommand(): nullptr;
    }
    ExternalCommand* resumeStopped(int jobId, int *id){
        if(jobs.empty()){
            *id = EMPTY_LIST;
            return nullptr;
        }
        JobEntry *jobEntry;
        if(jobId == 0){
            jobEntry = getLastStoppedJob(id);
        } else {
            jobEntry = getJobById(jobId);
        }

        if(jobEntry != nullptr){
            *id = jobEntry->getId();
            if (jobEntry->getStatus()!=STATUS_STOPPED)
            {
                cout << "smash error: bg: job-id " << jobId << "  is already running in the background\n";
            }
            jobEntry->changeStatus(STATUS_ACTIVE);
        }
        return jobEntry != nullptr? (ExternalCommand*)jobEntry->getCommand(): nullptr;
    }
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    int getJobPid(int jobId){
        JobEntry *entry = getJobById(jobId);
        return entry != nullptr? entry->getCommand()->getPid():-1;
    }
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify existing ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    string errorMessage;
    int sigNum, jobId;
public:
    KillCommand(const char *cmdLine, char **args);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    static const int LATEST_JOB = 0;
    int jobId;
    string errorMessage;
public:
    ForegroundCommand(const char* cmd_line, char **args);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    static const int LATEST_JOB = 0;
    int jobId;
    string errorMessage;
public:
    BackgroundCommand(const char* cmd_line, char **args);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
    string errorMessage;
    bool isKill;
public:
    QuitCommand(const char *cmdLine, char **args);
    virtual ~QuitCommand() {}
    void execute() override;
};

class TailCommand : public BuiltInCommand {
    int numLines;
    string errorMessage;
    string fileName;

public:
    TailCommand(const char* cmd_line, char **args);
    virtual ~TailCommand() {}
    void execute() override;
};

class TouchCommand : public BuiltInCommand {
    string errorMessage;
    string fileName;
    string timeString;
public:
    TouchCommand(const char* cmd_line, char** args, int position, int specialCharPosition);
    virtual ~TouchCommand() {}
    void execute() override;
};


class SmallShell {
private:
    string prompt;
    string prevDirectory;
    JobsList *jobList;
    ExternalCommand *foregroundCommand;
    bool isRunning;
    SmallShell();
public:
    Command *CreateCommand(const char* cmd_line, int *commandType, string &specialArg, int *isBuiltIn, string *commandAfterSplit);
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    string getPrompt(){ return prompt; }
    void redirectStdout(Command *cmd, const string& specialArg, int redirectionType);
    int getJobPid(int jobId){
        return jobList->getJobPid(jobId);
    }
    void addJob(ExternalCommand *command){ jobList->addJob(command); }
    void removeFinishedJobs(){ jobList->removeFinishedJobs(); }
    void changeJobStatus(int jobId, int newStatus){
        jobList->getJobById(jobId)->changeStatus(newStatus);
    }
    void removeJobById(int jobId)
    {
        jobList->removeJobById(jobId);
    }
    void printJobs(){ jobList->printJobsList(); }
    int getForegroundCommandPid(){ return foregroundCommand != nullptr? foregroundCommand->getPid():-1; }
    void setForegroundCommand(ExternalCommand *command){ foregroundCommand = command; }
    void clearForegroundCommand(){
        delete foregroundCommand;
        foregroundCommand = nullptr;
    }
    void clearForegroundJob(){
        jobList->clearForegroundJob();
        clearForegroundCommand();
    }
    void killAll()
    {
        jobList->killAll();
        setIsRunning();
    }

    void pushToBackground(){
        jobList->addJob(foregroundCommand, true);
        foregroundCommand = nullptr;
    }

    ExternalCommand *pushToForeground(int jobId, int *id){
        // TODO: What happens if stopped command is pushed to foreground? Continue?
        ExternalCommand *cmd = jobList->pushToForeground(jobId, id);
        if(cmd != nullptr){
            foregroundCommand = cmd;
        }
        return cmd;
    }
    ExternalCommand *resumeStopped(int jobId, int *id){
        ExternalCommand *cmd = jobList->resumeStopped(jobId, id);
        if(cmd != nullptr){
            changeJobStatus(jobId, 0);
        }
        return cmd;
    }
    string getPrevDirectory(){ return prevDirectory; }
    void setPrevDirectory(const string& prev){ prevDirectory = prev; }
    void setPrompt(string newPrompt = "smash"){ prompt =  std::move(newPrompt); }
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    void setIsRunning()
    {
        this->isRunning= false;
    }
    bool getIsRunning()
    {
        return this->isRunning;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_

