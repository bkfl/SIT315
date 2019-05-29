#include <iostream>
#include <stdlib.h>
#include <chrono>
#include <mpi.h>

using namespace std;
using namespace chrono;

void swap(int &a, int &b) 
{ 
    int t = a; 
    a = b; 
    b = t; 
}

int partition(int* arr, int low, int high) 
{ 
    int pivot = arr[high];
    int i = (low - 1);
  
    for (int j = low; j <= high- 1; j++) 
    { 
        if (arr[j] <= pivot) 
        { 
            i++;
            swap(arr[i], arr[j]); 
        } 
    }

    swap(arr[i + 1], arr[high]);
    return (i + 1); 
} 

void quickSort(int* arr, int low, int high) 
{
    if (low < high) 
    {
        int pi = partition(arr, low, high);

        if ((high - low) < 5000) {
            quickSort(arr, low, pi - 1); 
            quickSort(arr, pi + 1, high); 
        }
        else {
            #pragma omp task
            {
                quickSort(arr, low, pi - 1); 
            }
            #pragma omp task
            {
                quickSort(arr, pi + 1, high);
            }
        }
    }
} 

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
        #pragma omp parallel
        {           
            #pragma omp single nowait
            {
                quickSort(data, 0, my_el_count - 1);
            }
        }

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
        #pragma omp parallel
        {           
            #pragma omp single nowait
            {
                quickSort(data, 0, my_el_count - 1);
            }
        }

        // gather (send) data
        MPI_Gatherv(data, send_counts[rank], MPI_INT, data, send_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // clean up memory
        free(data);
    }

    MPI_Finalize();
    return 0;
}