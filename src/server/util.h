#ifndef GODWHALE_SERVER_UTILITY_H
#define GODWHALE_SERVER_UTILITY_H

namespace godwhale {
namespace server {

extern bool IsThinkEnd(tree_t *restrict ptree, unsigned int turnTimeMS);

extern std::string ToString(move_t move);

} // namespace server
} // namespace godwhale

#endif
