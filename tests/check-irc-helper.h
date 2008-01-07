#include <stdlib.h>
#include <string.h>

#include <check.h>
#include "check-helpers.h"

#include <libempathy/empathy-irc-server.h>
#include <libempathy/empathy-irc-network.h>
#include <libempathy/empathy-irc-network-manager.h>

#ifndef __CHECK_IRC_HELPER_H__
#define __CHECK_IRC_HELPER_H__

void check_server (EmpathyIrcServer *server, const gchar *_address,
    guint _port, gboolean _ssl);

#endif /* __CHECK_IRC_HELPER_H__ */
