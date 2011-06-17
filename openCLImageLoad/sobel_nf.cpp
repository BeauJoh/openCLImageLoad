#include "io_tiff.h"
#include "openCLUtilities.h"

int main(int argc, char *argv[])
{

	uint16 *data = NULL, *out = NULL;
	
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
	
    data = read_tiff((char*)"GMARBLES.tif");
    
    //data = normalizeData(data);
    
	out = new uint16[getSamplesPerPixel() * getImageWidth() * getImageLength()];
	
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
	char *source = load_program_source("sobel_opt1.cl");
    if(!source)
    {
        printf("Error: Failed to load compute program from file!\n");
        return EXIT_FAILURE;    
    }
	// Create the compute program from the source buffer
	//
	if(!(program = clCreateProgramWithSource(context, 1, (const char **) &source, NULL, &err)))
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
    
	//input = clCreateBuffer(context,  CL_MEM_READ_ONLY,  dib.image_size, NULL, NULL);
	//output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, dib.image_size, NULL, NULL);

    getGPUUnitSupportedImageFormats(context);
    
    // specify the image format that the images a represented with
	cl_image_format format;
	format.image_channel_data_type = CL_UNORM_INT16;
	format.image_channel_order = CL_INTENSITY;
    
//    format.image_channel_data_type = CL_UNSIGNED_INT8;
//	format.image_channel_order = CL_RGBA;

    //CL_UNORM_INT8 = Each channel component is a normalized unsigned 8-bit integer value.
    //CL_UNORM_INT16 = Each channel component is a normalized unsigned 16-bit integer value.

    
    //cl_image_format format = {CL_INTENSITY, CL_UNORM_INT16};
    
    //CL_UNSIGNED_INT16 would be ideal Each channel component is an unnormalized unsigned 16-bit integer value. But not compatible for image_channel_order of  CL_INTENSITY
    
    if(!data){
        printf("the input image is empty\n");
        return -1;
    }
    
//    input = clCreateImage2D(context, CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, &format, getImageWidth(), getImageLength(), getImageRowPitch(), data, &err); 
    input = clCreateImage2D(context, CL_MEM_ALLOC_HOST_PTR, &format, getImageWidth(), getImageLength(), 0, NULL, &err); 
    
    if(there_was_an_error(err)){
        printf("clCreateImage2D for input failed\n");
        return -1;
    }
    
//    output = clCreateImage2D(context, CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR, &format, getImageWidth(), getImageLength(), getImageRowPitch(), out, &err);
//
//    if(there_was_an_error(err)){
//        printf("clCreateImage2D for output failed\n");
//        return -1;
//    }

    
	if (!input || !output )
		FATAL("Failed to allocate device memory!");
	
    
	// Write our data set into the input array in device memory
    const size_t origin[3] = {0, 0, 0};
    const size_t region[3] = {getImageWidth(), getImageLength(), 1}; 
    
    err = clEnqueueWriteImage(commands, input, CL_TRUE, origin, region, 0, 0, data, 0, NULL, NULL);

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

	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to set kernel arguments! %d\n", err);
		exit(1);
	}

	// Get the maximum work group size for executing the kernel on the device
	//
//	err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(int) * 4 + sizeof(float) * 2, &local, NULL);

	err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(unsigned short)* getImageLength()*getImageWidth(), &local, NULL);

	if (err != CL_SUCCESS)
	{
        printf("%s\n", print_cl_errstring(err));
        if(err == CL_INVALID_VALUE){
            printf("if param_name is not valid, or if size in bytes specified by param_value_size \n");
            printf("is less than the size of return type as described in the table above and \n");
            printf("param_value is not NULL.");
        }
		printf("Error: Failed to retrieve kernel work group info! %d\n", err);
		exit(1);
	}
	
	// Execute the kernel over the entire range of our 1d input data set
	// using the maximum number of work group items for this device
	//
	global =sizeof(unsigned short)* getImageLength()*getImageWidth();
	//local = 64;
    
	err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, &global, &local, 0, NULL, NULL);
	if (err)
	{
        printf("%s\n", print_cl_errstring(err));
		printf("Failed to execute kernel!, %d\n", err);
        exit(1);
	}
	
	// Wait for the command commands to get serviced before reading back results
	//
	clFinish(commands);
	
	// Read back the results from the device to verify the output
	//

    //collect results
//    ciErr1 = clEnqueueReadImage (
//                                 cqCommandQue,
//                                 myClImage2,     //              cl_mem image,
//                                 CL_TRUE,        //              cl_bool blocking_read,
//                                 origin,         //              const size_t origin[3],
//                                 region,         //              const size_t region[3],
//                                 0,                      //              size_t row_pitch,
//                                 0,                      //              size_t slice_pitch,
//                                 image2,         //              void *ptr,
//                                 0,                      //              cl_uint num_events_in_wait_list,
//                                 NULL,           //              const cl_event *event_wait_list,
//                                 NULL            //              cl_event *event)
//                                 
	err = clEnqueueReadImage(commands, output, CL_TRUE, origin, region, getImageRowPitch(), getImageSlicePitch(), out, 0, NULL, NULL);  
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to read output array! %d\n", err);
		printf("%s\n", print_cl_errstring(err));
        clReleaseMemObject(input);
        clReleaseMemObject(output);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
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
	
//write image here	
	return 0;
}
