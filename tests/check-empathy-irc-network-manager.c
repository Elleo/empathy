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
  EmpathyIrcNetwork *network;
  struct server_t freenode_servers[] = {
    { "irc.freenode.net", 6667, FALSE },
    { "irc.eu.freenode.net", 6667, FALSE }};
  struct server_t gimpnet_servers[] = {
    { "irc.gimp.org", 6667, FALSE },
    { "irc.us.gimp.org", 6667, FALSE }};
  struct server_t test_servers[] = {
    { "irc.test.org", 6669, TRUE }};

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

  /* check networks and servers */
  l = networks;

  network = l->data;
  check_network (network, "freenode", "Freenode", freenode_servers, 2);
  l = g_slist_next (l);
  network = l->data;
  check_network (network, "gimpnet", "GIMPNet", gimpnet_servers, 2);
  l = g_slist_next (l);
  network = l->data;
  check_network (network, "testsrv", "Test Server", test_servers, 1);

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
