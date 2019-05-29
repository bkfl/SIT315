__kernel
void quicksort(queue_t queue, __global int* arr, int low, int high)
{   
    if (low < high)
    {
        int pivot = arr[high];
        int i = (low - 1);
        for (int j = low; j <= high - 1; j++)
        {
            if (arr[j] <= pivot)
            {
                i++;
                int tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }

        int tmp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = tmp;

        int pi = i + 1;

        const ndrange_t ndrange = ndrange_1D(1);
        kernel_enqueue_flags_t flags = CLK_ENQUEUE_FLAGS_NO_WAIT;
        enqueue_kernel(queue, flags, ndrange, ^{ quicksort(queue, arr, low, pi - 1); });
        enqueue_kernel(queue, flags, ndrange, ^{ quicksort(queue, arr, pi + 1, high); });
    }
}