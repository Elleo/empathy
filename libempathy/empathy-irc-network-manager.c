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

#define IRC_NETWORKS_XML_FILENAME "irc-networks.xml"

#define DEBUG_DOMAIN "IrcNetworkManager"

G_DEFINE_TYPE (EmpathyIrcNetworkManager, empathy_irc_network_manager,
    G_TYPE_OBJECT);

/* properties */
enum
{
  PROP_FILENAME = 1,
  LAST_PROPERTY
};

typedef struct _EmpathyIrcNetworkManagerPrivate
    EmpathyIrcNetworkManagerPrivate;

struct _EmpathyIrcNetworkManagerPrivate {
  GSList *irc_networks;

  gchar *filename;
};

#define EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE(obj)\
    ((EmpathyIrcNetworkManagerPrivate *) obj->priv)

static gboolean
irc_network_manager_get_all (EmpathyIrcNetworkManager *manager);
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
      case PROP_FILENAME:
        g_value_set_string (value, priv->filename);
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
      case PROP_FILENAME:
        g_free (priv->filename);
        priv->filename = g_value_dup_string (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
empathy_irc_network_manager_finalize (GObject *object)
{
  EmpathyIrcNetworkManager *self = EMPATHY_IRC_NETWORK_MANAGER (object);
  EmpathyIrcNetworkManagerPrivate *priv = 
    EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  g_free (priv->filename);

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

  object_class->get_property = empathy_irc_network_manager_get_property;
  object_class->set_property = empathy_irc_network_manager_set_property;

  g_type_class_add_private (object_class,
          sizeof (EmpathyIrcNetworkManagerPrivate));

  object_class->finalize = empathy_irc_network_manager_finalize;

  param_spec = g_param_spec_string (
      "filename",
      "Filename path",
      "The path of the filename from which we have to load"
      "the networks list",
      "",
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_FILENAME, param_spec);
}

EmpathyIrcNetworkManager *
empathy_irc_network_manager_new (void)
{

  EmpathyIrcNetworkManager *manager;

  manager = g_object_new (EMPATHY_TYPE_IRC_NETWORK_MANAGER,
      "filename", "/home/cassidy/.gnome2/Empathy/irc-networks.xml",
      NULL);

  /* load file */
  // XXX move that in the constructor
  irc_network_manager_get_all (manager);

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
empathy_irc_network_manager_get_irc_networks (EmpathyIrcNetworkManager *self)
{
  EmpathyIrcNetworkManagerPrivate *priv;
  GSList *irc_networks;

  g_return_val_if_fail (EMPATHY_IS_IRC_NETWORK_MANAGER (self), NULL);

  priv = EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  irc_networks = g_slist_copy (priv->irc_networks);
  g_slist_foreach (irc_networks, (GFunc)g_object_ref, NULL);

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

static gboolean
irc_network_manager_get_all (EmpathyIrcNetworkManager *self)
{

  EmpathyIrcNetworkManagerPrivate *priv;
  gchar *dir;
  gchar *file_with_path = NULL;

  priv = EMPATHY_IRC_NETWORK_MANAGER_GET_PRIVATE (self);

  if (!priv->filename)
    {
      dir = g_build_filename (g_get_home_dir (), ".gnome2", PACKAGE_NAME,
          NULL);

      if (!g_file_test (dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
        {
          g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
        }

      file_with_path = g_build_filename (dir, IRC_NETWORKS_XML_FILENAME, NULL);
      g_free (dir);
    }
  else
    {
      file_with_path = g_strdup (priv->filename);
    }

  /* read file in */
  if (g_file_test (file_with_path, G_FILE_TEST_EXISTS) &&
      !irc_network_manager_file_parse (self, file_with_path))
    {
      g_free (file_with_path);
      return FALSE;
    }

  g_free (file_with_path);

  return TRUE;
}

static void
irc_network_manager_parse_irc_server (EmpathyIrcNetwork *network,
                                      xmlNodePtr node)
{
  xmlNodePtr server_node;
  gchar *str;

  for (server_node = node->children; server_node;
      server_node = server_node->next)
    {
      xmlNodePtr  child;
      gchar *address = NULL, *port = NULL, *ssl = NULL;

      if (strcmp (server_node->name, "server") != 0)
        continue;

      for (child = server_node->children; child; child = child->next)
        {
          gchar *tag;
          tag = (gchar *) child->name;
          str = (gchar *) xmlNodeGetContent (child);

          if (!str)
            continue;

          if (strcmp (tag, "address") == 0)
            {
              g_print ("server adr: %s\n", str);
              address = str;
            }

          else if (strcmp (tag, "port") == 0)
            {
              g_print ("server port: %s\n", str);
              port = str;
            }

          else if (strcmp (tag, "ssl") == 0)
            {
                g_print ("server ssl: %s\n", str);
                ssl = str;
            }
        }

      if (address != NULL && port != NULL && ssl != NULL)
        {
          gint port_nb;
          gboolean have_ssl = FALSE;
          EmpathyIrcServer *server;

          port_nb = strtol (str, NULL, 10);
          if (port_nb <= 0 || port_nb > 65556)
            port_nb = 6667;

          if (strcmp (ssl, "TRUE") == 0)
            have_ssl = TRUE;

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
                                       xmlNodePtr            node)
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
  g_print ("id: %s name %s\n", id, name);
  network = empathy_irc_network_new (id, name);
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
          g_print ("servers\n");
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

  empathy_debug (DEBUG_DOMAIN,
          "Attempting to parse file:'%s'...",
          filename);

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
