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
  fail_if (network == NULL);

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
  fail_if (network == NULL);

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

static gboolean modified;

static void
modified_cb (EmpathyIrcNetwork *network,
             gpointer unused)
{
  modified = TRUE;
}

START_TEST (test_modified_signal)
{
  EmpathyIrcNetwork *network;

  network = empathy_irc_network_new ("id1", "Network1");
  fail_if (network == NULL);

  modified = FALSE;
  g_signal_connect (network, "modified", G_CALLBACK (modified_cb), NULL);

  g_object_set (network, "name", "Network2", NULL);
  fail_if (!modified);
  modified = FALSE;
  g_object_set (network, "name", "Network2", NULL);
  fail_if (modified);

  g_object_unref (network);
}
END_TEST

struct server_t
{
  gchar *address;
  guint port;
  gboolean ssl;
};

START_TEST (test_add_server)
{
  EmpathyIrcNetwork *network;
  EmpathyIrcServer *server;
  GSList *servers, *l;
  guint i;
  struct server_t test_servers[] = {
    { "server1", 6667, FALSE },
    { "server2", 6668, TRUE },
    { "server3", 6667, FALSE },
    { "server4", 6669, TRUE}};

  network = empathy_irc_network_new ("id1", "Network1");
  fail_if (network == NULL);

  modified = FALSE;
  g_signal_connect (network, "modified", G_CALLBACK (modified_cb), NULL);

  servers = empathy_irc_network_get_servers (network);
  fail_if (servers != NULL);

  /* add the servers */
  for (i = 0; i < 4; i ++)
    {
      server = empathy_irc_server_new (test_servers[i].address,
          test_servers[i].port, test_servers[i].ssl);
      empathy_irc_network_add_server (network, server);
      g_object_unref (server);
    }

  /* get servers list */
  servers = empathy_irc_network_get_servers (network);
  fail_if (g_slist_length (servers) != 4);

  /* Is that the right servers ? */
  for (l = servers, i = 0; l != NULL; l = g_slist_next (l), i++)
    {
      gchar *address;
      guint port;
      gboolean ssl;

      server = l->data;

      g_object_get (server,
          "address", &address,
          "port", &port,
          "ssl", &ssl,
          NULL);

      fail_if (address == NULL || strcmp (address, test_servers[i].address)
          != 0);
      fail_if (port != test_servers[i].port);
      fail_if (ssl != test_servers[i].ssl);

      g_free (address);
    }

  /* Now let's remove the 3rd server */
  l = g_slist_nth (servers, 2);
  fail_if (l == NULL);
  server = l->data;
  empathy_irc_network_remove_server (network, server);

  /* free the list */
  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);

  /* Request the new servers list */
  servers = empathy_irc_network_get_servers (network);
  fail_if (g_slist_length (servers) != 3);

  /* The 3rd server should have disappear */
  for (l = servers, i = 0; l != NULL; l = g_slist_next (l), i++)
    {
      gchar *address;
      guint port;
      gboolean ssl;

      server = l->data;

      g_object_get (server,
          "address", &address,
          "port", &port,
          "ssl", &ssl,
          NULL);

      /* 3rd server was removed */
      if (i == 2)
        i++;

      fail_if (address == NULL || strcmp (address, test_servers[i].address)
          != 0);
      fail_if (port != test_servers[i].port);
      fail_if (ssl != test_servers[i].ssl);

      g_free (address);
    }

  /* free the list */
  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);

  g_object_unref (network);
}
END_TEST

TCase *
make_empathy_irc_network_tcase (void)
{
    TCase *tc = tcase_create ("empathy-irc-network");
    tcase_add_test (tc, test_empathy_irc_network_new);
    tcase_add_test (tc, test_property_change);
    tcase_add_test (tc, test_modified_signal);
    tcase_add_test (tc, test_add_server);
    return tc;
}
