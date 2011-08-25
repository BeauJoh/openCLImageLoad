//
//  FileHandler.cpp
//  openCLImageLoad
//
//  Created by Beau Johnston on 25/08/11.
//  Copyright 2011 University Of New England. All rights reserved.
//

#include "FileHandler.h"

using namespace std;

int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    
    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

int getFilesInDirectoryWithName (string dir, string name, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    
    while ((dirp = readdir(dp)) != NULL) {
        if (string::npos != string(dirp->d_name).find(name)) {
            files.push_back(string(dirp->d_name));
        }
    }
    closedir(dp);
    return 0;
}

void getListOfAssociatedFiles(char* filename)  
{
    
    string handedInString = filename;
    string file = handedInString.substr(handedInString.find_last_of('/')+1);
    string path = handedInString.substr(0, handedInString.find_last_of('/')+1);
    
    string cutDownFile = file.substr(0, file.find_last_of('.'));
    string extension = file.substr(file.find_last_of('.'));
    
    cout << "file has name: " << file << endl;
    cout << "path looks like this: " << path << endl;
    cout << "cut down file looks like this: " << cutDownFile << endl;
    cout << "extension looks like: " << extension << endl;
    
    string dir = string(path);
    vector<string> files = vector<string>();
    
    getFilesInDirectoryWithName(dir,cutDownFile,files);
    
    for (unsigned int i = 0;i < files.size();i++) {
        cout << files[i] << endl;
    }
    
    return;
}
