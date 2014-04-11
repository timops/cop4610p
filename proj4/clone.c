/*
 *
 * clone.c
 * Tim Green
 * 4/10/14
 * version 1.0
 *
 * Project 4: Clone Utility
 *
 * clone.x <source> <dest>
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>

#define _GNU_SOURCE

#define MAX_DIR 30
#define BUF_SIZE 4096

char *build_path(char * raw_path, int new);
void create_dir(DIR *dirp, char *src_path, char *dest_path);
void change_perms(char * new_dst, struct stat * curr_ent);
void make_directory(char * new_src);
void copy_file(char * new_src, char * new_dst);

int main(int argc, char** argv)
{
  char source_dir[MAX_DIR];
  char dest_dir[MAX_DIR];

  char *real_source;
  char *real_dest;

  DIR *dirp;

  // Program only functions correctly with 3 arguments, print some help if misused
  if (argc < 3)
  {
    printf("\nHelp:\n");
    printf("clone.x <source> <dest>\n\n");
    exit(0);
  }

  strcpy(source_dir, argv[1]);
  strcpy(dest_dir, argv[2]);

  real_source = build_path(source_dir, 0);
  if (real_source == NULL)
  {
    printf("%s is not a valid source directory.\n", source_dir);
    exit(1);
  }

  real_dest  = build_path(dest_dir, 1);
  if (strcmp(real_source, real_dest) == 0)
  {
    free(real_source);
    free(real_dest);
    exit(0);
  }
  
  if (mkdir(real_dest, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
    printf("Directory %s already exists, skipping create.\n", real_dest);

  dirp = opendir(real_source);
  create_dir(dirp, real_source, real_dest);

  free(real_source);
  free(real_dest);

  closedir(dirp);

  return 0;
}

/*
 *
 * This function takes a user specified path, detects if it is a relative path,
 *   and converts it to a real path (if previous condition is true).
 *
 */
char *build_path(char * raw_path, int new)
{
  char *new_path;
 
  if (new)
  {
    // realpath allocates its own memory, so we only create a pointer.
    new_path = realpath(raw_path, NULL);
    if (new_path == NULL)
    {
      new_path = realpath(".", NULL);
      sprintf(new_path, "%s/%s", new_path, raw_path);
    }
  } 
  else
  {
    new_path = realpath(raw_path, NULL);
  }
  return new_path;
}

/*
 *
 * Recursively copy files and directories.
 *
 */
void create_dir(DIR *dirp, char *src_path, char *dst_path)
{
  struct dirent *dp;
  struct stat curr_ent;

  const char *CURR = ".";
  const char *PARENT = "..";

  char *new_src;
  char *new_dst;

  DIR *subdir = NULL;

  new_src = (char *) malloc(sizeof(char) * MAXPATHLEN);
  new_dst = (char *) malloc(sizeof(char) * MAXPATHLEN);

  strcpy(new_src, src_path);

  do {
    if ( (dp = readdir(dirp)) != NULL )
    {
      // Skip current directory and parent directory ('.' and '..')
      if (strcmp(dp->d_name, CURR) == 0)
        continue;
      else if (strcmp(dp->d_name, PARENT) == 0)
        continue;

      sprintf(new_src, "%s/%s", src_path, dp->d_name);
      
      // This will check if the dest directory.
      if (strcmp(new_src, dst_path) == 0)
        continue;

      sprintf(new_dst, "%s/%s", dst_path, dp->d_name);

      stat(new_src, &curr_ent);
      if (S_ISDIR(curr_ent.st_mode))
      {
        // Create target directory and update perms.
        make_directory(new_dst);
        change_perms(new_dst, &curr_ent);
        subdir = opendir(new_src);
        create_dir(subdir, new_src, new_dst);
        closedir(subdir);
      }
      else if (S_ISREG(curr_ent.st_mode))
      {
        // Create target file and update perms.
        printf("Copying %s to %s\n", new_src, new_dst);
        copy_file(new_src, new_dst);
        change_perms(new_dst, &curr_ent);
      }
    }
  } while (dp != NULL);

  free(new_src);
  free(new_dst);
}

/*
 *
 * Create a new directory; really just a wrapper for mkdir syscall.
 *
 */
void make_directory(char * new_dst)
{
  printf("Creating directory %s\n", new_dst);
  if ( mkdir(new_dst, S_IRWXU) != 0 )
  {
    if (errno == EEXIST)
      return;
    else
      perror("[ERROR]");
  }
}

/*
 *
 * Allocate some memory and copy files (with some error checking)
 *
 */
void copy_file(char * new_src, char * new_dst)
{
  int fd_src, fd_dest;
  char *buf;

  size_t bytes = BUF_SIZE;
  ssize_t bytes_read, bytes_written;

  if ( (fd_src = open(new_src, O_RDONLY)) == -1)
  {
    perror("[ERROR]");
    return;
  }
  if ( (fd_dest = open(new_dst, O_CREAT | O_RDWR)) == -1)
  {
    perror("[ERROR]");
    close(fd_src);
    return;
  }

  buf = (char *) malloc(sizeof(char) * BUF_SIZE);

  while ( (bytes_read = read(fd_src, buf, bytes)) != 0 )
  {
    // something very bad happened if we get less than zero.
    if (bytes_read < 0)
      perror("[ERROR]");
    else
    {
      if ( (bytes_written = write(fd_dest, buf, bytes_read)) == -1)
        perror("[ERROR]");
    }
  }

  close(fd_src);
  close(fd_dest);

  free(buf);
}

/*
 *
 * This procedure handles uid, gid, and umask changes.
 *
 */
void change_perms(char * new_dst, struct stat * curr_ent)
{
  printf("Setting permissions for %s: %o\n", new_dst, curr_ent->st_mode);
  if (chmod(new_dst, curr_ent->st_mode) != 0)
  {
    perror("[ERROR]");
    return;
  }
  printf("Setting user and group for %s: %d, %d\n", new_dst, curr_ent->st_uid, curr_ent->st_gid);
  if (chown(new_dst, curr_ent->st_uid, curr_ent->st_gid) != 0)
  {
    perror("[ERROR]");
    return;
  }
}
