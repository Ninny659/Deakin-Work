// This is a kernal that is ran by every compute unit in the GPU or CPU
__kernel void matrix_multiply(const int size,
                      __global int* v1, __global int* v2, __global int* v3) {
    
    // Thread identifiers
    const int i = get_global_id(0);   
    const int j = get_global_id(1);  

    const int globalIndex = (i * size) + j;
 
    //uncomment to see the index each PE works on
    // printf("Kernel process index :(%d)\n ", globalIndex);

    v3[globalIndex] = v1[globalIndex] * v2[globalIndex];
}
