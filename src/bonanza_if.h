
/* bonanza‘¤‚©‚çQÆ‚·‚éŠÖ”/•Ï”‚Í‚·‚×‚Ä extern "C" ‚ÅéŒ¾‚µ‚Ü‚·B*/
#ifdef __cplusplus
extern "C" {
#endif

#if defined(GODWHALE_SERVER)
extern int CONV detect_signals_server();
extern void CONV set_position_hook(min_posi_t const * posi);
extern void CONV init_game_hook();
extern void CONV quit_game_hook();
extern void CONV make_move_root_hook(move_t move);
extern void CONV unmake_move_root_hook();
extern void CONV adjust_time_hook(int turn);
extern int CONV server_iterate(tree_t *restrict ptree, int *value,
                               move_t *pvseq, int *pvseq_length);
#endif

#ifdef __cplusplus
}
#endif
