#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>
#include <omp.h>
#include <thread>

using namespace std;
using namespace chrono;

#define MAX_ELEMENT 20

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
        int threads = thread::hardware_concurrency();
        #pragma omp parallel for collapse(2) shared(a, b, c) num_threads(threads) schedule(static, my_row_count*size/threads)
        for (int row = 0; row < my_row_count; row++)
        for (int col = 0; col < size; col++)
        for (int i = 0; i < size; i++)
            c[row][col] += a[row][i] * b[i][col];

        // gather (receive) c
        MPI_Gatherv(MPI_IN_PLACE, send_counts[rank], MPI_INT, c[0], send_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // stop timing
        high_resolution_clock::time_point t_stop = high_resolution_clock::now();

        // calculate execution time
        duration<double> exec_time = duration_cast<duration<double>>(t_stop - t_start);

        // open file for output
        ofstream fh;
        fh.open("mm-mpi-omp_result.txt");

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
        int threads = thread::hardware_concurrency();
        #pragma omp parallel for collapse(2) shared(a, b, c) num_threads(threads) schedule(static, my_row_count*size/threads)
        for (int row = 0; row < my_row_count; row++)
        for (int col = 0; col < size; col++)
        for (int i = 0; i < size; i++)
            c[row][col] += a[row][i] * b[i][col];

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