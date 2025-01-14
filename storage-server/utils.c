#include "headers.h"

extern pthread_mutex_t file_locks[MAX_PATHS];

char *extract_file_name(char *path)
{
    char *file_name = strrchr(path, '/'); // Look for the last '/' in the path
    if (file_name)
    {
        return file_name + 1; // Skip the '/' to get the file name
    }
    return path; // If no '/' is found, the entire path is the file name
}

// Function to clear the socket buffer
void clear_socket_buffer(int socket_fd)
{
    char temp_buffer[MAX_BUFFER];
    ssize_t bytes_read;
    do
    {
        bytes_read = recv(socket_fd, temp_buffer, sizeof(temp_buffer), MSG_DONTWAIT);
    } while (bytes_read > 0);
}

void initialize_locks()
{
    for (int i = 0; i < MAX_PATHS; i++)
    {
        pthread_mutex_init(&file_locks[i], NULL);
    }
}

// Function to get a lock index based on file path (simple hash function)
int get_lock_index(const char *path)
{
    unsigned int hash = 0;
    while (*path)
    {
        hash = (hash * 31 + *path++) % MAX_PATHS;
    }
    return hash;
}
int delete_directory_recursively(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        perror(RED "Error opening directory" RESET);
        return -1;
    }

    struct dirent *entry;
    char filepath[MAX_BUFFER];

    while ((entry = readdir(dir)) != NULL)
    {
        // Skip `.` and `..`
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
        struct stat entry_stat;

        if (stat(filepath, &entry_stat) == 0)
        {
            if (S_ISDIR(entry_stat.st_mode))
            {
                // Recursively delete the subdirectory
                if (delete_directory_recursively(filepath) != 0)
                {
                    closedir(dir);
                    return -1;
                }
            }
            else
            {
                // Delete the file
                if (remove(filepath) != 0)
                {
                    perror(RED "Error deleting file" RESET);
                    closedir(dir);
                    return -1;
                }
            }
        }
        else
        {
            perror(RED "Error getting file information" RESET);
            closedir(dir);
            return -1;
        }
    }

    closedir(dir);

    // Remove the directory itself
    if (rmdir(path) != 0)
    {
        perror(RED "Error deleting directory" RESET);
        return -1;
    }

    return 0;
}
// Recursive function to get all file and directory paths
int get_all_paths(char *base_dir, char paths[][MAX_PATH_LEN], int *path_count)
{

    if ((*path_count) == 0)
    {
        // Use strlen to determine the length of the string
        size_t base_dir_len = strlen(base_dir);

        // Ensure the length does not exceed MAX_PATH_LEN
        if (base_dir_len >= MAX_PATH_LEN)
        {
            fprintf(stderr, "Base directory path is too long\n");
            return -1; // Return an error
        }

        // Copy the base_dir to the paths array
        strncpy(paths[*path_count], base_dir, MAX_PATH_LEN - 1);
        paths[*path_count][MAX_PATH_LEN - 1] = '\0'; // Ensure null termination
        (*path_count)++;
    }

    DIR *dir;
    struct dirent *entry;
    struct stat path_stat;

    // Open the directory
    if ((dir = opendir(base_dir)) == NULL)
    {
        perror(RED "opendir failed" RESET);
        exit(EXIT_FAILURE);
    }

    // Read each entry in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Construct full path for the current entry
        char full_path[MAX_PATH_LEN];

        // Check if the full path length would exceed MAX_PATH_LEN
        if (strlen(base_dir) + strlen(entry->d_name) + 2 > MAX_PATH_LEN)
        {
            fprintf(stderr, RED "Path too long: %s/%s\n" RESET, base_dir, entry->d_name);
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, entry->d_name);

        // Add the path to the array if there's space
        if (*path_count < MAX_PATHS)
        {
            strncpy(paths[*path_count], full_path, sizeof(full_path)-1);
            paths[*path_count][sizeof(full_path)-1] = '\0'; // Ensure null termination
            (*path_count)++;
        }
        else
        {
            fprintf(stderr, RED "Maximum path count reached.\n" RESET);
            closedir(dir);
            return *path_count;
        }

        // If the entry is a directory, recurse into it
        if (stat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode))
        {
            get_all_paths(full_path, paths, path_count);
        }
    }

    closedir(dir);
    return *path_count;
}
int set_socket_info(int socket_fd, async *info, int remote)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // Use getpeername for remote sockets and getsockname for local sockets
    if (remote)
    {
        if (getpeername(socket_fd, (struct sockaddr *)&addr, &addr_len) == -1)
        {
            perror("getpeername failed");
            return -1;
        }
    }
    else
    {
        if (getsockname(socket_fd, (struct sockaddr *)&addr, &addr_len) == -1)
        {
            perror("getsockname failed");
            return -1;
        }
    }

    // Convert the IP to a readable string
    if (inet_ntop(AF_INET, &addr.sin_addr, info->ip, sizeof(info->ip)) == NULL)
    {
        perror("inet_ntop failed");
        return -1;
    }

    // Get the port number in host byte order
    info->port = ntohs(addr.sin_port);

    printf("Ip -> %s    port-> %d\n", info->ip, info->port);

    return 0; // Success
}
void get_file_info(char *file_path, char *response_data, size_t response_size)
{
    struct stat file_stat;

    if (stat(file_path, &file_stat) == -1)
    {
        snprintf(response_data, response_size, "Failed to get file info: %s", strerror(errno));
        return;
    }

    // Human-readable permissions
    char permissions[10];
    permissions[0] = (file_stat.st_mode & S_IRUSR) ? 'r' : '-'; // Owner read
    permissions[1] = (file_stat.st_mode & S_IWUSR) ? 'w' : '-'; // Owner write
    permissions[2] = (file_stat.st_mode & S_IXUSR) ? 'x' : '-'; // Owner execute
    permissions[3] = (file_stat.st_mode & S_IRGRP) ? 'r' : '-'; // Group read
    permissions[4] = (file_stat.st_mode & S_IWGRP) ? 'w' : '-'; // Group write
    permissions[5] = (file_stat.st_mode & S_IXGRP) ? 'x' : '-'; // Group execute
    permissions[6] = (file_stat.st_mode & S_IROTH) ? 'r' : '-'; // Others read
    permissions[7] = (file_stat.st_mode & S_IWOTH) ? 'w' : '-'; // Others write
    permissions[8] = (file_stat.st_mode & S_IXOTH) ? 'x' : '-'; // Others execute
    permissions[9] = '\0';

    // Format the file information
    snprintf(response_data, response_size,
             "File: %s\n"
             "Size: %lld bytes\n"
             "Permissions: %o (%s)\n"
             "Last Access: %s"
             "Last Modification: %s",
             file_path,
             (long long)file_stat.st_size,
             file_stat.st_mode & 0777,
             permissions,
             ctime(&file_stat.st_atime),
             ctime(&file_stat.st_mtime));
}