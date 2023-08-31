__kernel void matrixMultiplicationKernel(
    __global const int *m1,
    __global const int *m2,
    __global int *m3,
    int rows,
    int cols) {
    
    int globalRow = get_global_id(0);
    int globalCol = get_global_id(1);
    
    int sum = 0;
    for (int k = 0; k < cols; ++k) {
        sum += m1[globalRow * cols + k] * m2[k * cols + globalCol];
    }
    
    m3[globalRow * cols + globalCol] = sum;
}