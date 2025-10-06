
#### Q1. What is the crucial difference between the stat() and lstat() system calls? In the context of the ls command, when is it more appropriate to use lstat()?

Ans. The crucial difference between stat() and lstat() is how they handle symbolic links. The stat() system call returns information about the target file that a symbolic link points to, while lstat() returns information about the link itself. In simple words, stat() follows the link to see the file it points to, but lstat() looks at the link as it is. In the context of the ls command, it is more appropriate to use lstat() when listing files because ls -l wants to show details about symbolic links themselves, such as the link name, its permissions, and where it points, rather than the properties of the file the link points to. This way, symbolic links are displayed correctly instead of showing information about their target files.
#### Q2. The st_mode field in struct stat is an integer that contains both the file type (e.g., regular file, directory) and the permission bits. Explain how you can use bitwise operators (like &) and predefined macros (like S_IFDIR or S_IRUSR) to extract this information.

Ans. The st_mode field in struct stat contains information about both the file type and the file permissions in a single integer. To extract the file type, we use the bitwise AND operator & with the mask S_IFMT. This isolates the bits that represent the file type, which can then be compared with predefined macros like S_IFDIR for directories, S_IFREG for regular files, or S_IFLNK for symbolic links. For example, (st.st_mode & S_IFMT) == S_IFDIR checks if the file is a directory.  

Similarly, the permission bits can be extracted using & with macros such as S_IRUSR (owner read), S_IWUSR (owner write), S_IXUSR (owner execute), and corresponding macros for group and others. For instance, (st.st_mode & S_IRUSR) checks if the owner has read permission. Using bitwise AND in this way allows you to selectively check which permissions are set and what type of file you are dealing with.


