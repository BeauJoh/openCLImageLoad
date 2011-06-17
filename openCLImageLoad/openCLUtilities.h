//
//  openCLUtilities.h
//  openCLImageLoad
//
//  Created by Beau Johnston on 17/06/11.
//  Copyright 2011 University Of New England. All rights reserved.
//

#ifndef OPENCL_UTILITIES
#define OPENCL_UTILITIES

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

// Special automatic include statement, 
// openCL is dependent on specific libraries, depending on the OS
#if defined (__APPLE__)  && defined (__MACH__)
//macOSX openCL Framework
#include <OpenCL/opencl.h>
#else
//it must be for Tukey
//linux openCL Library
//#include <CL/oclUtils.h>
#include <CL/opencl.h>
//#include <tiff.h>
#endif

#define FATAL(msg)\
do {\
fprintf(stderr,"FATAL [%s:%d]:%s:%s\n", __FILE__, __LINE__, msg, strerror(errno)); \
assert(0); \
} while(0)

#define SRC 1
#define DST 2


char *print_cl_errstring(cl_int err);
cl_bool there_was_an_error(cl_int err);
void getGPUUnitSupportedImageFormats(cl_context context);
char *load_program_source(const char *filename);


#endif
