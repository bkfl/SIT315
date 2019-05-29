#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <CL/cl.h>
#include <chrono>

using namespace std;
using namespace chrono;

int debug_count = 0;

// forward declarations
cl_device_id create_cl_device();
cl_program create_cl_program(cl_context, cl_device_id, const char*);

int main(int argc, char **argv) {

    int el_count = 10;
    if (argc > 1) el_count = atoi(argv[1]);
    
    int max_el = el_count * 2;

    cl_int              err;
    cl_device_id        device;
    cl_context          context;
    cl_program          program;
    cl_command_queue    queue;
    cl_kernel           kernel;
    cl_mem              buf;

    int arr[el_count];
    /*
    printf("Unsorted: ");
    for (int i = 0; i < el_count; i++) {
        arr[i] = rand() % max_el;
        printf("%d ", arr[i]);
    }
    printf("\n");
    */

    // create device
    device = create_cl_device();

    // create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err < 0) {
        perror("Failed to create context. Exiting...\n");
        exit(1);
    }

    // create program from source file
    program = create_cl_program(context, device, "qs-kernel.cl");

    // create command queue
    queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
    if (err < 0) {
        perror("Failed to create command queue. Exiting...\n");
        exit(1);
    }

    // create kernel
    kernel = clCreateKernel(program, "quicksort", &err);
    if (err < 0) {
        perror("Failed to create kernel. Exiting...\n");
        exit(1);
    }

    buf = clCreateBuffer(context, CL_MEM_READ_ONLY,  el_count*sizeof(int), NULL, NULL);
    clEnqueueWriteBuffer(queue, buf, CL_TRUE, 0, el_count*sizeof(int), arr, 0, NULL, NULL);  

    int low = 0;
    int high = el_count - 1;

    clSetKernelArg(kernel, 0, sizeof(cl_command_queue), (void*)&queue);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&buf);
    clSetKernelArg(kernel, 2, sizeof(int), (void*)&low);
    clSetKernelArg(kernel, 3, sizeof(int), (void*)&high);

    // execute quicksort
    // start timing
    high_resolution_clock::time_point t_start = high_resolution_clock::now();
    clEnqueueTask(queue, kernel, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, buf, CL_TRUE, 0, el_count*sizeof(int), arr, 0, NULL, NULL);
    // stop timing
        high_resolution_clock::time_point t_stop = high_resolution_clock::now();

        // calculate execution time
        duration<double> exec_time = duration_cast<duration<double>>(t_stop - t_start);
        cout << "Sort completed in " << exec_time.count() << " seconds." << endl;
    
}

cl_device_id create_cl_device() {
    
    cl_int          err;
    cl_platform_id  platform;
    cl_device_id    device;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0) {
        perror("Failed to identify a platform. Exiting...\n");
        exit(1);
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err < 0) {
        perror("Failed to access GPU device. Attempting CPU...\n");
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    }
    if (err < 0) {
        perror("Failed to access CPU device. No devices available. Exiting...\n");
        exit(1);
    }

    return device;
}

cl_program create_cl_program(cl_context context, cl_device_id device, const char* filename) {
    
    FILE        *fp;
    char        *program_buffer, *program_log;
    size_t      program_size, log_size;
    cl_int      err;
    cl_program  program;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Failed to open program file. Exiting...\n");
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    program_size = ftell(fp);
    rewind(fp);

    program_buffer = (char*)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, fp);
    fclose(fp);

    program = clCreateProgramWithSource(context, 1, (const char**)&program_buffer, &program_size, &err);
    if (err < 0) {
        perror("Failed to create program. Exiting...\n");
        exit(1);
    }
    free(program_buffer);

    err = clBuildProgram(program, 0, NULL, "-cl-std=CL2.0", NULL, NULL);
    if (err < 0) {
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        
        program_log = (char*)malloc(log_size + 1);
        program_log[log_size] = '\0';
        
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
        
        printf("%s\n", program_log);
        
        free(program_log);
        exit(1);
    }

    return program;
}