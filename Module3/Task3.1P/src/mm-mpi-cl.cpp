#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <mpi.h>
#include <CL/cl.h>

using namespace std;
using namespace chrono;

// max value for a matrix element
#define MAX_ELEMENT 20

// forward declarations
int** new_matrix(int, int);
int** new_rand_matrix(int, int);
int** new_zero_matrix(int, int);
void free_matrix(int**);
void cl_matrix_mul(int**, int**, int**, size_t, size_t);
cl_device_id create_cl_device();
cl_program create_cl_program(cl_context, cl_device_id, const char*);

int main(int argc, char **argv) {
    
    MPI_Init(&argc, &argv);
    
    //srand(time(NULL));

    // get matrix size (rows, cols)
    int size = 5;
    if (argc > 1) size = atoi(argv[1]);

    // get number of mpi nodes
    int np;
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    // get my rank
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // calculate send counts and displacements
    int send_counts[np] = {0};
    int displacements[np] = {0};

    int base_rows_per_process = size / np;
    int base_elems_per_process = base_rows_per_process * size;
    int rows_remainder = size % np;

    for (int i = 0; i < np; i++) {
        send_counts[i] = base_elems_per_process;
        if (rows_remainder > 0) {
            send_counts[i] += size;
            rows_remainder--;
        }
        if (i < size - 1)
            displacements[i + 1] = displacements[i] + send_counts[i];
    }    

    // define vars for matrix multiplication
    int **a, **b, **c;
    int my_row_count = send_counts[rank] / size;

    if (rank == 0) {
        // init matrices
        a = new_rand_matrix(size, size);
        b = new_rand_matrix(size, size);
        c = new_zero_matrix(size, size);

        // start timing
        high_resolution_clock::time_point t_start = high_resolution_clock::now();

        // broadcast (send) b
        MPI_Bcast(b[0], size*size, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // scatter (send) a
        MPI_Scatterv(a[0], send_counts, displacements, MPI_INT, MPI_IN_PLACE, send_counts[rank], MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // matrix multiplication
        cl_matrix_mul(a, b, c, my_row_count, size);

        // gather (receive) c
        MPI_Gatherv(MPI_IN_PLACE, send_counts[rank], MPI_INT, c[0], send_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // stop timing
        high_resolution_clock::time_point t_stop = high_resolution_clock::now();

        // calculate execution time
        duration<double> exec_time = duration_cast<duration<double>>(t_stop - t_start);

        // open file for output
        ofstream fh;
        fh.open("mm-mpi-cl_result.txt");

        // print matrix a
        fh << "Matrix A:" << endl;
        for (int row = 0; row < size; row++) 
        for (int col = 0; col < size; col++)
            fh << a[row][col] << (col < size - 1 ? "," : "\n");

        // print matrix b
        fh << "Matrix B:" << endl;
        for (int row = 0; row < size; row++) 
        for (int col = 0; col < size; col++)
            fh << b[row][col] << (col < size - 1 ? "," : "\n");

        // print matrix c
        fh << "Product:" << endl;
        for (int row = 0; row < size; row++) 
        for (int col = 0; col < size; col++)
            fh << c[row][col] << (col < size - 1 ? "," : "\n");

        // print stats
        fh << "Input Size:\t" << size << "\nElapsed Time:\t" << exec_time.count() << endl;
        cout << "Input Size:\t" << size << "\nElapsed Time:\t" << exec_time.count() << endl;

        fh.close();

        free_matrix(a);
        free_matrix(b);
        free_matrix(c);
    }
    else {
        // init matrices
        a = new_matrix(my_row_count, size);
        b = new_matrix(size, size);
        c = new_zero_matrix(my_row_count, size);

        // broadcast (receive) b
        MPI_Bcast(b[0], size*size, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // scatter (receive) a
        MPI_Scatterv(a[0], send_counts, displacements, MPI_INT, a[0], send_counts[rank], MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // matrix multiplication
        cl_matrix_mul(a, b, c, my_row_count, size);

        // gather (send) c
        MPI_Gatherv(c[0], send_counts[rank], MPI_INT, c[0], send_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // clean up memory
        free_matrix(a);
        free_matrix(b);
        free_matrix(c);
    }

    MPI_Finalize();

    return 0;
}

int** new_matrix(int rows, int cols) {

    int** matrix = new int*[rows];
    matrix[0] = new int[rows * cols];
    for (int r = 1; r < rows; r++)
        matrix[r] = &matrix[0][r*cols];

    return matrix;
}

int** new_rand_matrix(int rows, int cols) {

    int** matrix = new_matrix(rows, cols);
    for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++)
        matrix[r][c] = rand() % MAX_ELEMENT;

    return matrix;
}

int** new_zero_matrix(int rows, int cols) {

    int** matrix = new_matrix(rows, cols);
    for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++)
        matrix[r][c] = 0;

    return matrix;
}

void free_matrix(int** matrix) {
    delete matrix[0];
    delete matrix;
}

void cl_matrix_mul(int** a, int** b, int** c, size_t rows, size_t cols) {

    cl_int              err;
    cl_device_id        device;
    cl_context          context;
    cl_program          program;
    cl_command_queue    queue;
    cl_kernel           kernel;
    cl_mem              buf_a, buf_b, buf_c;
    cl_event            event = NULL;
    const size_t        global[2] = {rows, cols};
    const size_t        local[2] = {1, 1};

    // create device
    device = create_cl_device();

    // create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err < 0) {
        perror("Failed to create context. Exiting...\n");
        exit(1);
    }

    // create program from source file
    program = create_cl_program(context, device, "matrix_mul.cl");

    // create command queue
    queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
    if (err < 0) {
        perror("Failed to create command queue. Exiting...\n");
        exit(1);
    }

    // create kernel
    kernel = clCreateKernel(program, "multiply_matrices", &err);
    if (err < 0) {
        perror("Failed to create kernel. Exiting...\n");
        exit(1);
    }

    // create matrix buffers
    buf_a = clCreateBuffer(context, CL_MEM_READ_ONLY,  rows*cols*sizeof(int), NULL, NULL);
    buf_b = clCreateBuffer(context, CL_MEM_READ_ONLY,  cols*cols*sizeof(int), NULL, NULL);
    buf_c = clCreateBuffer(context, CL_MEM_READ_ONLY,  rows*cols*sizeof(int), NULL, NULL);

    // copy buffers to device
    clEnqueueWriteBuffer(queue, buf_a, CL_TRUE, 0, rows*cols*sizeof(int), a[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, buf_b, CL_TRUE, 0, cols*cols*sizeof(int), b[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, buf_c, CL_TRUE, 0, rows*cols*sizeof(int), c[0], 0, NULL, NULL);

    // copy kernel args to device
    clSetKernelArg(kernel, 0, sizeof(int), (void*)&rows);
    clSetKernelArg(kernel, 1, sizeof(int), (void*)&cols);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&buf_a);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&buf_b);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&buf_c);
    if(err < 0) {
      perror("Failed to copy kernel args. Exiting...\n");
      exit(1);
   }

    // execute matrix multiplication
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, buf_c, CL_TRUE, 0, rows*cols*sizeof(int), c[0], 0, NULL, NULL);

    // free memory
    clReleaseKernel(kernel);
    clReleaseMemObject(buf_a);
    clReleaseMemObject(buf_b);
    clReleaseMemObject(buf_c);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);
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

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
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