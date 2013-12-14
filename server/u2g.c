
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
      "mpiexec -host localhost -n 1 \"%s" SEP TARGET "\" usi -h0 -m0 : "
      "        -host localhost -n 1 \"%s" SEP TARGET "\"     -h3 -s1 -1 : "
      "        -host 54.235.70.35 -n 1 \"%s" SEP TARGET "\"     -h3 -s2 -8",
             buf, buf, buf);

    system(command);
    return 0;
}
