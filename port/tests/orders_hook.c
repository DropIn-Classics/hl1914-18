/* orders_hook.c - for the test programs that link draw.c without
 * orders.c: the hook fight_panel sends its waits to (hl.h).  With
 * LOAD_TEST_STEPS set (load_test's `fpanel`), the video memory at each
 * of the panel's steps, where the original calls wait_frames(6)
 * (S131D:1C9A, 1EE7), is written to that name followed by the step's
 * number and .vram (as -vram writes it).
 */
#include <stdio.h>
#include <stdlib.h>
#include "hl.h"
#include "vga.h"

static void steps_dump(int at, int n)
{
    static int step;
    char name[512];

    if (getenv("LOAD_TEST_STEPS") && at == ORDERS_WAIT && n == 7) {
        snprintf(name, sizeof name, "%s%d.vram", getenv("LOAD_TEST_STEPS"), ++step);
        vga_write_planes(name);
    }
}

void (*orders_hook)(int at, int n) = steps_dump;
