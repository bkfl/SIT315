#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <omp.h>

#define MAX_EL 20

using namespace std;
using namespace chrono;

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

int** multiply_matrices(int** matrix_a, int** matrix_b, int size, int threads) {

    int** result = 0;
    result = new int*[size];

    omp_set_num_threads(threads);

    #pragma omp parallel for
    for (int r = 0; r < size; r++) {
        result[r] = new int[size];
    }

    #pragma omp parallel for collapse(2) shared(matrix_a, matrix_b, size)
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            result[r][c] = calc_row_col(matrix_a, matrix_b, r, c, size);
        }
    }

    return result;
}

void print_matrix(int** matrix, char* title, int size, ofstream &file) {

    if (file) {
        file << title << ":\n";
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size - 1; c++) {
                file << matrix[r][c] << ",";
            }
            file << matrix[r][size - 1] << "\n";
        }
    }
}

duration<double> run_test(int size, ofstream &file, int threads) {
    
    int** matrix_a = create_matrix(size);
    int** matrix_b = create_matrix(size);

    high_resolution_clock::time_point timeStart = high_resolution_clock::now();
    int** multiplied = multiply_matrices(matrix_a, matrix_b, size, threads);
    high_resolution_clock::time_point timeEnd = high_resolution_clock::now();

    print_matrix(matrix_a, (char*)"Matrix A", size, file);
    print_matrix(matrix_b, (char*)"Matrix B", size, file);
    print_matrix(multiplied, (char*)"Result", size, file);

    clear_matrix(matrix_a, size);
    clear_matrix(matrix_b, size);

    return duration_cast<duration<double>>(timeEnd - timeStart);;
}

int main(int argc, char** argv) {

    srand(time(NULL));

    int threads = 0;
    // set number of threads
    if (argc > 1) threads = atoi(argv[0]);
    // if no arg or invalid arg, set default threads = cores
    if (threads < 1) threads = thread::hardware_concurrency();

    ofstream file;
    file.open("OpenMP.txt");

    int tests[] = {100, 500, 1000, 2500, 5000};
    duration<double> testDuration;

    int numTests = sizeof(tests) / sizeof(tests[0]);
    for (int t = 0; t < numTests; t++) {
        file << "Test " << t << "\n";
        testDuration = run_test(tests[t], file, threads);
        file << "Input Size:\t" << tests[t] << "\nElapsed Time:\t" << testDuration.count() << "\n\n";
        cout << "Input Size:\t" << tests[t] << "\nElapsed Time:\t" << testDuration.count() << "\n\n";
    }

    file.close();

    return 0;
}