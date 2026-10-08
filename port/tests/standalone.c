/* standalone.c - for the test programs that link battle.c without the
 * menu, the play and the saved games: what battle_begin reaches only with
 * a saved game to load or to write (load_game, save_game: a test that
 * gets there stops and says so) or on a map with new units' screens (briefing.c).  Of those screens
 * the two texts in the menu's font are not drawn here (text_mid_uncut is
 * menu.c's), no time passes and no key is waited for: a test that
 * compares memory behind battle_begin on such a map compares the files
 * the screens load, not their pictures.
 */
#include <stdio.h>
#include <stdlib.h>
#include "hl.h"

static void not_here(const char *what)
{
    fprintf(stderr, "%s is not linked into this test program (port/tests/standalone.c)\n", what);
    exit(2);
}

int16_t load_game(Far buf)
{
    (void)buf;
    not_here("load_game");
    return 0;
}

/* loop.c's key for saving (F1 and a digit in a battle) */
uint16_t save_game(Far buf)
{
    (void)buf;
    not_here("save_game");
    return 0;
}

void text_mid_uncut(int16_t y, Far text, Far font, uint16_t base, int16_t gap)
{
    (void)y; (void)text; (void)font; (void)base; (void)gap;
}

void inputs_release(int wait) { (void)wait; }
void play_tick(void) { }
void play_retraces(int n) { (void)n; }
int play_idle(void) { return 1; }
int plat_pump(void) { return 1; }
