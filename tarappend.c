/* Parallel TAR archive writer utility based on transient daemon process
 * Author: Andrei (Andreas) Scherbakov  */



#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "tarheader.h"

#define TAR_BLK_SIZE 512

#define LARGE_BLOCK_SIZE (1024* TAR_BLK_SIZE)
#define MAX_LARGE_BLOCKS 4096

size_t nbytes = 0UL;
void* large_blocks[MAX_LARGE_BLOCKS];

static int addee_read(FILE *in_file) {
	size_t n;
	int do_next = 1;
	size_t nblocks = 0U;

	large_blocks[nblocks] = malloc(LARGE_BLOCK_SIZE);
	while (do_next)
		 {
		 	n = fread(large_blocks[nblocks], 1, LARGE_BLOCK_SIZE, in_file);
			if (n > 0) {
				if (n==LARGE_BLOCK_SIZE) {
					nbytes += n;
					if  (nblocks == MAX_LARGE_BLOCKS) {
						fprintf(stderr,"Read runctated due to hard limit size reached\n");
						return 127;
						}
					large_blocks[++nblocks] = malloc(LARGE_BLOCK_SIZE);
				} else {
					nbytes+=n;
		 		}
	 		}
		 	do_next = n== LARGE_BLOCK_SIZE;
		 	}
	return ferror(in_file);
}



static int write_header(size_t sz, const char* name, FILE* tar) {
	time_t t;
	struct tm * lt;
	unsigned char b[TAR_BLK_SIZE];
	struct posix_header* s = (struct posix_header*) &b;
	int sum = 0;

	memset(b,0,TAR_BLK_SIZE);
	b[0]='q';
	strncpy(s->name,name,100);               /*   0 */
	strcpy(s->mode,"0000644");               /*   0 */
	sprintf(s->size,"%0.11lo",sz);                /* 124 */
	time(&t);
	lt= localtime( &t );
	sprintf(s->mtime,"%0.11lo",(long)t);
	memset(s->chksum,' ',8);
	s->typeflag = REGTYPE;/*file*/                /* 156 */
	sprintf (s->magic,TMAGIC);                /* 257 */
	strncpy(s->version,TVERSION,2);              /* 263 */

	for (int i=0; i<TAR_BLK_SIZE; ++i)
	sum += b[i];
	sprintf(s->chksum, "%7o", sum);
	fwrite(s,TAR_BLK_SIZE,1,tar);
	return ferror(tar);
  }

static int tar_write(FILE* tar, const char* name) {
	struct flock lock;
	int err;
	size_t nblock = 0UL;
	size_t written = 0UL;
	lock.l_type=F_WRLCK;
	lock.l_whence=SEEK_SET;
	lock.l_start=0;   /* Starting offset for lock */
	lock.l_len = 0;
	lock.l_pid  = 0;
	fcntl(fileno(tar), F_SETLKW, lock);
	/*fseek(tar, 0, SEEK_END)*/;
	err = write_header( nbytes, name, tar );
	if (err) return err;

	while (written < nbytes) {
		size_t towrite = nbytes-written;
		if (towrite >= LARGE_BLOCK_SIZE) towrite = LARGE_BLOCK_SIZE;
		else {
			size_t tar_block_offset = towrite%TAR_BLK_SIZE;
			if (tar_block_offset) {
					memset(large_blocks[nblock]+towrite,'|',TAR_BLK_SIZE-tar_block_offset);
					towrite+= TAR_BLK_SIZE-tar_block_offset;
					}
		}
		fwrite(large_blocks[nblock++],1,towrite,tar);
		int err = ferror(tar);
		if (err) return err;
		written+=towrite;
	}
		

	fflush(tar);
	lock.l_type=F_UNLCK;
	fcntl(fileno(tar), F_SETLK, lock);

	return 0;
	}

static void usage(const char* prog) {
		fprintf(stderr, "Usage: %s tarfile file_name_for_stdin_read\n", prog) ;
	}
	
int main(int argc, char **argv) {
		if (argc != 3 || argv[2][0] == '-' || argv[1][0] == '-') {
			usage(*argv);
			exit(1);
		}
		int e = addee_read( stdin );
		if (e) return e;
		if (!nbytes) {
			fprintf (stderr,"Zero lenght file ignored\n");
			return 0;
		}

		FILE* tar = fopen(argv[1],"ab");
		e = errno;
		if (e) {
			perror(argv[1]);
			exit (e);
		}
	
		//Daemonize to write tar without interrupt
		setsid();
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
		if (fork()) exit(0);

		//Append to the tar
		e = tar_write(tar, argv[2]);
		return e;
	}
