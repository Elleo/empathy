#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include "check-helpers.h"
#include "check-libempathy.h"

#include <libempathy/empathy-irc-server.h>

TCase *
make_empathy_irc_server_tcase (void)
{
    TCase *tc = tcase_create ("empathy-irc-server");
    return tc;
}
