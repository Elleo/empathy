/*
 * Copyright (C) 2007 Guillaume Desmottes
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
 *
 * Authors: Guillaume Desmottes <gdesmott@gnome.org>
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>

#include <telepathy-glib/util.h>

#include "empathy-marshal.h"
#include "empathy-irc-network.h"

G_DEFINE_TYPE (EmpathyIrcNetwork, empathy_irc_network, G_TYPE_OBJECT);

/* properties */
enum
{
  PROP_ID = 1,
  PROP_NAME,
  LAST_PROPERTY
};

/* signals */
enum
{
  MODIFIED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

typedef struct _EmpathyIrcNetworkPrivate EmpathyIrcNetworkPrivate;

struct _EmpathyIrcNetworkPrivate
{
  gchar *id;
  gchar *name;
  GSList *servers;
};

#define EMPATHY_IRC_NETWORK_GET_PRIVATE(obj)\
    ((EmpathyIrcNetworkPrivate *) obj->priv)

static void
empathy_irc_network_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  EmpathyIrcNetwork *self = EMPATHY_IRC_NETWORK (object);
  EmpathyIrcNetworkPrivate *priv = EMPATHY_IRC_NETWORK_GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_ID:
        g_value_set_string (value, priv->id);
        break;
      case PROP_NAME:
        g_value_set_string (value, priv->name);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
empathy_irc_network_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  EmpathyIrcNetwork *self = EMPATHY_IRC_NETWORK (object);
  EmpathyIrcNetworkPrivate *priv = EMPATHY_IRC_NETWORK_GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_ID:
        g_free (priv->id);
        priv->id = g_value_dup_string (value);
        break;
      case PROP_NAME:
        if (tp_strdiff (priv->name, g_value_get_string (value)))
          {
            g_free (priv->name);
            priv->name = g_value_dup_string (value);
            g_signal_emit (object, signals[MODIFIED], 0);
          }
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
empathy_irc_network_finalize (GObject *object)
{
  EmpathyIrcNetwork *self = EMPATHY_IRC_NETWORK (object);
  EmpathyIrcNetworkPrivate *priv = EMPATHY_IRC_NETWORK_GET_PRIVATE (self);

  g_free (priv->id);
  g_free (priv->name);

  G_OBJECT_CLASS (empathy_irc_network_parent_class)->finalize (object);
}

static void
empathy_irc_network_init (EmpathyIrcNetwork *self)
{
  EmpathyIrcNetworkPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_IRC_NETWORK, EmpathyIrcNetworkPrivate);

  self->priv = priv;

  priv->servers = NULL;
}

static void
empathy_irc_network_class_init (EmpathyIrcNetworkClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  object_class->get_property = empathy_irc_network_get_property;
  object_class->set_property = empathy_irc_network_set_property;

  g_type_class_add_private (object_class,
          sizeof (EmpathyIrcNetworkPrivate));

  object_class->finalize = empathy_irc_network_finalize;

  param_spec = g_param_spec_string (
      "id",
      "Identifier",
      "The unique identifier of this network",
      NULL,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_ID, param_spec);

  param_spec = g_param_spec_string (
      "name",
      "Network name",
      "The displayed name of this network",
      NULL,
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_NAME, param_spec);

  signals[MODIFIED] = g_signal_new (
      "modified",
      G_OBJECT_CLASS_TYPE (object_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
      0,
      NULL, NULL,
      empathy_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

EmpathyIrcNetwork *
empathy_irc_network_new (const gchar *id,
                         const gchar *name)
{
  return g_object_new (EMPATHY_TYPE_IRC_NETWORK,
      "id", id,
      "name", name,
      NULL);
}

const GSList *
empathy_irc_network_get_servers (EmpathyIrcNetwork *self)
{
  EmpathyIrcNetworkPrivate *priv;

  g_return_val_if_fail (EMPATHY_IS_IRC_NETWORK (self), NULL);
  priv = EMPATHY_IRC_NETWORK_GET_PRIVATE (self);

  return priv->servers;
}

void
empathy_irc_network_add_server (EmpathyIrcNetwork *self,
                                EmpathyIrcServer *server)
{
  EmpathyIrcNetworkPrivate *priv;

  g_return_if_fail (EMPATHY_IS_IRC_NETWORK (self));
  g_return_if_fail (server != NULL && EMPATHY_IS_IRC_SERVER (server));

  priv = EMPATHY_IRC_NETWORK_GET_PRIVATE (self);

  priv->servers = g_slist_append (priv->servers, server);
  g_signal_emit (self, signals[MODIFIED], 0);
}
