#include <iostream>
#include <stdlib.h>
#include <chrono>
#include <mpi.h>
#include <CL/cl.h>

using namespace std;
using namespace chrono;

// forward declarations
void quickSort(int*, int, int);
cl_device_id create_cl_device();
cl_program create_cl_program(cl_context, cl_device_id, const char*);

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int el_count = 10;
    if (argc > 1) el_count = atoi(argv[1]);

    int max_el = el_count * 10;

    // get world size
    int np;
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    
    // get my rank
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // enforce 2 nodes
    if (np != 2)
    {
        if (rank == 0)
            perror("Error: Number of MPI nodes must be 2. Exiting... ");
        
        exit(1);
    }

    // calculate send counts and displacements
    int elems_per_process = el_count / 2;
    
    int send_counts[2] = {elems_per_process, elems_per_process};
    if (el_count % 2 == 1) send_counts[0] += 1;

    int displacements[2] = {0, send_counts[0]};

    int *data;
    int my_el_count = send_counts[rank];

    if (rank == 0) {
        
        // generate random array
        int* data = (int*)malloc(el_count * sizeof(int));
        for (int i = 0; i < el_count; i++) {
            data[i] = rand() % (max_el - 1) + 1;
        }

        // start timing
        high_resolution_clock::time_point t_start = high_resolution_clock::now();

        // scatter (send) data
        MPI_Scatterv(data, send_counts, displacements, MPI_INT, MPI_IN_PLACE, send_counts[rank], MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // quicksort my elements
        quickSort(data, 0, my_el_count - 1);

        // gather (receive) data
        MPI_Gatherv(MPI_IN_PLACE, send_counts[rank], MPI_INT, data, send_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // merge sorted data
        int* sorted = (int*)malloc(el_count * sizeof(int));
        int u1 = 0, u2 = displacements[1], s = 0;
        while (u1 < displacements[1] && u2 < el_count)
        {
            if (data[u1] <= data[u2])
                sorted[s++] = data[u1++];
            else
                sorted[s++] = data[u2++];
        }

        while (u1 < displacements[1])
            sorted[s++] = data[u1++];

        while (u2 < el_count)
            sorted[s++] = data[u2++];

        // stop timing
        high_resolution_clock::time_point t_stop = high_resolution_clock::now();

        // calculate execution time
        duration<double> exec_time = duration_cast<duration<double>>(t_stop - t_start);
        cout << "Sort completed in " << exec_time.count() << " seconds." << endl;

        // Validity Test (10 Samples)
        int span = el_count / 10;
        bool fail = false;
        printf("Validity Test (10 Samples): \n");
        for (int i = 0; i < 10; i++) {
            printf("%d ", sorted[i*span]);
            if (i > 0) {
                if (sorted[i*span] < sorted[(i-1)*span]) {
                    printf("<-- FAIL");
                    fail = true;
                    break;
                }
            }
        }
        if (fail == false) {
            printf(": PASS");
        }
        printf("\n");

        // clean up data
        free(sorted);
        free(data);
    }
    else
    {
        int* data = (int*)malloc(my_el_count * sizeof(int));

        // scatter (receive) data
        MPI_Scatterv(data, send_counts, displacements, MPI_INT, data, send_counts[rank], MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // quicksort my elements
        quickSort(data, 0, my_el_count - 1);

        // gather (send) data
        MPI_Gatherv(data, send_counts[rank], MPI_INT, data, send_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // clean up memory
        free(data);
    }

    MPI_Finalize();
    return 0;
}

void quickSort(int* data, int low, int high) {

    cl_int              err;
    cl_device_id        device;
    cl_context          context;
    cl_program          program;
    cl_command_queue    queue;
    cl_kernel           kernel;
    cl_mem              buf;

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

    buf = clCreateBuffer(context, CL_MEM_READ_ONLY,  (high + 1)*sizeof(int), NULL, NULL);
    clEnqueueWriteBuffer(queue, buf, CL_TRUE, 0, (high + 1)*sizeof(int), data, 0, NULL, NULL);  

    clSetKernelArg(kernel, 0, sizeof(cl_command_queue), (void*)&queue);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&buf);
    clSetKernelArg(kernel, 2, sizeof(int), (void*)&low);
    clSetKernelArg(kernel, 3, sizeof(int), (void*)&high);

    // execute quicksort
    clEnqueueTask(queue, kernel, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, buf, CL_TRUE, 0, (high + 1)*sizeof(int), data, 0, NULL, NULL);
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