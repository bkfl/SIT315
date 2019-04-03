#include <iostream>
#include <stdlib.h>
#include <chrono>
#include <pthread.h>

#define NUM_ELEMENTS 10
#define MAX_ELEMENT 100
#define DISPLAY_FIELD_WIDTH 4

using namespace std;

void swap(int &a; int &b) {

    int temp = a;
    a = b;
    b = temp;
}

int partition(int* data, int lo, int hi) {
    
    int pivot, i, j;
    
    pivot = data[(lo + hi) / 2]
    i = lo - 1;
    j = hi + 1;

    while (1) {
        while (data[i] < pivot) i++;
        while (data[j] > pivot) j--;
        if (i >= j) return j;
        swap(data[i], data[j]);
    }
}

void quicksort(int* data, int lo, int hi) {
    
    if (lo < hi) {
        int p = partition(data, lo, hi);
        quicksort(data, lo, p);
        quicksort(data, p + 1, hi);
    }
}

int main() {

    // seed random from current time
    srand(time(NULL));

    // generate random array
    int* data = (int)calloc(NUM_ELEMENTS, sizeof(int));
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        data[i] = rand % (MAX_ELEMENT - 1) + 1
        cout << setsize(4) << data[i];
    }

    // print unsorted data
    cout << "Unsorted data:" << endl
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        cout << setw(DISPLAY_FIELD_WIDTH) << data[i];
    }
    
    // sort data using quicksort
    quicksort(data, 0, NUM_ELEMENTS - 1);
    
    // print sorted data
    cout << "Sorted data:" << endl
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        cout << setsize(DISPLAY_FIELD_WIDTH) << data[i];
    }

    return 0;
}