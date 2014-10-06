
#include "precomp.h"
#include "stdinc.h"
#include "util.h"

namespace godwhale {

const char *PieceStrTable[16]  = {
    "Z", "•à", "", "Œj", "‹â", "‹à",
    "Šp", "”ò", "‰¤", "ƒg", "ˆÇ", "NK",
    "NG", "~", "”n", "—³"
};

const char *TurnStrTable[] =
{
    "£", "¢", ""
};

/**
 * @brief —§‚Á‚Ä‚¢‚éƒrƒbƒg‚Ì”‚ð’²‚×‚Ü‚·B
 */ 
int popCount(int x)
{
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0f0f0f0f) + ((x >> 4) & 0x0f0f0f0f);
    x = (x & 0x00ff00ff) + ((x >> 8) & 0x00ff00ff);
    return ((x & 0x0000ffff) + ((x >> 16) & 0x0000ffff));
}

/**
 * @brief bonanza‚Ì’Tõ—p•Ï”‚ð‰Šú‰»‚µ‚Ü‚·B
 */ 
void initBonanzaSearch(tree_t * restrict ptree)
{
    ptree->node_searched = 0;
    ptree->nquies_called = 0;
    ptree->current_move[0] = 0;
    ptree->pv[0].a[0] = 0;
    ptree->pv[0].a[1] = 0;
    ptree->pv[0].depth = 0;
    ptree->pv[0].length = 0;

    root_abort = 0;
    root_alpha = -score_bound;
    root_beta =   score_bound;

    game_status &= ~(flag_move_now | flag_quit_ponder | flag_search_error);

    ptree->save_eval[0] =
    ptree->save_eval[1] = INT_MAX;

    for (int ply = 0; ply < PLY_MAX; ++ply) {
        ptree->amove_killer[ply].no1 =
        ptree->amove_killer[ply].no2 = 0;
    }

    ptree->nsuc_check[0] = 0;
    ptree->nsuc_check[1] = InCheck(root_turn) ? 1 : 0;

    ptree->move_last[0] = ptree->amove;
    ptree->move_last[1] = ptree->amove;
}

} // namespace godwhale
