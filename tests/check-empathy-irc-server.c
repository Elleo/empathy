#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include "check-helpers.h"
#include "check-libempathy.h"

#include <libempathy/empathy-irc-server.h>

START_TEST (test_empathy_irc_server_new)
{
  EmpathyIrcServer *server;
  gchar *address;
  guint port;
  gboolean ssl;

  server = empathy_irc_server_new ("test.localhost", 6667, TRUE);
  fail_if (server == NULL);

  g_object_get (server,
      "address", &address,
      "port", &port,
      "ssl", &ssl,
      NULL);

  fail_if (address == NULL || strcmp (address, "test.localhost") != 0);
  fail_if (port != 6667);
  fail_if (!ssl);

  g_free (address);
  g_object_unref (server);
}
END_TEST


TCase *
make_empathy_irc_server_tcase (void)
{
    TCase *tc = tcase_create ("empathy-irc-server");
    tcase_add_test (tc, test_empathy_irc_server_new);
    return tc;
}
