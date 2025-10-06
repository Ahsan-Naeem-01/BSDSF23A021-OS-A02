/*
* Programming Assignment 02: lsv1.1.0
* This is the source file of version 1.1.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.1.0 
*       % lsv1.1.0  /home
*       $ lsv1.1.0  /home/kali/   /etc/
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>

extern int errno;

void do_ls(const char *dir);
void do_ls_long(const char *dir);
long long get_block_size(DIR * dp, const char *dir);
int main(int argc, char const *argv[])
{
    int opt;
    bool long_listing = false; 

    // Parse command-line options
    while ((opt = getopt(argc, (char * const *)argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directories...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // If no directory is given, use "."
    if (optind == argc) {
        if (long_listing)
            do_ls_long(".");
        else
            do_ls(".");
    } 
    else {
        // Process all directories listed
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            if (long_listing)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i]);
            puts("");
        }
    }

    return 0;
}




void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

void do_ls_long(const char *dir){
	struct dirent *entry;
   	DIR *dp = opendir(dir);
   	if (dp == NULL)
    	{
       	 	fprintf(stderr, "Cannot open directory : %s\n", dir);
        	return;
    	}
	long long total_block_size = get_block_size(dp, dir);
	printf("total %lld\n", total_block_size/2);
	while ((entry = readdir(dp)) != NULL)
    	{
        	if (entry->d_name[0] == '.')
           		 continue;
	       	char path[1024];
       		snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

		struct stat info;
		int rv = lstat(path, &info);
		if (rv == -1){
 			perror("stat failed");
			exit(1);
		}
		printf("%o %ld %d %d %ld %ld %s\n", info.st_mode, info.st_nlink, info.st_uid, info.st_gid, info.st_size, info.st_mtime, entry->d_name);
    	}
}

long long get_block_size(DIR * dp, const char *dir){
	long long total_blocks = 0;
	struct dirent *entry;
    	// First pass: calculate total blocks
	while ((entry = readdir(dp)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;
		char path[1024];
		snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
		struct stat info;
		if (lstat(path, &info) == -1)
			continue;
		total_blocks += info.st_blocks;
	}
	// Reset directory stream to beginning
	rewinddir(dp);
	return total_blocks;
}
