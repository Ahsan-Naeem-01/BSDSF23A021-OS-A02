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
#include <grp.h>
#include <pwd.h>
#include <utime.h>
#include <time.h>
#include <sys/ioctl.h>


extern int errno;

void do_ls(const char *dir);
void do_ls_long(const char *dir);
long long get_block_size(DIR * dp, const char *dir);
void get_file_permissions(int mode, char str[]);
char get_file_type(int mode);
void format_time(time_t file_epoch, char *out_str);
char **read_filenames(const char *dir, int *num_files, int *max_len);
void calculate_layout(int num_files, int max_len, int *num_cols, int *num_rows);
int get_terminal_width();

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

void do_ls(const char *dir) {
	int num_files, max_len;
	char **filenames = read_filenames(dir, &num_files, &max_len);

	int num_cols, num_rows;
	calculate_layout(num_files, max_len, &num_cols, &num_rows);
 
	if (!filenames)
	       	return;
	for (int i = 0; i < num_files; i++) {
		printf("%s\n", filenames[i]);
	       	free(filenames[i]); // free individual strings
       	}
	free(filenames); // free array
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
		struct group * grp = getgrgid(info.st_gid);
		struct passwd * pwd = getpwuid(info.st_uid);
		char filePermissions[10];
		get_file_permissions(info.st_mode, filePermissions);
		char fileType = get_file_type(info.st_mode);
		char ls_time[16];
                format_time(info.st_mtime, ls_time);

		printf("%c%s %ld %s %s %ld %s %s\n",fileType, filePermissions, info.st_nlink, pwd->pw_name, grp->gr_name, info.st_size, ls_time, entry->d_name);

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

void get_file_permissions(int mode, char str[]){
	strcpy(str, "---------");
	//owner  permissions
  	 if((mode & 0000400) == 0000400) str[0] = 'r';
	if((mode & 0000200) == 0000200) str[1] = 'w';
	if((mode & 0000100) == 0000100) str[2] = 'x';
	//group permissions
	if((mode & 0000040) == 0000040) str[3] = 'r'; 
	if((mode & 0000020) == 0000020) str[4] = 'w';
     	if((mode & 0000010) == 0000010) str[5] = 'x';
	//others  permissions
	if((mode & 0000004) == 0000004) str[6] = 'r';
     	if((mode & 0000002) == 0000002) str[7] = 'w';
    	if((mode & 0000001) == 0000001) str[8] = 'x';
	//special  permissions
     	if((mode & 0004000) == 0004000) str[2] = 's';
     	if((mode & 0002000) == 0002000) str[5] = 's';
     	if((mode & 0001000) == 0001000) str[8] = 't';
}

char get_file_type(int mode){
	if ((mode &  0170000) == 0010000)
  		return 'p';
       	else if ((mode &  0170000) == 0020000)
		return 'c';
       	else if ((mode &  0170000) == 0040000)
        	return 'd';
	else if ((mode &  0170000) == 0060000)
               	return 'b';
       	else if ((mode &  0170000) == 0100000)
                return '-';
	else if ((mode &  0170000) == 0120000)
               	return 'l';
	else if ((mode &  0170000) == 0140000)
                return 's';
	else
		return 'u';//Unknown mode

}
void format_time(time_t file_epoch, char *out_str) {
       	time_t now = time(NULL);
     
	double diff = difftime(now, file_epoch);
	char *ctime_str = ctime(&file_epoch);
	if (diff > 15552000 || file_epoch > now) {
       	 	// older than 6 months → show year
		snprintf(out_str, 16, "%.3s %2.2s  %.4s",
               		ctime_str + 4, ctime_str + 8, ctime_str + 20);
       	} else {
	       	// recent → show HH:MM
		snprintf(out_str, 16, "%.3s %2.2s %5.5s",
         		ctime_str + 4, ctime_str + 8, ctime_str + 11);
    	}
}

char **read_filenames(const char *dir, int *num_files, int *max_len) {
    DIR *dp = opendir(dir);
    if (!dp) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return NULL;
    }

    struct dirent *entry;
    int capacity = 64; // initial capacity
    char **filenames = malloc(capacity * sizeof(char *));
    *num_files = 0;
    *max_len = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        int len = strlen(entry->d_name);
        if (len > *max_len)
            *max_len = len;

        if (*num_files >= capacity) {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
        }

        filenames[*num_files] = strdup(entry->d_name); // copy string
        (*num_files)++;
    }

    closedir(dp);
    return filenames;
}

int get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        // fallback if ioctl fails
        return 80;
    }
    return w.ws_col;
}

void calculate_layout(int num_files, int max_len, int *num_cols, int *num_rows) {
    int term_width = get_terminal_width();
    int spacing = 2; // space between columns
    *num_cols = term_width / (max_len + spacing);
    if (*num_cols < 1)
        *num_cols = 1;

    *num_rows = (num_files + *num_cols - 1) / *num_cols; // ceiling division
}

