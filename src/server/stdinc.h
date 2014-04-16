#ifndef GODWHALE_SERVER_STDINC_H
#define GODWHALE_SERVER_STDINC_H

#include "precomp.h"
#include "../common/bonanza6/shogi.h"

namespace godwhale {
namespace server {

using namespace boost;
typedef boost::asio::ip::tcp tcp;

typedef boost::recursive_mutex Mutex;
typedef boost::recursive_mutex::scoped_lock ScopedLock;

} // namespace server
} // namespace godwhale

#include "logger.h"

#endif
