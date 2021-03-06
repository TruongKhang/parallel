/**
 * Exercise a limited-depth-tree parallel merge sort under MPI.
 *
 * This works best if the number of processes in the communicator
 * is an exact power of two.  Then at the leaf level all processes
 * are computing and then (where the "parent" is not the node
 * itself) sending the results back to the parent.
 *
 * Author:  Timothy Rolfe
 */
#include <stdio.h>
#include <stdlib.h>     // for rand, etc.
#include <time.h>       // for time(NULL)
#include <mpi.h>
#include <string.h>
#include "utils.c"
#include "merge_sort.c"

//#define  DEBUG

#define  INIT  1        // Message giving size and height
#define  DATA  2        // Message giving vector to sort
#define  ANSW  3        // Message returning sorted vector
#define  FINI  4        // Send permission to terminate
#define  ARRAY1 5
#define ARRAY2 6

// Required by qsort()
int compare ( const void* left, const void* right );  // for qsort()

// Parallel merge logic under MPI
void parallelMerge ( long *vector, int size, int myHeight );

// Generate a vector of random data for a size.  Modify the
// variables vector and size by writing through the pointers
// passed (i.e., pointer to long* and pointer to int).
void getData ( long **vPtr, int *sizePtr );

// Verify the array:  for all k, x[k] = k+1
int  validate ( long *vector, int size );

void mergeRanking();

void mergeSequen(long *arr, long *arr1, long *arr2, int start1, int finish1, int start2, int finish2);

int main ( int argc, char* argv[] )
{
   int myRank, nProc;
   int rc;
   int   size;          // Size of the vector being sorted
   long *vector,        // Vector for parallel sort
        *solo;          // Copy for sequential sort
   double start,        // Begin parallel sort
          middle,       // Finish parallel sort
          finish;       // Finish sequential sort

   rc = MPI_Init(&argc, &argv);
   srand(time(NULL));// Set up for shuffling

   if ( rc < 0 )
   {
      puts ("Failed to enroll in MPI.  Abort!");
      exit(-1);
   }

   if ( argc > 1 )
      size = atoi(argv[1]);
   rc = MPI_Comm_rank (MPI_COMM_WORLD, &myRank);
   rc = MPI_Comm_size (MPI_COMM_WORLD, &nProc);

#ifdef DEBUG
   printf ("Started rank %d\n", myRank);  fflush(stdout);
#endif

	
   if ( myRank == 0 )        // Host process
   {
      int rootHt = 0, nodeCount = 1;

      while ( nodeCount < nProc )
      {  nodeCount += nodeCount; rootHt++;  }

      printf ("%d processes mandates root height of %d\n", nProc, rootHt);

      getData (&vector, &size);   // The vector to be sorted.
	//int i =0;
	//for (i=0; i<size; i++) printf("%ld ", vector[i]);
	//printf("\n");

   // Capture time to sequentially sort the idential array
      solo = (long*) calloc ( size, sizeof *solo );
      memcpy (solo, vector, size * sizeof *solo);

      start = MPI_Wtime();
      parallelMerge ( vector, size, rootHt);
      middle = MPI_Wtime();
   }
   else                      // Node process
   {
      int   iVect[2],        // Message sent as an array
            height,          // Pulled from iVect
            parent;          // Computed from myRank and height
      MPI_Status status;     // required by MPI_Recv

      rc = MPI_Recv( iVect, 2, MPI_INT, MPI_ANY_SOURCE, INIT,
           MPI_COMM_WORLD, &status );
      size   = iVect[0];     // Isolate size
      height = iVect[1];     // and height
      vector = (long*) calloc (size, sizeof *vector);

      rc = MPI_Recv( vector, size, MPI_LONG, MPI_ANY_SOURCE, DATA,
           MPI_COMM_WORLD, &status );

      parallelMerge ( vector, size, height );

	  mergeRanking();

#ifdef DEBUG
      printf ("%d resigning from MPI\n", myRank); fflush(stdout);
#endif
      MPI_Finalize();
      return 0;
   }
// Only the rank-0 process executes here.

   mergeSort(solo, 0, size-1);

   finish = MPI_Wtime();

#ifdef DEBUG
      printf ("%d resigning from MPI\n", myRank); fflush(stdout);
#endif
   if ( validate ( vector, size ) )
      puts ("Sorting succeeds.");
   else
      puts ("SORTING FAILS.");

   printf ("  Parallel:  %3.3f\n", (middle-start) );
   printf ("Sequential:  %3.3f\n", (finish-middle) );
   printf ("  Speed-up:  %3.3f\n", (finish-middle)/(middle-start) );
   MPI_Finalize();
}



// Verify the array:  for all k, x[k] = k+1
int  validate ( long *vector, int size )
{
   int k;

   for ( k = 0; k < size; k++ )
      if ( vector[k] != k+1 ){
		//printf("%ld ", vector[k]);
         return 0;
		}
   return 1;
}

// Required by qsort()
int compare ( const void* left, const void* right )
{
   long *lt = (long*) left,
        *rt = (long*) right,
         diff = *lt - *rt;

   if ( diff < 0 ) return -1;
   if ( diff > 0 ) return +1;
   return 0;
}

/**
 * Parallel merge logic under MPI
 *
 * The working core:  each internal node ships its right-hand
 * side to the proper node below it in the processing tree.  It
 * recurses on this function to process the left-hand side, as
 * the node one closer to the leaf level.
 */
void parallelMerge ( long *vector, int size, int myHeight )
{  int parent;
   int myRank, nProc;
   int rc, nxt, rtChild;

   rc = MPI_Comm_rank (MPI_COMM_WORLD, &myRank);
   rc = MPI_Comm_size (MPI_COMM_WORLD, &nProc);

   parent = myRank & ~(1<<myHeight);
   nxt = myHeight - 1;
   if ( nxt >= 0 )
      rtChild = myRank | ( 1 << nxt );

#ifdef DEBUG
   if ( myHeight > 0 )
      printf ("%#x -> %#x -> %#x among %d\n", parent, myRank, rtChild, nProc);
#endif
   if ( myHeight > 0 ) {//Possibly a half-full node in the processing tree
      if ( rtChild >= nProc )     // No right child.  Move down one level
         parallelMerge ( vector, size, nxt );
      else {
         int   left_size  = size / 2,
               right_size = size - left_size;
         long *leftArray  = (long*) calloc (left_size, sizeof *leftArray),
              *rightArray = (long*) calloc (right_size, sizeof *rightArray);
         int   iVect[2];
         int   i, j, k;                // Used in the merge logic
         MPI_Status status;            // Return status from MPI

         memcpy (leftArray, vector, left_size*sizeof *leftArray);
         memcpy (rightArray, vector+left_size, right_size*sizeof *rightArray);
#ifdef DEBUG
         printf ("%d sending data to %d\n", myRank, rtChild); fflush(stdout);
#endif
         iVect[0] = right_size;
         iVect[1] = nxt;
         rc = MPI_Send( iVect, 2, MPI_INT, rtChild, INIT,
              MPI_COMM_WORLD);
         rc = MPI_Send( rightArray, right_size, MPI_LONG, rtChild, DATA,
              MPI_COMM_WORLD);

         parallelMerge ( leftArray, left_size, nxt );
#ifdef DEBUG
         printf ("%d waiting for data from %d\n", myRank, rtChild); fflush(stdout);
#endif
         rc = MPI_Recv( rightArray, right_size, MPI_LONG, rtChild, ANSW,
              MPI_COMM_WORLD, &status );

		int nProcFree = 1<<myHeight;
		int chunk_size = left_size/nProcFree;
		int start = 0, end, size;
		int finishLR = binarySearch(leftArray[left_size-1], rightArray, 0, right_size-1);
		
		if (finishLR == -1) {
			memcpy(vector, leftArray, left_size*sizeof *vector);
			memcpy(&vector[left_size], rightArray, right_size*sizeof *vector);
		} else {
			
		for (i=0; i<nProcFree; i++) {
			if (i==0) {
				end = start+chunk_size-1;
				int finishR = binarySearch(leftArray[end], rightArray, 0, finishLR);
				long *arrMerged;
				if (finishR==-1) {
					memcpy(vector, leftArray, chunk_size*sizeof vector);
					size = chunk_size;
				} else {
					size = chunk_size+finishR+1;
					arrMerged = (long*) calloc (size, sizeof *arrMerged);
					mergeSequen(arrMerged, leftArray, rightArray, start, end, 0, finishR);
					memcpy(vector, arrMerged, size*sizeof *vector);
				}
				start = end+1; 
			} else {
				int info[4];
				info[0]=chunk_size+1; info[1]=finishLR+1; info[2]=myHeight; info[3]=myRank;
				if (i==nProcFree-1) {
					info[0]=left_size-start+1;
					MPI_Send(info, 4, MPI_INT, myRank+i, INIT, MPI_COMM_WORLD);
					MPI_Send(&leftArray[start-1], left_size-start+1, MPI_LONG, myRank+i, ARRAY1, MPI_COMM_WORLD);
					MPI_Send(rightArray, finishLR+1, MPI_LONG, myRank+i, ARRAY2, MPI_COMM_WORLD);
					
				} else {
					MPI_Send(info, 4, MPI_INT, myRank+i, INIT, MPI_COMM_WORLD);
					MPI_Send(&leftArray[start-1], chunk_size+1, MPI_LONG, myRank+i, ARRAY1, MPI_COMM_WORLD);
					MPI_Send(rightArray, finishLR+1, MPI_LONG, myRank+i, ARRAY2, MPI_COMM_WORLD);
					start = start + chunk_size;
				}
			}
		}
		
		for (i=1; i<nProcFree; i++) {
			int sizeMerged;
			MPI_Recv(&sizeMerged, 1, MPI_INT, myRank+i, INIT, MPI_COMM_WORLD, &status);
			MPI_Recv(&vector[size], sizeMerged, MPI_LONG, myRank+i, ANSW, MPI_COMM_WORLD, &status);
			size = size + sizeMerged;
		}
		//printf("rootRank: %d, finishLR: %d, size: %d\n", myRank, finishLR, size);
		if (finishLR < right_size-1) memcpy(&vector[size], &rightArray[finishLR+1], (right_size-finishLR-1)*sizeof *vector); 
		}
		
		free(leftArray); free(rightArray);
         // Merge the two results back into vector
         /*i = j = k = 0;
         while ( i < left_size && j < right_size )
            if ( leftArray[i] > rightArray[j])
               vector[k++] = rightArray[j++];
            else
               vector[k++] = leftArray[i++];
         while ( i < left_size )
            vector[k++] = leftArray[i++];
         while ( j < right_size )
            vector[k++] = rightArray[j++];*/
      }
   } else {
      mergeSort(vector, 0, size-1);
#ifdef DEBUG
      printf ("%d leaf sorting.\n", myRank); fflush(stdout);
#endif
   }

/**
 * Note:  If the computed parent is different from myRank, then
 * this is a right-hand side and needs to be sent as a message
 * back to its parent.  Otherwise, the "communication" is done
 * automatically because the result is generated in place.
 */
   if ( parent != myRank )
      rc = MPI_Send( vector, size, MPI_LONG, parent, ANSW,
           MPI_COMM_WORLD );
}

void mergeRanking() {
    int infor[4];
    int sizeL, totalSizeR, height, rootRank;
    int myRank, rootHt=0, nProc;
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);
	int nodeCount=1;
	while ( nodeCount < nProc ) {  nodeCount += nodeCount; rootHt++;  }
    MPI_Status status;
	MPI_Recv(infor, 4, MPI_INT, MPI_ANY_SOURCE, INIT, MPI_COMM_WORLD, &status);
    sizeL = infor[0]; totalSizeR = infor[1]; height=infor[2]; rootRank=infor[3];
    
    long *arrL, *totalArrR;
    arrL = (long*) calloc (sizeL, sizeof *arrL);
    totalArrR = (long*) calloc (totalSizeR, sizeof *totalArrR);

    MPI_Recv(arrL, sizeL, MPI_LONG, MPI_ANY_SOURCE, ARRAY1, MPI_COMM_WORLD, &status);
    MPI_Recv(totalArrR, totalSizeR, MPI_LONG, MPI_ANY_SOURCE, ARRAY2, MPI_COMM_WORLD, &status);

	int startR = binarySearch(arrL[0], totalArrR, 0, totalSizeR-1) + 1;
	int finishR = binarySearch(arrL[sizeL-1], totalArrR, 0, totalSizeR-1);
	int size;
	if (finishR == -1) {
		size = sizeL-1;
		MPI_Send(&size, 1, MPI_INT, rootRank, INIT, MPI_COMM_WORLD);		
		MPI_Send(&arrL[1], size, MPI_LONG, rootRank, ANSW, MPI_COMM_WORLD);
	} else {
		long *arrMerged;
		size = sizeL+finishR-startR;
		arrMerged = (long*) calloc (size, sizeof *arrMerged);
		mergeSequen(arrMerged, arrL, totalArrR, 1, sizeL-1, startR, finishR);
		/*int i;
		printf("My rank: %d, rootRank: %d, rootHeight: %d, sizeL: %d, startR: %d, finishR: %d\n", myRank, rootRank, height, sizeL, startR, finishR);
		for (i=0; i<sizeL; i++) printf("%ld ", arrL[i]);
		printf("\n");
		for (i=0; i<totalSizeR; i++) printf("%ld ", totalArrR[i]);
		printf("\n");
		for (i=0; i<size; i++) printf("%ld ", arrMerged[i]);
		printf("\n");*/
			
		/*int i=1, j=startR, k=0;
         while ( i < sizeL && j <= finishR )
            if ( arrL[i] > totalArrR[j])
               arrMerged[k++] = totalArrR[j++];
            else
               arrMerged[k++] = arrL[i++];
         while ( i < sizeL )
            arrMerged[k++] = arrL[i++];
         while ( j <= finishR )
            arrMerged[k++] = totalArrR[j++];*/
		
		MPI_Send(&size, 1, MPI_INT, rootRank, INIT, MPI_COMM_WORLD);		
		MPI_Send(arrMerged, size, MPI_LONG, rootRank, ANSW, MPI_COMM_WORLD);
		free(arrMerged);
	}
	free(arrL); free(totalArrR);
	if (height < rootHt) mergeRanking();
}

void mergeSequen(long *arr, long *arr1, long *arr2, int start1, int finish1, int start2, int finish2) {
	int i=start1, j=start2, k=0;
	while ( i <= finish1 && j <= finish2 )
            if ( arr1[i] > arr2[j])
               arr[k++] = arr2[j++];
            else
               arr[k++] = arr1[i++];
         while ( i <= finish1 )
            arr[k++] = arr1[i++];
         while ( j <= finish2 )
            arr[k++] = arr2[j++];
}

int binarySearch(long key, long *arr, int start, int finish) {
	if (key < arr[start]) return -1;
	if (key == arr[start]) return start;
	if (key >= arr[finish]) return finish;
    int s=start, f=finish, index;
    while (s < f) {
		int middle = (s+f)/2;
        if (arr[middle] > key) f = middle-1;
        if (arr[middle] == key) index = middle;
        if (arr[middle] < key) s = middle+1;  
    }
    if (start > finish) {
		printf("Error in binarySearch!");
		exit(0);
	}
	if (s < f) return index;
	else {
		if (s==start) return s;
		else {
			if (s==finish) return s-1;
			else {
				if (arr[s] > key) return s-1;
				else return s;
			}
		}
	}
}
