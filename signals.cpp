#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	cout << "smash: got ctrl-Z\n";
    SmallShell &smash = SmallShell::getInstance();

    int foregroundCmdPid = smash.getForegroundCommandPid();
    if(foregroundCmdPid != -1){
        kill(foregroundCmdPid, SIGSTOP);
        cout << "smash: process " << foregroundCmdPid << " was stopped\n";
        smash.pushToBackground();
        smash.clearForegroundJob();
    }
}

void ctrlCHandler(int sig_num) {
  	cout << "smash: got ctrl-C\n";
    SmallShell &smash = SmallShell::getInstance();

    int foregroundCmdPid = smash.getForegroundCommandPid();
    if(foregroundCmdPid != -1){
        kill(foregroundCmdPid, SIGKILL);
        cout << "smash: process " << foregroundCmdPid << " was killed\n";
        smash.clearForegroundJob();
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

