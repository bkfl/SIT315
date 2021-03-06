#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <chrono>
#include <pthread.h>

#define NUM_ELEMENTS 500000000
#define MAX_ELEMENT 10000000

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
        quickSort(arr, low, pi - 1); 
        quickSort(arr, pi + 1, high); 
    }
} 

int main() {

    // seed random from current time
    srand(time(NULL));

    // generate random array
    int* data = (int*)calloc(NUM_ELEMENTS, sizeof(int));
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        data[i] = rand() % (MAX_ELEMENT - 1) + 1;
    }
    
    high_resolution_clock::time_point timeStart = high_resolution_clock::now();
    quickSort(data, 0, NUM_ELEMENTS - 1);
    high_resolution_clock::time_point timeEnd = high_resolution_clock::now();

    duration<double> testDuration = duration_cast<duration<double>>(timeEnd - timeStart);
    cout << "Sort completed in " << testDuration.count() << " seconds." << endl;

    /*for (int i = 0; i < NUM_ELEMENTS; i++) {
        cout << data[i] << " ";
    }
    cout << endl;*/

    free(data);
    return 0;
}