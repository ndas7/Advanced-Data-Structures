#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<sys/stat.h>

typedef struct { 
	int key; /* Record's key */ 
	long off; /* Record's offset in file */ 
} index_S;

typedef struct {
	int size; /* Hole's size */ 
	long off; /* Hole's offset in file */ 
} avail_S;

/* Function for performing Binary Search recursively */
int bSearch(int s, int begin, int end, index_S prim[]) {

	int mid; 
	if (begin < end) {
		mid = (begin + end) / 2;
		if (s == prim[mid].key)
			return mid;
		else if (s > prim[mid].key)
			return bSearch(s, mid + 1, end, prim);
		else if (s < prim[mid].key)
			return bSearch(s, begin, mid, prim);
	}
	else if (begin == end) {
		if (s == prim[begin].key)
			return begin;
		else
			return -1;		
	}
	return -1;
}


int comparator_index(const void *s1, const void *s2) 
{
	const index_S *a = (index_S *)s1;
    const index_S *b = (index_S *)s2;
    int key1 = a->key;
    int key2 = b->key; 
    return (key1 - key2);
}

int comparator_avail_best(const void *s1, const void *s2) 
{
	const avail_S *a1 = (avail_S *)s1;
    const avail_S *a2 = (avail_S *)s2;
    if ( a1->size - a2->size < 0 )
        return -1;
    else if ( a1->size - a2->size > 0 )
        return 1;
    else if ( a1->off - a2->off < 0)
        return -1;
    else if ( a1->off - a2->off > 0)
        return 1;
   else
        return 0;
}

int comparator_avail_worst(const void *s1, const void *s2) 
{
	const avail_S *a1 = (avail_S *)s1;
    const avail_S *a2 = (avail_S *)s2;
    if ( a1->size - a2->size < 0 )
        return 1;
    else if ( a1->size - a2->size > 0 )
        return -1;
    else if ( a1->off - a2->off < 0)
        return -1;
    else if ( a1->off - a2->off > 0)
        return 1;
   else
        return 0;
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
       	printf("Invalid number of arguments!\nCommand Line Format: assn_2 avail-list-order studentfile-name \nExiting.\n");
        return 1; 
    }
    
    char* availListOrder = argv[1];
    char* studentfile = argv[2];
    
    FILE *fs; 
    FILE *index_in, *index_out;
    FILE *avail_in, *avail_out;

    int i, j;
    char *buf; 
    long rec_off; 
    int rec_siz; 
    int hole_siz = 0;

    char *line = NULL;  
    size_t len = 0;     
    ssize_t read;

    index_S *prim;
    avail_S *hole;

    int keyCount = 0;
    size_t keySize;
    struct stat st;

    int holeCount = 0;
    size_t holeSize;
    struct stat sta;
   		
    /* Open student db file */
    if ((fs = fopen( studentfile, "r+b" )) == NULL) {
        fs = fopen( studentfile, "w+b" ); 
        index_in = fopen("index.bin", "w+b" ); 
        avail_in = fopen("avail.bin", "w+b" ); 
    }
    else {
    	index_in = fopen( "index.bin", "rb" );
    	stat("index.bin", &st);
		keySize = st.st_size;
		keyCount = keySize/sizeof(index_S); 

		avail_in = fopen( "avail.bin", "rb" );
		stat("avail.bin", &sta);
		holeSize = sta.st_size;
		holeCount = holeSize/sizeof(avail_S); 
    }
    
    /* Load index file */
    prim = (index_S*) malloc(sizeof(index_S)*keyCount);
    fread(prim, sizeof( index_S ), keyCount, index_in); 
    fclose(index_in); 

   /* Load availability list file */
    hole = malloc(sizeof(avail_S)*holeCount);
    fread(hole, sizeof( avail_S ), holeCount, avail_in); 
    fclose(avail_in); 

    read = getline(&line, &len, stdin);

    while (strcmp(line, "end") != 0) {

        /*if (read > 0) {
            printf ("Read %zd chars from stdin, allocated %zd bytes for line : %s\n", read, len, line);
        } */

        char* token;
        char* buf;
        int key;
        int result;
        rec_off = 0;
        int seekCount = 0;
		size_t seekSize;
		int total_size = 0;
		int flag = 0;
		int index_found = 0;
		
        token = strtok (line," \r\n");

        if (strcmp(token, "add") == 0) {

        	token = strtok(NULL, " \r\n");
	        key = (int)strtoul(token, NULL, 10);
	    	token = strtok(NULL, " \r\n");
	    	rec_siz = strlen(token);
	    	char rec[rec_siz];
	    	strcpy(rec, token); 
	    	
	    	if( prim!= NULL || keyCount > 0){
	    		result = bSearch(key, 0, keyCount - 1, prim);
	    		if (result > 0){
	    			printf("Record with SID=%d exists\n", key);
	    			read = getline(&line, &len, stdin);
	    			continue;
	    		} 
	    	}

	    	/* Search avail list for hole, else append to end of student file */
	    	if (holeCount > 0) {

	    		total_size = rec_siz + sizeof(rec_siz);
	    		for ( i = 0; i < holeCount; i++) {
	    			if (hole[i].size >= total_size) {
	    				flag = 1;
	    				index_found = i;
	    				break;
	    			}
	    		}

	    		if (flag == 0) {	/* No appropriately-sized hole; append to end of student file */
	    			fseek(fs, 0, SEEK_END); 
		    		rec_off = ftell(fs); 
				    
		    		fwrite(&rec_siz,sizeof(rec_siz), 1, fs);
		    		fwrite(rec, 1, rec_siz, fs);
		    		(void) fseek(fs, 0L, SEEK_SET);
	    		}
	    		else if (flag == 1) {

	    			/* Write rec size and body to hole's offset */
	    			rec_off = hole[index_found].off;
	    			fseek(fs, rec_off, SEEK_SET);

	    			fwrite(&rec_siz,sizeof(rec_siz), 1, fs);
		    		fwrite(rec, 1, rec_siz, fs);
		    		(void) fseek(fs, 0L, SEEK_SET);

		    		/* Add leftover fragment back to the avail list as a new, smaller hole */
		    		if (hole[index_found].size > total_size){
		    			holeCount++;
				    	hole = (avail_S *)realloc(hole, sizeof(avail_S)*(holeCount) );
				    	hole[holeCount-1].size = hole[index_found].size - total_size;
			    		hole[holeCount-1].off = rec_off + total_size;
		    		}

		    		/* Remove hole from avail list based on order */
		    		if (strcmp(availListOrder, "--first-fit") == 0) {
						for (j = index_found; j < holeCount - 1; j++) {
							hole[j].size = hole[j + 1].size;
							hole[j].off = hole[j + 1].off;
						}
						holeCount--;
						hole = (avail_S *)realloc(hole, sizeof(avail_S)*(holeCount) );					

					} 
					else if (strcmp(availListOrder, "--best-fit") == 0) {
						hole[index_found].size = hole[holeCount - 1].size;
						hole[index_found].off = hole[holeCount - 1].off;
						holeCount--;
						hole = (avail_S *)realloc(hole, sizeof(avail_S)*(holeCount) );
						qsort(hole, holeCount, sizeof(avail_S), comparator_avail_best);	
					}
					else if (strcmp(availListOrder, "--worst-fit") == 0) {
						hole[index_found].size = hole[holeCount - 1].size;
						hole[index_found].off = hole[holeCount - 1].off;
						holeCount--;
						hole = (avail_S *)realloc(hole, sizeof(avail_S)*(holeCount) );
						qsort(hole, holeCount, sizeof(avail_S), comparator_avail_worst);		
					}
					else {
						printf("Invalid availability list order. Exiting.\n");
						return 1;
					}

	    		}

	    	}
	    	/* No existing holes; append to end of student file */
	    	else {
	    		fseek(fs, 0, SEEK_END); 
	    		rec_off = ftell(fs); 
			    
	    		fwrite(&rec_siz,sizeof(rec_siz), 1, fs);
	    		fwrite(rec, 1, rec_siz, fs);
	    		(void) fseek(fs, 0L, SEEK_SET);
	    	}

	    	/* Write to primary key index array and sort it */
	    	keyCount++;
	    	prim = (index_S *)realloc(prim, sizeof(index_S)*(keyCount) );
	    	prim[keyCount-1].key = key;
    		prim[keyCount-1].off = rec_off;
    		qsort(prim, keyCount, sizeof(index_S), comparator_index);
	    	
		} /* End add command*/ 

		else if (strcmp(token, "find") == 0) {
			
			token = strtok(NULL, " \r\n");
		    key = (int)strtoul(token, NULL, 10);
		    
		    if( prim!= NULL || keyCount > 0){
	    		result = bSearch(key, 0, keyCount - 1, prim);
	    		if (result < 0){
	    			printf("No record with SID=%d exists\n", key);
	    		} 
	    		else {
	    			rec_siz = 0;
	    			rec_off = prim[result].off;
	    			fseek(fs, rec_off, SEEK_SET);
				    fread( &rec_siz, sizeof( int ), 1, fs ); 
				    buf = malloc (rec_siz+1);
				    fread(buf, 1, rec_siz, fs);
				    buf[rec_siz] = '\0';
				    printf("%s\n", buf);
				    (void) fseek(fs, 0L, SEEK_SET);
	    		}
	    	}
	    	else {
	    		printf("No record with SID=%d exists\n", key);
	    	}

	    	  
		} /* End find command*/

		else if (strcmp(token, "del") == 0) {
			
			token = strtok(NULL, " \r\n");
	    	key = (int)strtoul(token, NULL, 10);
			
			if( keyCount > 0 ){
	    		result = bSearch(key, 0, keyCount - 1, prim);
	    		if (result < 0){
	    			printf("No record with SID=%d exists\n", key);
	    		} 
	    		else {
	    			rec_siz = 0;
	    			rec_off = prim[result].off;
	    			fseek(fs, rec_off, SEEK_SET);
				    fread( &rec_siz, sizeof( int ), 1, fs ); 
				    (void) fseek(fs, 0L, SEEK_SET);

				    /* Write to availability hole array and order it accordingly */
			    	holeCount++;
			    	hole = (avail_S *)realloc(hole, sizeof(avail_S)*(holeCount) );
			    	hole[holeCount-1].size = rec_siz + sizeof( rec_siz );
		    		hole[holeCount-1].off = rec_off;

		    		if (strcmp(availListOrder, "--best-fit") == 0) {
						qsort(hole, holeCount, sizeof(avail_S), comparator_avail_best);
					}

					else if (strcmp(availListOrder, "--worst-fit") == 0) {
						qsort(hole, holeCount, sizeof(avail_S), comparator_avail_worst);
					}

					/* Remove deleted entry from index file */

					prim[result].key = prim[keyCount - 1].key;
					prim[result].off = prim[keyCount - 1].off;
					keyCount--;
					prim = (index_S *)realloc(prim, sizeof(index_S)*(keyCount) );
					qsort(prim, keyCount, sizeof(index_S), comparator_index);

	    		}
	    	}
	    	else {
	    		printf("No record with SID=%d exists\n", key);
	    	}
	    	
		} /* End del command*/
        
        read = getline(&line, &len, stdin);
        
    } /* End while loop */
    
    /* Writing results at the end  */
	printf( "Index:\n");
	for ( i = 0; i < keyCount; i++){
		printf( "key=%d: offset=%ld\n", prim[i].key, prim[i].off );
	}
	printf( "Availability:\n");
	for ( i = 0; i < holeCount; i++){
		printf( "size=%d: offset=%ld\n", hole[i].size, hole[i].off );
		hole_siz += hole[i].size;
	}
	printf( "Number of holes: %d\n", holeCount );
	printf( "Hole space: %d\n", hole_siz );

    /* int n;
    
    fseek(fs, 0, SEEK_SET);
    fread( &n, sizeof( int ), 1, fs ); 
    printf("\nFirst = %d\n", n);
    (void) fseek(fs, 0L, SEEK_SET);  */


    index_out = fopen( "index.bin", "wb" ); 
    fwrite( prim, sizeof( index_S ), keyCount, index_out ); 
    fclose( index_out );

    avail_out = fopen( "avail.bin", "wb" ); 
    fwrite( hole, sizeof( avail_S ), holeCount, avail_out ); 
    fclose( avail_out );
	
	/* Closing file descriptors and exiting */
    fclose(fs);
    free(line);
    free(prim);
    free(hole);
    
    return 0;
}

