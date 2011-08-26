//
//  FileHandler.h
//  openCLImageLoad
//
//  Created by Beau Johnston on 25/08/11.
//  Copyright 2011 University Of New England. All rights reserved.
//

#ifndef openCLImageLoad_FileHandler_h
#define openCLImageLoad_FileHandler_h

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>


void generateListOfAssociatedFiles(char* filename);
char* getNextFileName(void);
bool areFilesLeft(void);
int numberOfFiles(void);

#endif
