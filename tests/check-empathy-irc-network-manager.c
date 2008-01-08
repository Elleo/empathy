#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include "check-helpers.h"
#include "check-libempathy.h"
#include "check-irc-helper.h"

#include <libempathy/empathy-irc-network-manager.h>

#define GLOBAL_SAMPLE "xml/default-irc-networks-sample.xml"

START_TEST (test_load_global_file)
{
  EmpathyIrcNetworkManager *mgr;
  gchar *global_file, *user_file;
  GSList *networks, *l;
  struct server_t freenode_servers[] = {
    { "irc.freenode.net", 6667, FALSE },
    { "irc.eu.freenode.net", 6667, FALSE }};
  struct server_t gimpnet_servers[] = {
    { "irc.gimp.org", 6667, FALSE },
    { "irc.us.gimp.org", 6667, FALSE }};
  struct server_t test_servers[] = {
    { "irc.test.org", 6669, TRUE }};
  gboolean network_checked[3];

  mgr = empathy_irc_network_manager_new (GLOBAL_SAMPLE, NULL);

  g_object_get (mgr,
      "global-file", &global_file,
      "user-file", &user_file,
      NULL);
  fail_if (global_file == NULL || strcmp (global_file, GLOBAL_SAMPLE) != 0);
  fail_if (user_file != NULL);
  g_free (global_file);
  g_free (user_file);

  networks = empathy_irc_network_manager_get_networks (mgr);
  fail_if (g_slist_length (networks) != 3);

  network_checked[0] = network_checked[1] = network_checked[2] = FALSE;
  /* check networks and servers */
  for (l = networks; l != NULL; l = g_slist_next (l))
    {
      gchar *name;

      g_object_get (l->data, "name", &name, NULL);
      fail_if (name == NULL);

      if (strcmp (name, "Freenode") == 0)
        {
          check_network (l->data, "Freenode", freenode_servers, 2);
          network_checked[0] = TRUE;
        }
      else if (strcmp (name, "GIMPNet") == 0)
        {
          check_network (l->data, "GIMPNet", gimpnet_servers, 2);
          network_checked[1] = TRUE;
        }
      else if (strcmp (name, "Test Server") == 0)
        {
          check_network (l->data, "Test Server", test_servers, 1);
          network_checked[2] = TRUE;
        }
      else
        {
          fail_if (TRUE);
        }
    }
  fail_if (!network_checked[0] || !network_checked[1] || !network_checked[2]);

  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
  g_slist_free (networks);
  g_object_unref (mgr);
}
END_TEST

TCase *
make_empathy_irc_network_manager_tcase (void)
{
    TCase *tc = tcase_create ("empathy-irc-network-manager");
    tcase_add_test (tc, test_load_global_file);
    return tc;
}
