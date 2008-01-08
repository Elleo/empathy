/*
 * Copyright (C) 2004-2007 Guillaume Desmottes
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libempathy/empathy-debug.h>

#include "empathy-irc-network-manager.h"

#define DEBUG_DOMAIN "IrcNetworkManager"

G_DEFINE_TYPE (EmpathyIrcNetworkManager, empathy_irc_network_manager,
    G_TYPE_OBJECT);

/* properties */
enum
{
  PROP_GLOBAL_FILE = 1,
  PROP_USER_FILE,
  LAST_PROPERTY
};

typedef struct _EmpathyIrcNetworkManagerPrivate
    EmpathyIrcNetworkManagerPrivate;

struct _EmpathyIrcNetworkManagerPrivate {
  GSList *irc_networks;

  gchar *global_file;
  gchar *user_file;
  gboolean modified;
};

#define EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE(obj)\
    ((EmpathyIrcNetworkManagerPrivate *) obj->priv)

static void
irc_network_manager_load_servers (EmpathyIrcNetworkManager *manager);
static gboolean
irc_network_manager_file_parse (EmpathyIrcNetworkManager *manager,
    const gchar *filename);
static gboolean
irc_network_manager_file_save (EmpathyIrcNetworkManager *manager);

static void
empathy_irc_network_manager_get_property (GObject *object,
                                          guint property_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
  EmpathyIrcNetworkManager *self = EMPATHY_IRC_NETWORK_MANAGER (object);
  EmpathyIrcNetworkManagerPrivate *priv =
    EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_GLOBAL_FILE:
        g_value_set_string (value, priv->global_file);
        break;
      case PROP_USER_FILE:
        g_value_set_string (value, priv->user_file);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
empathy_irc_network_manager_set_property (GObject *object,
                                          guint property_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
  EmpathyIrcNetworkManager *self = EMPATHY_IRC_NETWORK_MANAGER (object);
  EmpathyIrcNetworkManagerPrivate *priv =
    EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_GLOBAL_FILE:
        g_free (priv->global_file);
        priv->global_file = g_value_dup_string (value);
        break;
      case PROP_USER_FILE:
        g_free (priv->user_file);
        priv->user_file = g_value_dup_string (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static GObject *
empathy_irc_network_manager_constructor (GType type,
                                         guint n_props,
                                         GObjectConstructParam *props)
{
  GObject *obj;
  EmpathyIrcNetworkManager *self;

  /* Parent constructor chain */
  obj = G_OBJECT_CLASS (empathy_irc_network_manager_parent_class)->
        constructor (type, n_props, props);

  self = EMPATHY_IRC_NETWORK_MANAGER (obj);
  irc_network_manager_load_servers (self);

  return obj;
}

static void
empathy_irc_network_manager_finalize (GObject *object)
{
  EmpathyIrcNetworkManager *self = EMPATHY_IRC_NETWORK_MANAGER (object);
  EmpathyIrcNetworkManagerPrivate *priv = 
    EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  g_free (priv->global_file);
  g_free (priv->user_file);

  g_slist_foreach (priv->irc_networks, (GFunc) g_object_unref, NULL);
  g_slist_free (priv->irc_networks);

  G_OBJECT_CLASS (empathy_irc_network_manager_parent_class)->finalize (object);
}

static void
empathy_irc_network_manager_init (EmpathyIrcNetworkManager *self)
{
  EmpathyIrcNetworkManagerPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_IRC_NETWORK_MANAGER, EmpathyIrcNetworkManagerPrivate);

  self->priv = priv;
}

static void
empathy_irc_network_manager_class_init (EmpathyIrcNetworkManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  object_class->constructor = empathy_irc_network_manager_constructor;
  object_class->get_property = empathy_irc_network_manager_get_property;
  object_class->set_property = empathy_irc_network_manager_set_property;

  g_type_class_add_private (object_class,
          sizeof (EmpathyIrcNetworkManagerPrivate));

  object_class->finalize = empathy_irc_network_manager_finalize;

  param_spec = g_param_spec_string (
      "global-file",
      "path of the global networks file",
      "The path of the system-wide filename from which we have to load"
      "the networks list",
      NULL,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_GLOBAL_FILE, param_spec);

  param_spec = g_param_spec_string (
      "user-file",
      "path of the user networks file",
      "The path of user's  filename from which we have to load"
      "the networks list and to which we'll save his modifications",
      NULL,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_USER_FILE, param_spec);
}

EmpathyIrcNetworkManager *
empathy_irc_network_manager_new (const gchar *global_file,
                                 const gchar *user_file)
{
  EmpathyIrcNetworkManager *manager;

  manager = g_object_new (EMPATHY_TYPE_IRC_NETWORK_MANAGER,
      "global-file", global_file,
      "user-file", user_file,
      NULL);

  return manager;
}

gboolean
empathy_irc_network_manager_add (EmpathyIrcNetworkManager *self,
                                 EmpathyIrcNetwork *irc_network)
{
  EmpathyIrcNetworkManagerPrivate *priv;

  g_return_val_if_fail (EMPATHY_IS_IRC_NETWORK_MANAGER (self), FALSE);
  g_return_val_if_fail (EMPATHY_IS_IRC_NETWORK (irc_network), FALSE);

  priv = EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  /* Don't add more than once */
  /*
  if (!empathy_irc_network_manager_find (manager, empathy_irc_network_get_name (irc_network))) {
    const gchar       *name;

    name = empathy_irc_network_get_name (irc_network);

    empathy_debug (DEBUG_DOMAIN, "Adding %s irc_network with name:'%s'",
            empathy_irc_network_type_to_string (type),
            name);

    priv->irc_networks = g_list_append (priv->irc_networks, g_object_ref (irc_network));

    g_signal_emit (manager, signals[IRC_NETWORK_ADDED], 0, irc_network);

    return TRUE;
  }
  */
  priv->irc_networks = g_slist_append (priv->irc_networks,
      g_object_ref (irc_network));

  return FALSE;
}

void
empathy_irc_network_manager_remove (EmpathyIrcNetworkManager *self,
                                    EmpathyIrcNetwork *irc_network)
{
  EmpathyIrcNetworkManagerPrivate *priv;

  g_return_if_fail (EMPATHY_IS_IRC_NETWORK_MANAGER (self));
  g_return_if_fail (EMPATHY_IS_IRC_NETWORK (irc_network));

  priv = EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  /*
  empathy_debug (DEBUG_DOMAIN,
          "Removing irc_network with name:'%s'",
          empathy_irc_network_get_name (irc_network));

  priv->irc_networks = g_slist_remove (priv->irc_networks, irc_network);

  g_object_unref (irc_network);
  */
}

GSList *
empathy_irc_network_manager_get_networks (EmpathyIrcNetworkManager *self)
{
  EmpathyIrcNetworkManagerPrivate *priv;
  GSList *irc_networks;

  g_return_val_if_fail (EMPATHY_IS_IRC_NETWORK_MANAGER (self), NULL);

  priv = EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  irc_networks = g_slist_copy (priv->irc_networks);
  g_slist_foreach (irc_networks, (GFunc) g_object_ref, NULL);

  return irc_networks;
}

gboolean
empathy_irc_network_manager_store (EmpathyIrcNetworkManager *self)
{
  g_return_val_if_fail (EMPATHY_IS_IRC_NETWORK_MANAGER (self), FALSE);

  empathy_debug (DEBUG_DOMAIN, "Saving IRC networks");

  return irc_network_manager_file_save (self);
}

/*
 * API to save/load and parse the irc_networks file.
 */

static void
load_global_file (EmpathyIrcNetworkManager *self)
{
  EmpathyIrcNetworkManagerPrivate *priv =
    EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  if (priv->global_file == NULL)
    return;

  if (!g_file_test (priv->global_file, G_FILE_TEST_EXISTS))
    {
      empathy_debug (DEBUG_DOMAIN, "Global networks file %s doesn't exist",
          priv->global_file);
      return;
    }

  irc_network_manager_file_parse (self, priv->global_file);
}

static void
irc_network_manager_load_servers (EmpathyIrcNetworkManager *self)
{
  EmpathyIrcNetworkManagerPrivate *priv =
    EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  /* TODO: load user file */
  load_global_file (self);

  priv->modified = FALSE;
}

static void
irc_network_manager_parse_irc_server (EmpathyIrcNetwork *network,
                                      xmlNodePtr node)
{
  xmlNodePtr server_node;

  for (server_node = node->children; server_node;
      server_node = server_node->next)
    {
      gchar *address = NULL, *port = NULL, *ssl = NULL;

      if (strcmp (server_node->name, "server") != 0)
        continue;

      address = xmlGetProp (server_node, "address");
      port = xmlGetProp (server_node, "port");
      ssl = xmlGetProp (server_node, "ssl");

      if (address != NULL)
        {
          gint port_nb = 0;
          gboolean have_ssl = FALSE;
          EmpathyIrcServer *server;

          if (port != NULL)
            port_nb = strtol (port, NULL, 10);

          if (port_nb <= 0 || port_nb > G_MAXUINT16)
            port_nb = 6667;

          if (ssl == NULL || strcmp (ssl, "TRUE") == 0)
            have_ssl = TRUE;

          empathy_debug (DEBUG_DOMAIN, "add server %s port %d ssl %d", address,
              port_nb, have_ssl);

          server = empathy_irc_server_new (address, port_nb, have_ssl);
          empathy_irc_network_add_server (network, server);
        }

      if (address)
        xmlFree (address);
      if (port)
        xmlFree (port);
      if (ssl)
        xmlFree (ssl);
    }
}

static void
irc_network_manager_parse_irc_network (EmpathyIrcNetworkManager *self,
                                       xmlNodePtr node)
{
  EmpathyIrcNetwork  *network;
  xmlNodePtr child;
  gchar *str;
  gchar *id, *name;

  if (!xmlHasProp (node, "id"))
    return;

  if (!xmlHasProp (node, "name"))
    return;

  id = xmlGetProp (node, "id");
  name = xmlGetProp (node, "name");
  network = empathy_irc_network_new (id, name);
  empathy_debug (DEBUG_DOMAIN, "add network %s (id %s)", name, id);
  xmlFree (name);
  xmlFree (id);

  for (child = node->children; child; child = child->next)
    {
      gchar *tag;

      tag = (gchar *) child->name;
      str = (gchar *) xmlNodeGetContent (child);

      if (!str)
        continue;

      if (strcmp (tag, "servers") == 0)
        {
          irc_network_manager_parse_irc_server (network, child);
        }

      xmlFree (str);
    }

  empathy_irc_network_manager_add (self, network);

  g_object_unref (network);
}

static gboolean
irc_network_manager_file_parse (EmpathyIrcNetworkManager *self,
                                const gchar *filename)
{
  EmpathyIrcNetworkManagerPrivate *priv;
  xmlParserCtxtPtr ctxt;
  xmlDocPtr doc;
  xmlNodePtr networks;
  xmlNodePtr node;

  priv = EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  empathy_debug (DEBUG_DOMAIN, "Attempting to parse file:'%s'...", filename);

  ctxt = xmlNewParserCtxt ();

  /* Parse and validate the file. */
  doc = xmlCtxtReadFile (ctxt, filename, NULL, 0);
  if (!doc)
    {
      g_warning ("Failed to parse file:'%s'", filename);
      xmlFreeParserCtxt (ctxt);
      return FALSE;
    }

  /*
  if (!empathy_xml_validate (doc, IRC_NETWORKS_DTD_FILENAME)) {
    g_warning ("Failed to validate file:'%s'", filename);
    xmlFreeDoc (doc);
    xmlFreeParserCtxt (ctxt);
    return FALSE;
  }
  */

  /* The root node, networks. */
  networks = xmlDocGetRootElement (doc);

  for (node = networks->children; node; node = node->next)
    {
      irc_network_manager_parse_irc_network (self, node);
    }

  /*
  empathy_debug (DEBUG_DOMAIN,
          "Parsed %d irc_networks",
          g_list_length (priv->irc_networks));

  */

  xmlFreeDoc(doc);
  xmlFreeParserCtxt (ctxt);

  return TRUE;
}

static gboolean
irc_network_manager_file_save (EmpathyIrcNetworkManager *self)
{
  //TODO
  return TRUE;
}
