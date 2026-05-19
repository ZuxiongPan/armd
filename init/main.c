#include "armd_event_loop.h"
#include "armd_log.h"

int main(int argc, const char *argv[])
{
    armd_log("event loop start ...\n");

    armd_event_loop_init();

    armd_event_loop_run();

    armd_event_loop_exit();

    return 0;
}