#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include "check-helpers.h"
#include "check-libempathy.h"

#include <libempathy/empathy-irc-network.h>

START_TEST (test_empathy_irc_network_new)
{
  EmpathyIrcNetwork *network;
  gchar *id = NULL, *name = NULL;

  network = empathy_irc_network_new ("id1", "Network1");

  g_object_get (network,
      "id", &id,
      "name", &name,
      NULL);

  fail_if (id == NULL || strcmp (id, "id1") != 0);
  fail_if (name == NULL || strcmp (name, "Network1") != 0);

  g_free (id);
  g_free (name);
  g_object_unref (network);
}
END_TEST

START_TEST (test_property_change)
{
  EmpathyIrcNetwork *network;
  gchar *name = NULL;

  network = empathy_irc_network_new ("id1", "Network1");

  g_object_set (network,
      "name", "Network2",
      NULL);

  g_object_get (network,
      "name", &name,
      NULL);

  fail_if (name == NULL || strcmp (name, "Network2") != 0);

  g_free (name);
  g_object_unref (network);

}
END_TEST

TCase *
make_empathy_irc_network_tcase (void)
{
    TCase *tc = tcase_create ("empathy-irc-network");
    tcase_add_test (tc, test_empathy_irc_network_new);
    tcase_add_test (tc, test_property_change);
    return tc;
}
