__kernel void multiply_matrices(const int my_row_count, const int col_count,
                                const __global int* a, const __global int* b, __global int* c) {
    
    __global const int row = get_global_id(0);
    __global const int col = get_global_id(1);
    __global const int el = row * col_count + col;
 
    int total = 0;
    for (int i = 0; i < col_count; i++) {
        total += a[row * col_count + i] * b[i * col_count + col];
    }
    c[el] = total;
}