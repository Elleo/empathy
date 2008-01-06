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

struct server_t test_servers[] = {
  { "server1", 6667, FALSE },
  { "server2", 6668, TRUE },
  { "server3", 6667, FALSE },
  { "server4", 6669, TRUE }};

static void
add_test_servers (EmpathyIrcNetwork *network)
{
  guint i;

  for (i = 0; i < 4; i ++)
    {
      EmpathyIrcServer *server;

      server = empathy_irc_server_new (test_servers[i].address,
          test_servers[i].port, test_servers[i].ssl);
      modified = FALSE;
      empathy_irc_network_add_server (network, server);
      fail_if (!modified);
      g_object_unref (server);
    }
}

START_TEST (test_add_server)
{
  EmpathyIrcNetwork *network;
  EmpathyIrcServer *server;
  GSList *servers, *l;
  guint i;

  network = empathy_irc_network_new ("id1", "Network1");
  fail_if (network == NULL);

  modified = FALSE;
  g_signal_connect (network, "modified", G_CALLBACK (modified_cb), NULL);

  servers = empathy_irc_network_get_servers (network);
  fail_if (servers != NULL);

  /* add the servers */
  add_test_servers (network);

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
  modified = FALSE;
  empathy_irc_network_remove_server (network, server);
  fail_if (!modified);

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

START_TEST (test_modified_signal_because_of_server)
{
  EmpathyIrcNetwork *network;
  EmpathyIrcServer *server;

  network = empathy_irc_network_new ("id1", "Network1");
  fail_if (network == NULL);

  g_signal_connect (network, "modified", G_CALLBACK (modified_cb), NULL);

  server = empathy_irc_server_new ("server1", 6667, FALSE);
  empathy_irc_network_add_server (network, server);

  /* Change server properties */
  modified = FALSE;
  g_object_set (server, "address", "server2", NULL);
  fail_if (!modified);
  modified = FALSE;
  g_object_set (server, "port", 6668, NULL);
  fail_if (!modified);
  modified = FALSE;
  g_object_set (server, "ssl", TRUE, NULL);
  fail_if (!modified);

  empathy_irc_network_remove_server (network, server);
  modified = FALSE;
  g_object_set (server, "address", "server3", NULL);
  /* We removed the server so the network is not modified anymore */
  fail_if (modified);

  g_object_unref (network);
}
END_TEST

START_TEST (test_empathy_irc_network_set_server_position)
{
  EmpathyIrcNetwork *network;
  GSList *servers, *l;
  gchar *address;

  network = empathy_irc_network_new ("id1", "Network1");
  fail_if (network == NULL);

  modified = FALSE;
  g_signal_connect (network, "modified", G_CALLBACK (modified_cb), NULL);

  /* add the servers */
  add_test_servers (network);

  /* get servers list */
  servers = empathy_irc_network_get_servers (network);
  fail_if (g_slist_length (servers) != 4);
  modified = FALSE;

  /* server1 go to the last position */
  empathy_irc_network_set_server_position (network, servers->data, -1);

  /* server2 go to the first position */
  l = servers->next;
  empathy_irc_network_set_server_position (network, l->data, 0);

  /* server3 go to the third position */
  l = l->next;
  empathy_irc_network_set_server_position (network, l->data, 2);

  /* server4 go to the second position*/
  l = l->next;
  empathy_irc_network_set_server_position (network, l->data, 1);

  fail_if (!modified);

  /* free the list */
  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);

  /* get servers list */
  servers = empathy_irc_network_get_servers (network);
  fail_if (g_slist_length (servers) != 4);

  /* check the new servers list */
  l = servers;
  g_object_get (l->data, "address", &address, NULL);
  fail_if (address == NULL || strcmp (address, "server2") != 0);
  g_free (address);

  l = g_slist_next (l);
  g_object_get (l->data, "address", &address, NULL);
  fail_if (address == NULL || strcmp (address, "server4") != 0);
  g_free (address);

  l = g_slist_next (l);
  g_object_get (l->data, "address", &address, NULL);
  fail_if (address == NULL || strcmp (address, "server3") != 0);
  g_free (address);

  l = g_slist_next (l);
  g_object_get (l->data, "address", &address, NULL);
  fail_if (address == NULL || strcmp (address, "server1") != 0);
  g_free (address);

  /* free the list */
  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);
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
    tcase_add_test (tc, test_modified_signal_because_of_server);
    tcase_add_test (tc, test_empathy_irc_network_set_server_position);
    return tc;
}
