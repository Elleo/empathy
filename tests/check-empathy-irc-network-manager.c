#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include "check-helpers.h"
#include "check-libempathy.h"
#include "check-irc-helper.h"

#include <libempathy/empathy-irc-network-manager.h>

#define GLOBAL_SAMPLE "xml/default-irc-networks-sample.xml"
#define USER_SAMPLE "xml/user-irc-networks-sample.xml"

START_TEST (test_empathy_irc_network_manager_add)
{
  EmpathyIrcNetworkManager *mgr;
  EmpathyIrcNetwork *network;
  GSList *networks;
  gchar *name;

  mgr = empathy_irc_network_manager_new (NULL, NULL);
  fail_if (mgr == NULL);

  networks = empathy_irc_network_manager_get_networks (mgr);
  fail_if (networks != NULL);

  /* add a network */
  network = empathy_irc_network_new ("My Network");
  fail_if (network == NULL);
  empathy_irc_network_manager_add (mgr, network);
  g_object_unref (network);

  networks = empathy_irc_network_manager_get_networks (mgr);
  fail_if (g_slist_length (networks) != 1);
  g_object_get (networks->data, "name", &name, NULL);
  fail_if (name == NULL || strcmp (name, "My Network") != 0);
  g_free (name);
  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
  g_slist_free (networks);

  /* add another network having the same name */
  network = empathy_irc_network_new ("My Network");
  fail_if (network == NULL);
  empathy_irc_network_manager_add (mgr, network);
  g_object_unref (network);

  networks = empathy_irc_network_manager_get_networks (mgr);
  fail_if (g_slist_length (networks) != 2);
  g_object_get (networks->data, "name", &name, NULL);
  fail_if (name == NULL || strcmp (name, "My Network") != 0);
  g_free (name);
  g_object_get (g_slist_next(networks)->data, "name", &name, NULL);
  fail_if (name == NULL || strcmp (name, "My Network") != 0);
  g_free (name);
  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
  g_slist_free (networks);

  g_object_unref (mgr);
}
END_TEST

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

      g_free (name);
    }
  fail_if (!network_checked[0] || !network_checked[1] || !network_checked[2]);

  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
  g_slist_free (networks);
  g_object_unref (mgr);
}
END_TEST

static gboolean
remove_network_named (EmpathyIrcNetworkManager *mgr,
                      const gchar *network_name)
{
  GSList *networks, *l;
  gboolean removed = FALSE;

  networks = empathy_irc_network_manager_get_networks (mgr);

  /* check networks and servers */
  for (l = networks; l != NULL && !removed; l = g_slist_next (l))
    {
      EmpathyIrcNetwork *network = l->data;
      gchar *name;

      g_object_get (network, "name", &name, NULL);
      fail_if (name == NULL);

      if (strcmp (name, network_name) == 0)
        {
          empathy_irc_network_manager_remove (mgr, network);
          removed = TRUE;
        }

      g_free (name);
    }

  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
  g_slist_free (networks);

  return removed;
}

START_TEST (test_empathy_irc_network_manager_remove)
{
  EmpathyIrcNetworkManager *mgr;
  GSList *networks, *l;
  struct server_t freenode_servers[] = {
    { "irc.freenode.net", 6667, FALSE },
    { "irc.eu.freenode.net", 6667, FALSE }};
  struct server_t test_servers[] = {
    { "irc.test.org", 6669, TRUE }};
  gboolean network_checked[2];
  gboolean result;

  mgr = empathy_irc_network_manager_new (GLOBAL_SAMPLE, NULL);

  result = remove_network_named (mgr, "GIMPNet");
  fail_if (!result);

  networks = empathy_irc_network_manager_get_networks (mgr);
  fail_if (g_slist_length (networks) != 2);

  network_checked[0] = network_checked[1] = FALSE;
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
      else if (strcmp (name, "Test Server") == 0)
        {
          check_network (l->data, "Test Server", test_servers, 1);
          network_checked[1] = TRUE;
        }
      else
        {
          fail_if (TRUE);
        }

      g_free (name);
    }
  fail_if (!network_checked[0] || !network_checked[1]);

  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
  g_slist_free (networks);
  g_object_unref (mgr);
}
END_TEST

START_TEST (test_load_user_file)
{
  EmpathyIrcNetworkManager *mgr;
  gchar *global_file, *user_file;
  GSList *networks, *l;
  struct server_t gimpnet_servers[] = {
    { "irc.gimp.org", 6667, FALSE },
    { "irc.us.gimp.org", 6667, FALSE },
    { "irc.au.gimp.org", 6667, FALSE }};
  struct server_t my_server[] = {
    { "irc.mysrv.net", 7495, TRUE }};
  gboolean network_checked[2];

  mgr = empathy_irc_network_manager_new (NULL, USER_SAMPLE);

  g_object_get (mgr,
      "global-file", &global_file,
      "user-file", &user_file,
      NULL);
  fail_if (global_file != NULL);
  fail_if (user_file == NULL || strcmp (user_file, USER_SAMPLE) != 0);
  g_free (global_file);
  g_free (user_file);

  networks = empathy_irc_network_manager_get_networks (mgr);
  fail_if (g_slist_length (networks) != 2);

  network_checked[0] = network_checked[1] = FALSE;
  /* check networks and servers */
  for (l = networks; l != NULL; l = g_slist_next (l))
    {
      gchar *name;

      g_object_get (l->data, "name", &name, NULL);
      fail_if (name == NULL);

      if (strcmp (name, "GIMPNet") == 0)
        {
          check_network (l->data, "GIMPNet", gimpnet_servers, 3);
          network_checked[0] = TRUE;
        }
      else if (strcmp (name, "My Server") == 0)
        {
          check_network (l->data, "My Server", my_server, 1);
          network_checked[1] = TRUE;
        }
      else
        {
          fail_if (TRUE);
        }

      g_free (name);
    }
  fail_if (!network_checked[0] || !network_checked[1]);

  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
  g_slist_free (networks);
  g_object_unref (mgr);
}
END_TEST

START_TEST (test_load_both_files)
{
  EmpathyIrcNetworkManager *mgr;
  gchar *global_file, *user_file;
  GSList *networks, *l;
  struct server_t freenode_servers[] = {
    { "irc.freenode.net", 6667, FALSE },
    { "irc.eu.freenode.net", 6667, FALSE }};
  struct server_t gimpnet_servers[] = {
    { "irc.gimp.org", 6667, FALSE },
    { "irc.us.gimp.org", 6667, FALSE },
    { "irc.au.gimp.org", 6667, FALSE }};
  struct server_t my_server[] = {
    { "irc.mysrv.net", 7495, TRUE }};
  gboolean network_checked[3];

  mgr = empathy_irc_network_manager_new (GLOBAL_SAMPLE, USER_SAMPLE);

  g_object_get (mgr,
      "global-file", &global_file,
      "user-file", &user_file,
      NULL);
  fail_if (global_file == NULL || strcmp (global_file, GLOBAL_SAMPLE) != 0);
  fail_if (user_file == NULL || strcmp (user_file, USER_SAMPLE) != 0);
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
          check_network (l->data, "GIMPNet", gimpnet_servers, 3);
          network_checked[1] = TRUE;
        }
      else if (strcmp (name, "My Server") == 0)
        {
          check_network (l->data, "My Server", my_server, 1);
          network_checked[2] = TRUE;
        }
      else
        {
          fail_if (TRUE);
        }

      g_free (name);
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
    tcase_add_test (tc, test_empathy_irc_network_manager_add);
    tcase_add_test (tc, test_load_global_file);
    tcase_add_test (tc, test_empathy_irc_network_manager_remove);
    tcase_add_test (tc, test_load_user_file);
    tcase_add_test (tc, test_load_both_files);
    return tc;
}
