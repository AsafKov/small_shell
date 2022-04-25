#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using namespace std;

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
public:
    int getPid() const{ return pid; }
    explicit ExternalCommand(const char* cmd_line) : Command(cmd_line){}
    virtual ~ExternalCommand() {}
    void execute() override;
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
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
    string newDir;
    string errorMessage;
public:
    ChangeDirCommand(const char *cmd_line, char** args): BuiltInCommand(cmd_line) {
        if (args[1] == nullptr) {
            errorMessage = "smash error: cd: invalid arguments\n";
        } else {
            if(args[2] != nullptr){
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
    string output;
public:
    explicit GetCurrDirCommand(const char* cmd_line, char** args);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
private:
    string newPrompt;
    string errorMessage;
    bool isValid;
public:
    explicit ChangePromptCommand(const char* cmd_line, char** args): BuiltInCommand(cmd_line) {
        isValid = true;
        if (args[2] != nullptr) {
            isValid = false;
            errorMessage = "smash error: chprompt: too many arguments\n";
        } else {
            if (args[1] != nullptr) {
                newPrompt = args[1];
            }
        }
    }
    ~ChangePromptCommand() override = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
    string output;
public:
    ShowPidCommand(const char* cmd_line, char** args);
    ~ShowPidCommand() override = default;
    void execute() override;
};

class JobsList;
//class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
  //  QuitCommand(const char* cmd_line, JobsList* jobs);
    //virtual ~QuitCommand() {}
    //void execute() override;
//};

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
private:
    int pid;
    int initTime;
    int endTime;
public:
    int getPid(){ return pid; }
    int getInitTime(){ return initTime; }
    void setEndTime(int time){ this->endTime = time;}
    int getEndTime(){ return endTime; }
    BackgroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
    string errorMessage;
    int sigNum, jobId;
    bool isKill;
public:
    QuitCommand(const char *cmdLine, char **args);
    virtual ~QuitCommand() {}
    void execute() override;
};

class TailCommand : public BuiltInCommand {
public:
    TailCommand(const char* cmd_line);
    virtual ~TailCommand() {}
    void execute() override;
};

class TouchCommand : public BuiltInCommand {
public:
    TouchCommand(const char* cmd_line);
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
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    string getPrompt(){ return prompt; }
    int getJobPid(int jobId){
        return jobList->getJobPid(jobId);
    }
    void addJob(ExternalCommand *command){ jobList->addJob(command); }
    void removeFinishedJobs(){ jobList->removeFinishedJobs(); }
    void changeJobStatus(int jobId, int newStatus){
        jobList->getJobById(jobId)->changeStatus(newStatus);
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

