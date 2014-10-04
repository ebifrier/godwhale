#ifndef GODWHALE_STDINC_H
#define GODWHALE_STDINC_H

#include "precomp.h"
#include "bonanza6/shogi.h"

#define LOCK_IMPL(obj, lock, flag)       \
    if (bool flag = false) {} else       \
    for (ScopedLock lock(obj); !flag; flag = true)

/** オリジナルのロック定義 */
#define LOCK(obj) \
    LOCK_IMPL(obj, lock_7D0B4924019B, flag_7D0B4924019B)

namespace godwhale {

using namespace boost;
using namespace boost::system;
typedef boost::asio::ip::tcp tcp;

typedef boost::recursive_mutex Mutex;
typedef boost::recursive_mutex::scoped_lock ScopedLock;

} // namespace godwhale

#include "logger.h"
#include "util.h"

#endif
