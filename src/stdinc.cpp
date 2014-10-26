
#include "precomp.h"
#include "stdinc.h"

namespace godwhale {

tree_t * g_ptree;

unsigned int state_node = (
    node_do_mate | node_do_null | node_do_futile |
    node_do_recap | node_do_recursion | node_do_hashcut);

} // namespace godwhale
