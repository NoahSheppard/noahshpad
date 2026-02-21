#include <string.h>

#include "d_net.h"
#include "doomstat.h"
#include "i_net.h"

void I_InitNetwork(void) {
    static doomcom_t local_doomcom;

    memset(&local_doomcom, 0, sizeof(local_doomcom));
    local_doomcom.id = DOOMCOM_ID;
    local_doomcom.consoleplayer = 0;
    local_doomcom.numplayers = 1;
    local_doomcom.numnodes = 1;
    local_doomcom.ticdup = 1;

    doomcom = &local_doomcom;
    netgame = false;
}

void I_NetCmd(void) {
    if (!doomcom) {
        I_InitNetwork();
    }
    doomcom->remotenode = -1;
}
