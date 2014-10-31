#ifndef GODWHALE_STDINC_H
#define GODWHALE_STDINC_H

#include "precomp.h"
#include "bonanza6/shogi.h"

#define LOCK_IMPL(obj, lock, flag)       \
    if (bool flag = false) {} else       \
    for (ScopedLock lock(obj); !flag; flag = true)

/** オリジナルのロック定義 */
#define LOCK(obj) \
    LOCK_IMPL(obj, BOOST_PP_CAT(lock_, __LINE__), BOOST_PP_CAT(flag_, __LINE__))

#define F(fmt) ::boost::format(fmt)

namespace godwhale {

using boost::shared_ptr;
using boost::make_shared;
using boost::weak_ptr;
using boost::enable_shared_from_this;
using boost::lexical_cast;

typedef boost::asio::ip::tcp tcp;
typedef boost::recursive_mutex Mutex;
typedef boost::recursive_mutex::scoped_lock ScopedLock;

/**
 * @brief bonanzaの探索時に使います。
 */
enum
{
    PROCE_OK = 0,
    PROCE_ABORT = 1,
    PROCE_CONTINUE = 2,
};

/**
 * @brief 評価値の種類を示します。
 */
enum ULEType
{
    /// 特になし？
    ULE_NONE  = 0,
    /// 数字が評価値の下限値であることを示す。
    ULE_LOWER = 1,
    /// 数字が評価値の上限値であることを示す。
    ULE_UPPER = 2,
    /// 数字が評価値そのものであることを示す。
    ULE_EXACT = 3,
};

/**
 * @brief 値の種類を示します。
 */
enum ValueType
{
    VALUETYPE_ALPHA,
    VALUETYPE_BETA,
};

extern tree_t * g_ptree;

namespace server {
    using namespace boost;
    using namespace boost::system;
} // namespace server

} // namespace godwhale

#include "logger.h"
#include "util.h"

#endif
