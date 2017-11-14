#include <stdio.h>
//#include "utils.c"

void merge(long *vector, int left, int middle, int right);

void mergeSort(long *vector, int left, int right) {
	if (left < right) {
		int middle = (left+right)/2;
		mergeSort(vector, left, middle);
		mergeSort(vector, middle+1, right);
		merge(vector, left, middle, right);
	}
}

void merge(long *vector, int left, int middle, int right) {
	int i, j, k;
	long *temp;
	temp = (long*) calloc(right-left+1, sizeof(*temp));
	i = left;
	j = middle+1;
    k = 0;
	while (i <= middle && j <= right) {
		if (*(vector+i) < *(vector+j)) {
			*(temp+k) = *(vector+i);
			i++;
		} else {
			*(temp+k) = *(vector+j);
			j++;
		}
		k++;
	}
	if (i > middle) {
		while (j <= right) {
			*(temp+k) = *(vector+j);
			k++;
			j++;
		}
	}
	if (j > right) {
		while (i <= middle) {
			*(temp+k) = *(vector+i);
			k++;
			i++;
		}
	}
	k = 0;
	for(i=left; i<=right; i++) {
		*(vector+i) = *(temp+k);
		k++;
	}
	free(temp);
}

/*int main() {
	long *vector;
	int size=10, i;
	getData(&vector, &size);
	for (i=0; i<size; i++)
		printf("%li, ", *(vector+i));
	printf("\n");
	mergeSort(vector, 0, size-1);
	for (i=0; i<size; i++)
		printf("%li, ", *(vector+i));
	printf("\n");
}*/
