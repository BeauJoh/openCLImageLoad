#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bmp.h"

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <OpenCL/opencl.h>

#define FATAL(msg)\
do {\
fprintf(stderr,"FATAL [%s:%d]:%s:%s\n", __FILE__, __LINE__, msg, strerror(errno)); \
assert(0); \
} while(0)

#define SRC 1
#define DST 2

#define BMP_SIZE 14
#define DIB_SIZE 40

#define COEFS_SIZE sizeof(float)*9

struct rgb_8 {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

const float vert[3][3] = { {1.0, 0.0, -1.0}, {2.0, 0.0, -2.0}, {1.0, 0.0, -1.0}};
const float horz[3][3] = { {1.0, 2.0, 1.0}, {0.0, 0.0, 0.0}, {-1.0, -2.0, -1.0}};

static char *load_program_source(const char *filename)
{
    struct stat statbuf;
    FILE        *fh;
    char        *source;
	
    fh = fopen(filename, "r");
    if (fh == 0)
        return 0;
	
    stat(filename, &statbuf);
    source = (char *) malloc(statbuf.st_size + 1);
    fread(source, statbuf.st_size, 1, fh);
    source[statbuf.st_size] = '\0';
	
    return source;
}

void read_image(char *file, struct bmp_header *bmp, struct dib_header *dib, uint8_t **data, uint8_t **palete)
{
	size_t palete_size;
	int fd;
	char *KernelSource;
	
	if((fd = open(file, O_RDONLY)) < 0)
		FATAL("Open Source");
	
	if(read(fd, bmp, BMP_SIZE) != BMP_SIZE)
		FATAL("Read BMP Header");
	
	if(read(fd, dib, DIB_SIZE) != DIB_SIZE)
		FATAL("Read DIB Header");
	
	assert(dib->bpp == 8);
	
	palete_size = bmp->offset - BMP_SIZE - DIB_SIZE;
	if(palete_size > 0) {
		*palete = (uint8_t *)malloc(palete_size);
		if(read(fd, *palete, palete_size) != palete_size)
			FATAL("Read Palete");
	}
	
	*data = (uint8_t *)malloc(dib->image_size);
	if(read(fd, *data, dib->image_size) != dib->image_size)
		FATAL("Read Image");
	
	close(fd);
}

void write_image(char *file, struct bmp_header *bmp, struct dib_header *dib, uint8_t *data, uint8_t *palete)
{
	size_t palete_size;
	int fd;
	
	palete_size = bmp->offset - BMP_SIZE - DIB_SIZE;
	
	if((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 
				  S_IRUSR | S_IWUSR |S_IRGRP)) < 0)
		FATAL("Open Destination");
	
	if(write(fd, bmp, BMP_SIZE) != BMP_SIZE)
		FATAL("Write BMP Header");
	
	if(write(fd, dib, DIB_SIZE) != DIB_SIZE)
		FATAL("Write BMP Header");
	
	if(palete_size != 0) {
		if(write(fd, palete, palete_size) != palete_size)
			FATAL("Write Palete");
	}
	if(write(fd, data, dib->image_size) != dib->image_size)
		FATAL("Write Image");
	close(fd);
}

int main(int argc, char *argv[])
{
	struct bmp_header bmp;
	struct dib_header dib;
	
	int i, fp;
	uint8_t *palete = NULL;
	uint8_t *data = NULL, *out = NULL;
	float f;
	
	// OpenCL variables
	int err, gpu;                            // error code returned from api calls
	
	size_t global;                      // global domain size for our calculation
	size_t local;                       // local domain size for our calculation
	
	cl_device_id device_id;             // compute device id 
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_kernel kernel;                   // compute kernel
	
	cl_mem input;                       // device memory used for the input array
	cl_mem output;                      // device memory used for the output array
	cl_mem coefs_ver, coefs_hor, lids;
	
	read_image(argv[SRC], &bmp, &dib, &data, &palete);
	
	out = (uint8_t *)malloc(dib.image_size);
	
	// Connect to a compute device
	//
	gpu = 1;
	if(clGetDeviceIDs(NULL, gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL) != CL_SUCCESS)
		FATAL("Failed to create a device group!");
	
	// Create a compute context 
	//
	if(!(context = clCreateContext(0, 1, &device_id, NULL, NULL, &err)))
		FATAL("Failed to create a compute context!");
	
	// Create a command commands
	//
	if(!(commands = clCreateCommandQueue(context, device_id, 0, &err)))
		FATAL("Failed to create a command commands!");
	
	// Load Program source
	//
	char *source = load_program_source("sobel_opt1.cl");
    if(!source)
    {
        printf("Error: Failed to load compute program from file!\n");
        return EXIT_FAILURE;    
    }
	// Create the compute program from the source buffer
	//
	if(!(program = clCreateProgramWithSource(context, 1, (const char **) & source, NULL, &err)))
		FATAL("Failed to create compute program!");
	
	// Build the program executable
	//
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];
		
		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		exit(1);
	}
	
	// Create the compute kernel in the program we wish to run
	//
	kernel = clCreateKernel(program, "sobel", &err);
	if (!kernel || err != CL_SUCCESS)
		FATAL("Failed to create compute kernel!");
	
	// Create the input and output arrays in device memory for our calculation
	//
    
	input = clCreateBuffer(context,  CL_MEM_READ_ONLY,  dib.image_size, NULL, NULL);
	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, dib.image_size, NULL, NULL);
	coefs_ver = clCreateBuffer(context,  CL_MEM_READ_ONLY,  COEFS_SIZE, NULL, NULL);
	coefs_hor = clCreateBuffer(context,  CL_MEM_READ_ONLY,  COEFS_SIZE, NULL, NULL);
	if (!input || !output || !coefs_ver || !coefs_hor)
		FATAL("Failed to allocate device memory!");
	
	// Write our data set into the input array in device memory 
	//
	err = clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, dib.image_size, data, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(commands, coefs_ver, CL_TRUE, 0, COEFS_SIZE, vert, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(commands, coefs_hor, CL_TRUE, 0, COEFS_SIZE, horz, 0, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to write to source array!\n");
		exit(1);
	}
	
	// Set the arguments to our compute kernel
	//
	err = 0;
	err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
	err |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &dib.width);
	err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &dib.height);
	err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &coefs_ver);
	err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &coefs_hor);
	// Shared memory
	err |= clSetKernelArg(kernel, 6, COEFS_SIZE, NULL);
	err |= clSetKernelArg(kernel, 7, COEFS_SIZE, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to set kernel arguments! %d\n", err);
		exit(1);
	}
	
	// Get the maximum work group size for executing the kernel on the device
	//
	err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(int) * 4 + sizeof(float) * 2, &local, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to retrieve kernel work group info! %d\n", err);
		exit(1);
	}
	
	// Execute the kernel over the entire range of our 1d input data set
	// using the maximum number of work group items for this device
	//
	global = dib.width * dib.height;
	//local = 64;
	err = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
	if (err)
	{
		FATAL("Failed to execute kernel!");
	}
	
	// Wait for the command commands to get serviced before reading back results
	//
	clFinish(commands);
	
	// Read back the results from the device to verify the output
	//
	err = clEnqueueReadBuffer(commands, output, CL_TRUE, 0, dib.image_size, out, 0, NULL, NULL );  
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to read output array! %d\n", err);
		exit(1);
	}
	
	// Shutdown and cleanup
	//
	clReleaseMemObject(input);
	clReleaseMemObject(output);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);
	
	write_image(argv[DST], &bmp, &dib, out, palete);
	
	return 0;
}
