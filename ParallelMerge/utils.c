#include <stdio.h>
#include <stdlib.h>

// If *sizePtr == 0, dialog with the user; otherwise use the
// specified size.  Generate a vector of that size, fill it
// so that x[k] = k+1, and then shuffle the vector.
int nextInt(int ceiling)
{  return (int) ((double)rand() * ceiling / RAND_MAX);  }

/**
 * Shuffle the entire array
 *
 * See Rolfe, Timothy.  "Algorithm Alley:  Randomized Shuffling",
 * Dr. Dobb's Journal, Vol. 25, No. 1 (January 2000), pp. 113-14.
 */
void shuffleArray ( long*  x, int lim )
{  while ( lim > 1 )
   {  int item;
      int save = x[lim-1];
      item = nextInt(lim);
      x[--lim] = x[item];                // Note predecrement on lim
      x[item] = save;
   } // end while
} // end shuffleArray()

// If *sizePtr == 0, dialog with the user; otherwise use the
// specified size.  Generate a vector of that size, fill it
// so that x[k] = k+1, and then shuffle the vector.
void getData ( long **vPtr, int *sizePtr )
{
   int   size;
   int   k;
   long *data;

   fputs ("Size:  ", stdout);
   if ( *sizePtr == 0 )
      scanf ("%d", &size);
   else
   {
      size = *sizePtr;
      printf ("%d\n", size);
   }
   data = (long*) calloc ( size, sizeof *data );
   for ( k = 0; k < size; k++ )
      data[k] = k+1;
   shuffleArray ( data, size );
	/*FILE *f;
	f = fopen("data.txt", "rt");
	int i=0;
	while (!feof(f)) {long n; fscanf(f,"%ld",&n); data[i]=n; i++;}
	fclose(f);*/
	//data[0]=5; data[1]=14; data[2]=4; data[3]=18; data[4]=20; data[5]=12; data[6]=7; data[7]=3; data[8]=13; data[9]=6;
	//data[10]=16; data[11]=11; data[12]=8; data[13]=19; data[14]=10; data[15]=17; data[16]=9; data[17]=15; data[18]=2; data[19]=1;
   // Write the results back through the pointer parameters
   *vPtr = data;
   *sizePtr = size;
}
