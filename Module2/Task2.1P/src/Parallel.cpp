#include <iostream>
#include <fstream>
#include <chrono>
#include <pthread.h>
#include <thread>

#define MAX_EL 20
#define MAX_THREADS 2

using namespace std;
using namespace chrono;

struct worker_args {
    int** matrix_a;
    int** matrix_b;
    int** result;
    int el_start, el_end, size;
};

int** create_matrix(int size) {

    int** matrix = 0;
    matrix = new int*[size];

    for (int r = 0; r < size; r++) {
        matrix[r] = new int[size];
        for (int c = 0; c < size; c++) {
            matrix[r][c] = rand() % MAX_EL;
        }
    }

    return matrix;
}

void clear_matrix(int** matrix, int size) {

    for (int r = 0; r < size; r++) {
        delete [] matrix[r];
    }
    delete [] matrix;
}

int calc_row_col(int** matrix_a, int** matrix_b, int row, int col, int size) {

    int result = 0;

    for (int i = 0; i < size; i++) {
        result += matrix_a[row][i] * matrix_b[i][col];
    }

    return result;
}

void* worker(void* arg) {
    
    struct worker_args* args = (struct worker_args*)arg;

    for (int el = args->el_start; el <= args->el_end; el++) {
        int row = el / args->size;
        int col = el % args->size;
        args->result[row][col] = calc_row_col(args->matrix_a, args->matrix_b, row, col, args->size);
    }

    free(arg);
}

int** multiply_matrices(int** matrix_a, int** matrix_b, int size) {

    // create matrix to store result
    int** result = new int*[size];
    for (int r = 0; r < size; r++) {
        result[r] = new int[size];
    }

    // calculate data partitioning based on threads
    int elCount = size * size;
    int threadCount = MAX_THREADS;
    if (MAX_THREADS > elCount) threadCount = elCount;
    int elPart = elCount / threadCount;

    // create array to store thread refs
    pthread_t threads[threadCount];

    // partition and create worker threads
    for (int t = 0; t < (threadCount); t++) {
        
        struct worker_args* args = (struct worker_args*)malloc(sizeof(struct worker_args));
        args->matrix_a = matrix_a;
        args->matrix_b = matrix_b;
        args->result = result;
        args->el_start = t * elPart;
        args->el_end = args->el_start + elPart - 1;
        if (args->el_end > elCount - 1) args->el_end = elCount -1;
        args->size = size;

        pthread_create(&threads[t], NULL, &worker, args);
    }

    // wait for worker threads to finish
    for (int t = 0; t < threadCount; t++) {
        pthread_join(threads[t], NULL);
    }

    return result;
}

void print_matrix(int** &matrix, char* title, int size, ofstream &file) {

    if (file) {
        file << title << ":\n";
        for (int r = 0; r < size; r++) {
            for (int c = 1; c < size - 1; c++) {
                file << matrix[r][c] << ",";
            }
            file << matrix[r][size - 2] << "\n";
        }
    }
}

duration<double> run_test(int size, ofstream &file) {
    
    int** matrix_a = create_matrix(size);
    int** matrix_b = create_matrix(size);

    high_resolution_clock::time_point timeStart = high_resolution_clock::now();
    int** multiplied = multiply_matrices(matrix_a, matrix_b, size);
    high_resolution_clock::time_point timeEnd = high_resolution_clock::now();

    print_matrix(matrix_a, (char*)"Matrix A", size, file);
    print_matrix(matrix_b, (char*)"Matrix B", size, file);
    print_matrix(multiplied, (char*)"Result", size, file);

    clear_matrix(matrix_a, size);
    clear_matrix(matrix_b, size);

    return duration_cast<duration<double>>(timeEnd - timeStart);;
}

int main() {

    srand(time(NULL));

    ofstream file;
    file.open("Parallel.txt");

    int tests[] = {100, 500, 1000, 2500, 5000};
    duration<double> testDuration;

    int cores = thread::hardware_concurrency();
    cout << "Cores: " << cores << endl;
    cout << "Max threads: " << MAX_THREADS << endl << endl;

    int numTests = sizeof(tests) / sizeof(tests[0]);
    for (int t = 0; t < numTests; t++) {
        file << "Test " << t << "\n";
        testDuration = run_test(tests[t], file);
        file << "Input Size:\t" << tests[t] << "\nElapsed Time:\t" << testDuration.count() << "\n\n";
        cout << "Input Size:\t" << tests[t] << "\nElapsed Time:\t" << testDuration.count() << "\n\n";
    }

    file.close();

    return 0;
}