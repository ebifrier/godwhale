
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  define SEP      "\\"
#  define TARGET   "gwx.exe"
#  define snprintf sprintf_s
#else
#  define SEP      "/"
#  define TARGET   "gwx"
#endif


static void
get_directory(const char *path, char *dir, size_t dir_size)
{
    const char *p;

    p = strrchr(path, '\\');
    if (p == NULL) {
        p = strrchr(path, '/');
    }

    snprintf(dir, dir_size, "%s", path);
    if (p != NULL) {
        int size = (int)(p - path);
        dir[size] = '\0';
    }
}


int
main(int argc, char *argv[])
{
    char buf[1024], command[1024];

    (void)argc;

    get_directory(argv[0], buf, sizeof(buf));

    snprintf(command, sizeof(command),
             "mpiexec -n 1 \"%s" SEP TARGET "\" usi -h0 -m0 : "
             "        -n 1 \"%s" SEP TARGET "\"     -h3 -s1 -1",
             buf, buf);

    system(command);
    return 0;
}
