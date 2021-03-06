#include <iostream>
#include <unistd.h>
//#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    std::cout.setf(std::ios::unitbuf);
    SmallShell& smash = SmallShell::getInstance();
//    int line = 1;
    while(smash.getIsRunning()) {
        std::cout << smash.getPrompt() << "> ";
        std::cout.flush();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
//        if(line == 30){
//            int a = 2;
//        }
//        cout<<"\nline: "<<line<<" "<<cmd_line<<"\n";
        smash.executeCommand(cmd_line.c_str());
//        line++;
    }
    return 0;
}