/*
* Programming Assignment 02: lsv1.5.0
* This is the source file of version 1.5.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.5.0 
*       % lsv1.5.0  /home
*       $ lsv1.5.0  /home/kali/   /etc/
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

#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"


typedef enum { MODE_DEFAULT = 0, MODE_LONG = 1, MODE_HORIZONTAL = 2} display_mode_t;

extern int errno;

void do_ls(const char *dir, display_mode_t mode, bool show_hidden);
void do_ls_long(const char *dir, bool show_hidden);
long long get_block_size(const char *dir, bool show_hidden);
void get_file_permissions(int mode, char str[]);
char get_file_type(int mode);
void format_time(time_t file_epoch, char *out_str);
char **read_filenames(const char *dir, int *num_files, int *max_len, bool show_hidden);
void calculate_layout(int num_files, int max_len, int *num_cols, int *num_rows);
int get_terminal_width();
void print_columns(const char * dir, char **filenames, int num_files, int num_cols, int num_rows, int max_len);
void print_horizontal(const char * dir, char **filenames, int num_files, int max_len);
static int cmp_strings(const void *a, const void *b);
void print_colored(const char *name, mode_t mode); 

int main(int argc, char const *argv[])
{
	int opt;
	bool long_listing = false;  
    	display_mode_t mode = MODE_DEFAULT;
   
    
	// Accept both -l and -x
    	bool show_hidden = false;
	while ((opt = getopt(argc, (char * const *)argv, "lxa")) != -1) {
		switch (opt) {
			case 'l':
				// long listing should take precedence
				mode = MODE_LONG;
				break; 
			case 'x':
				if (mode != MODE_LONG) // don't override long if already set
					mode = MODE_HORIZONTAL;
				break;
			case 'a':
				show_hidden = true;
				break;
			default:
				fprintf(stderr, "Usage: %s [-l] [-x] [directories...]\n", argv[0]);
			   	exit(EXIT_FAILURE);
        
		}
    
	}

	if (optind == argc) {
	 // no directory arg -> use "."
        	if (mode == MODE_LONG)
          	  	do_ls_long(".", show_hidden);
		else
			do_ls(".", mode, show_hidden);   // modified do_ls signature (see below)
	}
       	else {       
		for (int i = optind; i < argc; i++) {            
			printf("Directory listing of %s:\n", argv[i]);          
		       	if (mode == MODE_LONG)
				do_ls_long(argv[i], show_hidden);
		       	else
			       	do_ls(argv[i], mode, show_hidden);
		       	puts("");
		}
	}
}

void do_ls(const char *dir, display_mode_t mode, bool show_hidden)
{
    int num_files = 0;
    int max_len = 0;
    char **filenames = read_filenames(dir, &num_files, &max_len, show_hidden);
    if (filenames == NULL) {
        return;
    }

    if (num_files == 0) {
        free(filenames);
        return;
    }

    /* sort alphabetically */
    if (num_files > 1) {
        qsort(filenames, (size_t)num_files, sizeof(char *), cmp_strings);
    }

    if (mode == MODE_HORIZONTAL) {
        print_horizontal(dir, filenames, num_files, max_len);
    } else {
        // default: down then across
        int num_cols, num_rows;
        calculate_layout(num_files, max_len, &num_cols, &num_rows);
        print_columns(dir, filenames, num_files, num_cols, num_rows, max_len);
    }

    // free memory
    for (int i = 0; i < num_files; i++) free(filenames[i]);
    free(filenames);
}



void do_ls_long(const char *dir, bool show_hidden) {

    long long total_block_size = get_block_size(dir, show_hidden);
    printf("total %lld\n", total_block_size/2);
    int num_files = 0, max_len = 0;
    char **filenames = read_filenames(dir, &num_files, &max_len, show_hidden);
    if (!filenames) return;
    if (num_files > 1) {
        qsort(filenames, (size_t)num_files, sizeof(char *), cmp_strings);
    }
    for (int i = 0; i < num_files; ++i) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, filenames[i]);

        struct stat info;
        if (lstat(path, &info) == -1) {
            perror("lstat failed");
            continue;
        }
        struct group *grp = getgrgid(info.st_gid);
        struct passwd *pwd = getpwuid(info.st_uid);
        char filePermissions[10];
        get_file_permissions(info.st_mode, filePermissions);
        char fileType = get_file_type(info.st_mode);
        char ls_time[16];
        format_time(info.st_mtime, ls_time);
	printf("%c%s %ld %s %s %ld %s ",fileType, filePermissions, info.st_nlink, pwd->pw_name, grp->gr_name, info.st_size, ls_time);
	print_colored(filenames[i], info.st_mode);
	printf("\n");
    }
    // free memory
    for (int i = 0; i < num_files; ++i) free(filenames[i]);
    free(filenames);
}

long long get_block_size(const char *dir, bool show_hidden){
	DIR *dp = opendir(dir);
        if (dp == NULL)
        {
                fprintf(stderr, "Cannot open directory : %s\n", dir);
		closedir(dp);
                return 0;
        }

	long long total_blocks = 0;
	struct dirent *entry;
    	// First pass: calculate total blocks
	while ((entry = readdir(dp)) != NULL) {
		if (entry->d_name[0] == '.' && !show_hidden)
			continue;
		char path[1024];
		snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
		struct stat info;
		if (lstat(path, &info) == -1)
			continue;
		total_blocks += info.st_blocks;
	}
	// Reset directory stream to beginning
	closedir(dp);
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

char **read_filenames(const char *dir, int *num_files, int *max_len, bool show_hidden) {
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
        if (entry->d_name[0] == '.' && show_hidden == false)
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

void print_columns(const char * dir, char **filenames, int num_files, int num_cols, int num_rows, int max_len) {
    int spacing = 2;
    char path[1024];
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            int idx = col * num_rows + row;
            if (idx >= num_files)
                continue;
	    snprintf(path, sizeof(path), "%s/%s", dir, filenames[idx]);
            struct stat info;
            int name_len = (int)strlen(filenames[idx]);
	    if (lstat(path, &info) == -1) {
                printf("%-*s", max_len + spacing, filenames[idx]);
                continue;
            }
	    /* print colored filename, then pad with spaces for alignment */
            print_colored(filenames[idx], info.st_mode);
	    int pad = max_len - name_len + spacing;
            for (int p = 0; p < pad; p++) putchar(' ');
           
        }
        printf("\n");
    }
}

// spacing between columns
static const int COL_SPACING = 2;

void print_horizontal(const char * dir, char **filenames, int num_files, int max_len) {
    int term_width = get_terminal_width();
    int col_width = max_len + COL_SPACING;
    if (col_width <= 0) col_width = 1;

    int current_pos = 0; // current horizontal position in characters
    char path[1024];

    for (int i = 0; i < num_files; i++) {
        // If filename itself longer than terminal width, print it on its own line
        int name_len = strlen(filenames[i]);
        // If adding this column would exceed terminal width, wrap
        if (current_pos != 0 && (current_pos + col_width) > term_width) {
            printf("\n");
            current_pos = 0;
        }
        snprintf(path, sizeof(path), "%s/%s", dir, filenames[i]);
        struct stat info;
	if (lstat(path, &info) == -1){
            // If filename is longer than column width, print it then a newline
            if (name_len > max_len) {
                // print directly (no padding) and wrap to next line
                printf("%s\n", filenames[i]);
                current_pos = 0;
             }else {
	     	printf("%-*s", col_width, filenames[i]);
                current_pos += col_width;
	     }
	    continue;
        }
        if (name_len > max_len) {
            print_colored(filenames[i], info.st_mode);
            printf("\n");
            current_pos = 0;
            continue;
        }
	print_colored(filenames[i], info.st_mode);
        int pad = col_width - name_len;
        for (int p = 0; p < pad; p++) putchar(' ');
        current_pos += col_width;
    }
    // finish with newline if we didn't just print one
    if (current_pos != 0)
        printf("\n");
}

// comparator for qsort - sorts strings alphabetically using strcmp
static int cmp_strings(const void *a, const void *b) {
    const char *const *sa = a;
    const char *const *sb = b;
    return strcmp(*sa, *sb);
}


void print_colored(const char *name, mode_t mode) {
    if (S_ISDIR(mode)) {
        printf("%s%s%s", COLOR_BLUE, name, COLOR_RESET);
    } else if (S_ISLNK(mode)) {
        printf("%s%s%s", COLOR_PINK, name, COLOR_RESET);
    } else if (mode & S_IXUSR) { // executable by owner
        printf("%s%s%s", COLOR_GREEN, name, COLOR_RESET);
    } else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip")) {
        printf("%s%s%s", COLOR_RED, name, COLOR_RESET);
    } else if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISSOCK(mode) || S_ISFIFO(mode)) {
        printf("%s%s%s", COLOR_REVERSE, name, COLOR_RESET);
    } else {
        printf("%s", name); // default color
    }
}

