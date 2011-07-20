//#include "io_tiff.h"
#include "openCLUtilities.h"
#include "RGBAUtilities.h"

#include <iostream>
using namespace std;

uint8 *inData, *outData;

// OpenCL variables
int err, gpu;                            // error code returned from api calls

size_t global;                      // global domain size for our calculation
size_t local;                       // local domain size for our calculation

cl_device_id device_id;             // compute device id 
cl_context context;                 // compute context
cl_command_queue commands;          // compute command queue
cl_program program;                 // compute program
cl_kernel kernel;                   // compute kernel
cl_sampler sampler;
cl_mem input;                       // device memory used for the input array
cl_mem output;                      // device memory used for the output array
int width;
int height;                  //input and output image specs

void cleanKill(int errNumber){
    clReleaseMemObject(input);
	clReleaseMemObject(output);
	clReleaseProgram(program);
    clReleaseSampler(sampler);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);
    exit(errNumber);
}

int main(int argc, char *argv[])
{
    //read_png_file((char*)"rgba.png");
    
    //inData = getImage();

    //inData = read_tiff((char*)"GMARBLES.tif");
    
    //data = normalizeData(data);
    
	//out = new uint16[getSamplesPerPixel() * getImageWidth() * getImageLength()];
	
	// Connect to a compute device
	//
	gpu = 1;
	if(clGetDeviceIDs(NULL, gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL) != CL_SUCCESS)  {
		cout << "Failed to create a device group!" << endl;
        cleanKill(EXIT_FAILURE);
    }
	
	// Create a compute context 
	//
	if(!(context = clCreateContext(0, 1, &device_id, NULL, NULL, &err))){
		cout << "Failed to create a compute context!" << endl;
        cleanKill(EXIT_FAILURE);
    }
	
	// Create a command commands
	//
	if(!(commands = clCreateCommandQueue(context, device_id, 0, &err))) {
		cout << "Failed to create a command commands!" << endl;
        cleanKill(EXIT_FAILURE);
    }
    
    // Load kernel source code
    //
	char *source = load_program_source("sobel_opt1.cl");
    if(!source)
    {
        cout << "Error: Failed to load compute program from file!" << endl;
        cleanKill(EXIT_FAILURE);
    }
    
	// Create the compute program from the source buffer
	//
	if(!(program = clCreateProgramWithSource(context, 1, (const char **) &source, NULL, &err))){
		cout << "Failed to create compute program!" << endl;
        cleanKill(EXIT_FAILURE);
    }
    
	// Build the program executable
	//
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];
		cout << "Error: Failed to build program executable!" << endl;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		cout << buffer << endl;
        cleanKill(EXIT_FAILURE);
	}
    
    if(!doesGPUSupportImageObjects){
        cleanKill(EXIT_FAILURE);
    }
	
	// Create the compute kernel in the program we wish to run
	//
	kernel = clCreateKernel(program, "sobel", &err);
    
	if (!kernel || err != CL_SUCCESS){
		cout << "Failed to create compute kernel!" << endl;
        cleanKill(EXIT_FAILURE);
    }
	
//    getGPUUnitSupportedImageFormats(context);
    
    // specify the image format that the images a represented with
	cl_image_format alternateImageFormat;
	alternateImageFormat.image_channel_data_type = CL_UNSIGNED_INT16;
	alternateImageFormat.image_channel_order = CL_A;
    
    // Create ouput image object 
    cl_image_format format; 
    format.image_channel_order = CL_RGBA; 
    format.image_channel_data_type = CL_UNORM_INT8;
    
    
//    format.image_channel_data_type = CL_UNSIGNED_INT8;
//	format.image_channel_order = CL_RGBA;

    //CL_UNORM_INT8 = Each channel component is a normalized unsigned 8-bit integer value.
    //CL_UNORM_INT16 = Each channel component is a normalized unsigned 16-bit integer value.

    
    //cl_image_format format = {CL_INTENSITY, CL_UNORM_INT16};
    
    //CL_UNSIGNED_INT16 would be ideal Each channel component is an unnormalized unsigned 16-bit integer value. But not compatible for image_channel_order of  CL_INTENSITY
    
//    if(!inData){
//        cout << "The input image is empty" << endl;
//        cleanKill(EXIT_FAILURE);
//    }
    
//    width = getImageWidth();
//    height = getImageLength();
    
//    uint16 *realInData[4];
//    realInData[0]=inData;
//    realInData[1]=inData;
//    realInData[2]=inData;
//    realInData[3]=inData;

//    input = clCreateImage2D(context, 
//                            CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, 
//                            &format, 
//                            width, 
//                            height, 
//                            getImageRowPitch(), 
//                            inData, 
//                                &err); 
//    
//    if(there_was_an_error(err)){
//        cout << "Input Image Buffer creation error!" << endl;
//        cleanKill(EXIT_FAILURE);
//    }
    
    //getGPUUnitSupportedImageFormats(context);
    
    input = LoadImage(context, (char*)"rgba.png", width, height);
    
    cout << "Image is " << width << " wide and " << height << " high" << endl;
    
    output = clCreateImage2D(context, 
                             CL_MEM_WRITE_ONLY, 
                             &format, 
                             width, 
                             height,
                             0, 
                             NULL, 
                             &err);

    if(there_was_an_error(err)){
        cout << "Output Image Buffer creation error!" << endl;
        cleanKill(EXIT_FAILURE);
    }    
    
	if (!input || !output ){
		cout << "Failed to allocate device memory!" << endl;
        cleanKill(EXIT_FAILURE);
	}
    
    char *buffer = new char [width * height * 4];
    size_t origin[3] = { 0, 0, 0 };
    size_t region[3] = { width, height, 1};
    
    cl_command_queue queue = clCreateCommandQueue(
                                                  context, 
                                                  device_id, 
                                                  0, 
                                                  &err);
    
    err = clEnqueueReadImage(queue, input,
                       CL_TRUE, origin, region, 0, 0, buffer, 0, NULL, NULL);
    
    SaveImage("outRGBA.png", (char*)buffer, width, height);
	// Write our data set into the input array in device memory
//    const size_t origin[3] = {0, 0, 0};
//    const size_t region[3] = {getImageWidth(), getImageLength(), 1}; 
//    
    
    // Create sampler for sampling image object 
    sampler = clCreateSampler(context,
                              CL_FALSE, // Non-normalized coordinates 
                              CL_ADDRESS_CLAMP_TO_EDGE, 
                              CL_FILTER_NEAREST, 
                              &err);
    
    if(there_was_an_error(err)){
        cout << "Error creating CL sampler object." << endl;
        cleanKill(EXIT_FAILURE);
    }
    
//    err = clEnqueueWriteImage(commands, input, CL_TRUE, origin, region, 0, 0, data, 0, NULL, NULL);
//
//	if (err != CL_SUCCESS)
//	{
//		cout << "Error: Failed to write to source array!" << endl;
//		cleanKill(EXIT_FAILURE);
//	}
//    
    
    
	// Set the arguments to our compute kernel
	//
	err = 0;
	err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_sampler), &sampler); 
    err |= clSetKernelArg(kernel, 3, sizeof(cl_int), &width);
    err |= clSetKernelArg(kernel, 4, sizeof(cl_int), &height);
    
    if(there_was_an_error(err)){
        cout << "Error: Failed to set kernel arguments! " << err << endl;
        cleanKill(EXIT_FAILURE);
    }    
    
	// Get the maximum work group size for executing the kernel on the device
	//
    //  err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(int) * 4 + sizeof(float) * 2, &local, NULL);

	err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(unsigned short)* getImageLength()*getImageWidth(), &local, NULL);

	if (err != CL_SUCCESS)
	{
        cout << print_cl_errstring(err) << endl;
        if(err == CL_INVALID_VALUE){
            cout << "if param_name is not valid, or if size in bytes specified by param_value_size "
            << "is less than the size of return type as described in the table above and "
            << "param_value is not NULL." << endl;
        }
		cout << "Error: Failed to retrieve kernel work group info!" << err << endl;
		cleanKill(EXIT_FAILURE);
	}
	
	// Execute the kernel over the entire range of our 1d input data set
	// using the maximum number of work group items for this device
	//
    global = sizeof(unsigned short)* getImageLength()*getImageWidth();
	local = 64;
    
	err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, &global, &local, 0, NULL, NULL);
	if (err)
	{
        cout << print_cl_errstring(err) << endl;
		cout << "Failed to execute kernel!, " << err << endl;
        cleanKill(EXIT_FAILURE);
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
	//err = clEnqueueReadImage(commands, output, CL_TRUE, origin, region, getImageRowPitch(), getImageSlicePitch(), out, 0, NULL, NULL);  
	if (err != CL_SUCCESS)
	{
		cout << "Error: Failed to read output array! " << err << endl;
		cout << print_cl_errstring(err) << endl;
        cleanKill(EXIT_FAILURE);
	}
	
	// Shutdown and cleanup
	//
	cleanKill(EXIT_SUCCESS);
	
//write image here	
}