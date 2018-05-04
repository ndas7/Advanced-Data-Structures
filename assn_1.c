#include<stdio.h>
#include<string.h>
#include<sys/time.h>
#include<sys/stat.h>

int bSearch(int s, int begin, int end, int K[]);

int main(int argc, char *argv[]) {

	if (argc != 4) {
        printf("Invalid number of arguments!\nCommand Line Format: assn_1 search-mode keyfile-name seekfile-name\nExiting.\n");
        return 1; 
	}
    
    char* searchMode = argv[1];
    char* keyfile = argv[2];
    char* seekfile = argv[3];
    
   	FILE *fs, *fk; /* Seek and Key file stream */ 
   	int i, j;
   	struct timeval start, end, exec_tm;
    	
	/* Open seek.db file */
	if ((fs = fopen( seekfile, "rb" )) == NULL) {
		printf("Error in opening key file. Exiting.\n");
		return 1; 
	}

	/* Find the size of seek.db file to get number of integers in the file*/
	int seekCount = 0;
	size_t seekSize;
	fseek(fs, 0L, SEEK_END); 
    seekSize = ftell(fs); 
    seekCount = seekSize/sizeof(int);
    (void) fseek(fs, 0L, SEEK_SET);
    
    /*
	if ((fk = fopen( keyfile, "rb" )) == NULL) {
		printf("Error in opening key file. Exiting.\n");
		return 1; 
	}
	
	int keyCount = 0;
    size_t keySize;
    fseek(fk, 0L, SEEK_END); 
    keySize = ftell(fk); 
    keyCount = keySize/sizeof(int);
    (void) fseek(fk, 0L, SEEK_SET); */

    int S[seekCount];
    /* Create a third array hit of same size as S */
    int hit[seekCount]; 

    /* Read seek.db into an appropriately-sized integer array S */
    for(i = 0; i < seekCount; i++) {
    	fread( &S[i], sizeof( int ), 1, fs ); 
    }
    (void) fseek(fs, 0L, SEEK_SET);
    
    /* Find the size of key.db file to get number of integers in the file */
    int keyCount = 0;
    size_t keySize;
    struct stat st;
	stat(keyfile, &st);
	keySize = st.st_size;
	keyCount = keySize/sizeof(int);

	/* In-Memory Sequential Search*/
	if (strcmp(searchMode, "--mem-lin") == 0) {
		
		int K[keyCount];

		/* Start recording time */
		gettimeofday( &start, NULL);

		/* Open and read key.db into an appropriately-sized integer array K */
		if ((fk = fopen( keyfile, "rb" )) == NULL) {
			printf("Error in opening key file. Exiting.\n");
			return 1; 
		}
		for(i = 0; i < keyCount; i++) {
	    	fread( &K[i], sizeof( int ), 1, fk ); 
	    }

	    /* For each S[i], search K sequentially from beginning to end for a matching key value.
	     If S[i] is found in K, set hit[i]=1. If S[i] is not found in K, set hit[i]=0 */
	    for(i = 0; i < seekCount; i++) {
	    	hit[i] = 0;
	    	for(j = 0; j < keyCount; j++){
	    		if (S[i] == K[j]){
	    			hit[i] = 1;
	    			break;
	    		}
	    	}
	    }
	    /* End recording time */
	    gettimeofday( &end, NULL);

	    /* Calculate time difference and store in exec_tm */
	    long timeDiff = ( end.tv_sec - start.tv_sec) * 1000000 + ( end.tv_usec - start.tv_usec);
	    /*double timeDiff = ( end.tv_sec - start.tv_sec) + ( (end.tv_usec - start.tv_usec)/1000000.0 ); */
	    exec_tm.tv_sec = timeDiff / 1000000;
	    exec_tm.tv_usec = timeDiff % 1000000;

	} 

	/* In-Memory Binary Search*/
	else if (strcmp(searchMode, "--mem-bin") == 0) {
		
		int K[keyCount];

		/* Start recording time */
		gettimeofday( &start, NULL);

		/* Open and read key.db into an appropriately-sized integer array K */
		if ((fk = fopen( keyfile, "rb" )) == NULL) {
			printf("Error in opening key file. Exiting.\n");
			return 1; 
		}
		for(i = 0; i < keyCount; i++) {
	    	fread( &K[i], sizeof( int ), 1, fk ); 
	    }

	    /* For each S[i], use a binary search on K to find a matching key value.
	     If S[i] is found in K, set hit[i]=1. If S[i] is not found, set hit[i]=0*/
	    for(i = 0; i < seekCount; i++) {
	    	hit[i] = bSearch(S[i], 0, keyCount - 1, K);
	    }
	    /* End recording time */
	    gettimeofday( &end, NULL);

	    /* Calculate time difference and store in exec_tm */
	    long timeDiff = ( end.tv_sec - start.tv_sec) * 1000000 + ( end.tv_usec - start.tv_usec);
	    exec_tm.tv_sec = timeDiff / 1000000;
	    exec_tm.tv_usec = timeDiff % 1000000;
	    
	}

	/* On-Disk Sequential Search*/
	else if (strcmp(searchMode, "--disk-lin") == 0) {
		
		int k;

		/* Open key.db for reading */
		if ((fk = fopen( keyfile, "rb" )) == NULL) {
			printf("Error in opening key file. Exiting.\n");
			return 1; 
		}
		
		/* Start recording time */
		gettimeofday( &start, NULL);

		/* For each S[i], search key.db sequentially from beginning to end for a matching key value. 
		If S[i] is found in key.db, set hit[i]=1. If S[i] is not found in key.db, set hit[i]=0. */
		for(i = 0; i < seekCount; i++) {
			hit[i] = 0;
			for(j = 0; j < keyCount; j++){
	    		fread( &k, sizeof( int ), 1, fk);
	    		if (S[i] == k){
	    			hit[i] = 1;
	    			break;
	    		}
	    		fseek( fk, (j + 1) * sizeof( int ), SEEK_SET );
	    	}
	    	(void) fseek(fk, 0, SEEK_SET);
	    	clearerr(fk);
		}
		/* End recording time */
		gettimeofday( &end, NULL);

		/* Calculate time difference and store in exec_tm */
	    long timeDiff = ( end.tv_sec - start.tv_sec) * 1000000 + ( end.tv_usec - start.tv_usec);
	    exec_tm.tv_sec = timeDiff / 1000000;
	    exec_tm.tv_usec = timeDiff % 1000000;

	}

	/* On-Disk Binary Search*/
	else if (strcmp(searchMode, "--disk-bin") == 0) {
		
		int k, mid, startIndex, endIndex;

		/* Open key.db for reading */
		if ((fk = fopen( keyfile, "rb" )) == NULL) {
			printf("Error in opening key file. Exiting.\n");
			return 1; 
		}
		
		/* Start recording time */
		gettimeofday( &start, NULL);

		/* For each S[i], use a binary search on key.db to find a matching key value.
		 If S[i] is found in key.db, set hit[i]=1. If S[i] is not found in key.db, set hit[i]=0.*/
		for(i = 0; i < seekCount; i++) {
			startIndex = 0;
			endIndex = keyCount - 1;
			hit[i] = 0;
			for(; startIndex <= endIndex; ){
				mid = (startIndex + endIndex) / 2;
				fseek( fk, mid * sizeof( int ), SEEK_SET );
	    		fread( &k, sizeof( int ), 1, fk);
	    		if (k == S[i]){
	    			hit[i] = 1;
	    			break;
	    		}
	    		else if (k < S[i])
	    			startIndex = mid + 1;
	    		else if (k > S[i])
	    			endIndex = mid - 1;
	    	}
	    	(void) fseek(fk, 0, SEEK_SET);
	    	clearerr(fk);
		}
		/* End recording time */
		gettimeofday( &end, NULL);

		/* Calculate time difference and store in exec_tm */
	    long timeDiff = ( end.tv_sec - start.tv_sec) * 1000000 + ( end.tv_usec - start.tv_usec);
	    exec_tm.tv_sec = timeDiff / 1000000;
	    exec_tm.tv_usec = timeDiff % 1000000;

	}

	else {
		printf("Invalid search-mode. Exiting.\n");
		return 1;
	}

	/* Writing results for each key in S[i], and the total time needed to perform all searching */
	for(i = 0; i < seekCount; i++) {
		if ( hit[i] == 1 ) {
			printf( "%12d: Yes\n", S[i] );
		}
		else if ( hit[i] == 0 ) {
			printf( "%12d: No\n", S[i] );
		}
	}
	printf( "Time: %ld.%06ld\n", exec_tm.tv_sec, exec_tm.tv_usec );

	/* Closing both file descriptors and exiting */
	fclose(fs);
    fclose(fk);

	return 0;
}

/* Function for performing Binary Search recursively */
int bSearch(int s, int begin, int end, int K[]) {

	int mid; 
	if (begin < end) {
		mid = (begin + end) / 2;
		if (s == K[mid])
			return 1;
		else if (s > K[mid])
			return bSearch(s, mid + 1, end, K);
		else if (s < K[mid])
			return bSearch(s, begin, mid, K);
	}
	else if (begin == end) {
		if (s == K[begin])
			return 1;
		else
			return 0;		
	}
	return 0;
}
