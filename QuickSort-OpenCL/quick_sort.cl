/* Max number of elements that can be sorted, this has to be fixed.
This is due to OpenCL not allowing variable length arrays.
*/
#define MAX_STACK_SIZE 10000

__kernel void QuickSort(const int arrSize,
                         __global int* unsortedArr, __global int* sortedArr)
{
    // Debug print
    // printf("\nArray size: (%u)", arrSize);
    // printf("\nSorted array in OpenCL Kernel:");
    // for(int i = 0; i < arrSize; i++)
    // {
    //     printf("%d ", sortedArr[i]);
    // }

    int stack[MAX_STACK_SIZE];
    int stackTop = -1;

    // Push the initial boundaries onto the stack
    stack[++stackTop] = 0;
    stack[++stackTop] = arrSize - 1;

    // Keep popping from the stack while it is not empty
    while (stackTop >= 0)
    {
        // Pop the boundaries from the stack
        int right = stack[stackTop--];
        int left = stack[stackTop--];

        // Partition the array
        if (left < right)
        {
            // Partition the array and get the pivot index
            int pivot = unsortedArr[right];
            int i = (left - 1);

            for (int j = left; j <= right - 1; j++)
            {
                // If current element is smaller than the pivot
                if (unsortedArr[j] < pivot)
                {
                    i++;

                    // Swap the elements
                    int t = unsortedArr[i];
                    unsortedArr[i] = unsortedArr[j];
                    unsortedArr[j] = t;
                }
            }

            // Swap the pivot element with the element at i + 1
            int t = unsortedArr[i + 1];
            unsortedArr[i + 1] = unsortedArr[right];
            unsortedArr[right] = t;
            
            int pivotIndex = i + 1;

            // Push the left subarray boundaries onto the stack
            stack[++stackTop] = left;
            stack[++stackTop] = pivotIndex - 1;

            // Push the right subarray boundaries onto the stack
            stack[++stackTop] = pivotIndex + 1;
            stack[++stackTop] = right;
        }
    }

}   

