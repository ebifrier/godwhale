
#include <stdlib.h>

int main(int argc, char *argv[])
{
    system("mpiexec -host localhost -n 1 gmx usi -tf -y0 -h0 -a -m0 : "
           "        -host localhost -n 1 gmx         -h3 -a -s1 -3");

    return 0;
}
