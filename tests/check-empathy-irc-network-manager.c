#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include "check-helpers.h"
#include "check-libempathy.h"

#include <libempathy/empathy-irc-network-manager.h>

TCase *
make_empathy_irc_network_manager_tcase (void)
{
    TCase *tc = tcase_create ("empathy-irc-network-manager");
    return tc;
}
