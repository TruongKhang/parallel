#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
	/*int arr[3];
	//arr = (int*) calloc (3, sizeof *arr);
	arr[0] = 1; arr[1] = 2; arr[2] = 3;
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int nProc;
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int *newArr;

    if (rank == 0) {
		newArr = (int*) calloc (12, sizeof *newArr);
		MPI_Gather(arr, 3, MPI_INT, newArr, 3, MPI_INT, 0, MPI_COMM_WORLD);
	} else {
	//if (rank==2) {
		int arr1[3];
		arr1[0] = rank+1; arr1[1]=rank+2; arr1[2]=rank+3;
		MPI_Gather(arr1, 3, MPI_INT, newArr, 3, MPI_INT, 0, MPI_COMM_WORLD);
		//printf ("%d %d %d", arr1[0], arr1[1], 1<<3);
		
	//}
	MPI_Finalize();
	}
    // Finalize the MPI environment.
	int i;	
	for (i=0; i<12; i++)
    	printf ("%d\n", newArr[i]);*/
	int *arr, i;
	arr = (int*) calloc (100, sizeof *arr);
	for (i=0; i<4; i++) {
		int *arri;
		arri = (int*) calloc (25, sizeof *arr);
		int j;
		for (j=0; j<25; j++) arri[j] = j+1;
		memcpy(&arr[i*25], arri, 25*sizeof *arr); 
	}
	for (i=0; i<100; i++) printf("%d ", arr[i]);
	return 0;
}
