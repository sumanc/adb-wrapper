//
//  main.m
//  madb
//
//  Created by Suman Cherukuri on 6/23/20.
//  Copyright © 2020 Suman Cherukuri. All rights reserved.
//

#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

string RunAndGetOutput(string cmd) {
    string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    cmd.append(" 2>&1");
    
    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream)) {
            if (fgets(buffer, max_buffer, stream) != NULL) {
                data.append(buffer);
            }
        }
        pclose(stream);
    }
    return data;
}

bool IsLocalPortFree(int port) {
    string hostname = "127.0.0.1";

    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cout << "ERROR opening socket" << endl;
        return true;
    }

    server = gethostbyname(hostname.c_str());

    if (server == NULL) {
        cout << "ERROR, no such host" << endl;
        return true;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    bool ret = true;
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        ret = true;
    }
    else {
        ret = false;;
    }
    close(sockfd);
    return ret;
}

int FindAPort() {
    //get a port between 6000 and 7000
    int port = 0;
    for (int i = 6000; i < 7000; i++) {
        bool isFree = IsLocalPortFree(i);
        if (isFree) {
            port = i;
            break;
        }
    }
    return port;
}

int main(int argc, const char * argv[]) {
    string device = "";
    stringstream commandArgs;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-s") {
            if (i + 1 < argc) {
                device = argv[i + 1];
            }
        }

        commandArgs << argv[i] << " ";
    }
    
    if (device == "") {
        string command = "adb " + commandArgs.str();
        system(command.c_str());
        std::size_t found = command.find("devices");
        if (found != std::string::npos) {
            int port = FindAPort();
            if (port == 0) {
                return 0;
            }
            
            stringstream startCommand;
            startCommand << "adb -P " << port << " devices";
            string devicesOut = RunAndGetOutput(startCommand.str().c_str());
            char *pch;
            pch = strtok((char *)(devicesOut.c_str()), "\n");
            while (pch != NULL) {
                string pchStr = string(pch);
                if (pchStr != "List of devices attached" &&
                    pchStr.rfind("* daemon", 0) != 0) {
                    cout << pch << endl;
                }
                pch = strtok(NULL, "\n");
            }
            
            stringstream killCommand;
            killCommand << "adb -P " << port << " kill-server";
            system(killCommand.str().c_str());
        }
    }
    else {
        // make sure device is available in this adb server
        string devices = RunAndGetOutput("adb devices");
        char *pch;
        pch = strtok((char *)(devices.c_str()), "\n");
        while (pch != NULL) {
            if (device == pch) {
                //found the target device
                string command = "adb " + commandArgs.str();
                system(command.c_str());
                return 0;
            }
            pch = strtok(NULL, "\n\t");
        }

        int port = FindAPort();
        
        if (port == 0) {
            cout << "*** No free port found to run adb server ***" << endl;
            return -1;
        }
        
        stringstream startCommand;
        startCommand << "adb -P " << port << " " << commandArgs.str();
        system(startCommand.str().c_str());
        
        stringstream killCommand;
        killCommand << "adb -P " << port << " kill-server";
        system(killCommand.str().c_str());
    }
    return 0;
}
