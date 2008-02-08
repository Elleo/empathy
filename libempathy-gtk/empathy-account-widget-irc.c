/*
 * Copyright (C) 2007-2008 Guillaume Desmottes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Guillaume Desmottes <gdesmott@gnome.org>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include <libmissioncontrol/mc-account.h>
#include <libmissioncontrol/mc-protocol.h>

#include <libempathy/empathy-utils.h>
#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-irc-network-manager.h>

#include "empathy-irc-network-dialog.h"
#include "empathy-account-widget-irc.h"
#include "empathy-ui-utils.h"

#define DEBUG_DOMAIN "AccountWidgetIRC"

#define IRC_NETWORKS_FILENAME "irc-networks.xml"

typedef struct {
  McAccount *account;
  EmpathyIrcNetworkManager *network_manager;

  GtkWidget *vbox_settings;

  GtkWidget *combobox_network;
  GtkWidget *button_network;
  GtkWidget *button_remove;

  GtkWidget *entry_nick;
  GtkWidget *entry_password;
  GtkWidget *entry_fullname;
  GtkWidget *entry_quit_message;
} EmpathyAccountWidgetIrc;

enum {
  COL_NETWORK_OBJ,
  COL_NETWORK_NAME,
};

static gboolean
account_widget_irc_entry_focus_cb (GtkWidget *widget,
                                   GdkEventFocus *event,
                                   EmpathyAccountWidgetIrc *settings)
{
  const gchar *param;
  const gchar *str;

  if (widget == settings->entry_nick)
    {
      param = "account";
    }
  else if (widget == settings->entry_fullname)
    {
      param = "fullname";
    }
  else if (widget == settings->entry_password)
    {
      param = "password";
    }
  else if (widget == settings->entry_quit_message)
    {
      param = "quit-message";
    }
  else
    {
      return FALSE;
    }

  str = gtk_entry_get_text (GTK_ENTRY (widget));
  if (G_STR_EMPTY (str))
    {
      gchar *value = NULL;

      mc_account_unset_param (settings->account, param);
      mc_account_get_param_string (settings->account, param, &value);
      empathy_debug (DEBUG_DOMAIN, "Unset %s and restore to %s", param, value);
      gtk_entry_set_text (GTK_ENTRY (widget), value ? value : "");
      g_free (value);
    }
  else
    {
      empathy_debug (DEBUG_DOMAIN, "Setting %s to %s", param, str);
      mc_account_set_param_string (settings->account, param, str);
    }

  return FALSE;
}

static void
account_widget_irc_destroy_cb (GtkWidget *widget,
                               EmpathyAccountWidgetIrc *settings)
{
  empathy_irc_network_manager_store (settings->network_manager);
  g_object_unref (settings->network_manager);
  g_object_unref (settings->account);
  g_slice_free (EmpathyAccountWidgetIrc, settings);
}

static void
irc_network_dialog_destroy_cb (GtkWidget *widget,
                               EmpathyAccountWidgetIrc *settings)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  EmpathyIrcNetwork *network;
  gchar *name;
  GSList *servers;

  /* name could be changed */
  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (settings->combobox_network),
      &iter);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (settings->combobox_network));
  gtk_tree_model_get (model, &iter, COL_NETWORK_OBJ, &network, -1);

  g_object_get (network, "name", &name, NULL);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      COL_NETWORK_NAME, name, -1);

  /* TODO: update charset */
  servers = empathy_irc_network_get_servers (network);
  if (servers == NULL)
    {
      /* Unset all values */
      empathy_debug (DEBUG_DOMAIN, "Unset server, port and ssl");
      mc_account_unset_param (settings->account, "server");
      mc_account_unset_param (settings->account, "port");
      mc_account_unset_param (settings->account, "ssl");
    }
  else
    {
      EmpathyIrcServer *server;
      gchar *address;
      guint port;
      gboolean ssl;

      /* Take the first server on the list */
      server = servers->data;

      g_object_get (server,
          "address", &address,
          "port", &port,
          "ssl", &ssl,
          NULL);

      empathy_debug (DEBUG_DOMAIN, "Setting server to %s", address);
      mc_account_set_param_string (settings->account, "server", address);
      empathy_debug (DEBUG_DOMAIN, "Setting port to %u", port);
      mc_account_set_param_int (settings->account, "port", port);
      empathy_debug (DEBUG_DOMAIN, "Setting ssl to %s", ssl ? "TRUE": "FALSE" );
      mc_account_set_param_boolean (settings->account, "use-ssl", ssl);

      g_free (address);
    }


  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);
  g_object_unref (network);
  g_free (name);
}

static void
account_widget_irc_button_network_clicked_cb (GtkWidget *button,
                                              EmpathyAccountWidgetIrc *settings)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  EmpathyIrcNetwork *network;
  GtkWindow *window;
  GtkWidget *dialog;

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (settings->combobox_network),
      &iter);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (settings->combobox_network));
  gtk_tree_model_get (model, &iter, COL_NETWORK_OBJ, &network, -1);

  if (network == NULL)
    return;

  window = empathy_get_toplevel_window (settings->vbox_settings);
  dialog = irc_network_dialog_show (settings->account, network,
      GTK_WIDGET (window));
  g_signal_connect (dialog, "destroy",
      G_CALLBACK (irc_network_dialog_destroy_cb), settings);

  g_object_unref (network);
}

static void
account_widget_irc_button_remove_clicked_cb (GtkWidget *button,
                                             EmpathyAccountWidgetIrc *settings)
{
  EmpathyIrcNetwork *network;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *name;

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (settings->combobox_network),
      &iter);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (settings->combobox_network));
  gtk_tree_model_get (model, &iter, COL_NETWORK_OBJ, &network, -1);

  if (network == NULL)
    return;

  g_object_get (network, "name", &name, NULL);
  empathy_debug (DEBUG_DOMAIN, "Remove network %s", name);

  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  empathy_irc_network_manager_remove (settings->network_manager, network);

  g_free (name);
  g_object_unref (network);
}

static gboolean
account_widget_irc_is_separator (GtkTreeModel *model,
                                 GtkTreeIter *iter,
                                 gpointer data)
{
  GtkTreePath *path;
  gboolean result;

  path = gtk_tree_model_get_path (model, iter);
  result = gtk_tree_path_get_indices (path)[0] == 0;
  gtk_tree_path_free (path);

  /* return result; */
  /* FIXME: "New..." disappear if we display the separator */
  return FALSE;
}

static gint
account_widget_irc_sort (GtkTreeModel *model,
                         GtkTreeIter *a,
                         GtkTreeIter *b,
                         gpointer user_data)
{
  gchar *name_a, *name_b;
  EmpathyIrcNetwork *network_a, *network_b;
  gint result;

  gtk_tree_model_get (model, a,
      COL_NETWORK_OBJ, &network_a,
      COL_NETWORK_NAME, &name_a,
      -1);
  gtk_tree_model_get (model, b,
      COL_NETWORK_OBJ, &network_b,
      COL_NETWORK_NAME, &name_b,
      -1);

  if (network_a == NULL)
    result = -1;
  else if (network_b == NULL)
    result = 1;
  else
    result = strcmp (name_a, name_b);

  g_free (name_a);
  g_free (name_b);
  if (network_a != NULL)
    g_object_unref (network_a);
  if (network_b != NULL)
    g_object_unref (network_b);

  return result;
}

static void
unset_server_values (EmpathyAccountWidgetIrc *settings)
{
  mc_account_unset_param (settings->account, "server");
  mc_account_unset_param (settings->account, "port");
  mc_account_unset_param (settings->account, "use-ssl");
}

static void
account_widget_irc_combobox_network_changed_cb (GtkWidget *combobox,
                                                EmpathyAccountWidgetIrc *settings)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  EmpathyIrcNetwork *network;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &iter))
    {
      unset_server_values (settings);
      return;
    }

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
  gtk_tree_model_get (model, &iter, COL_NETWORK_OBJ, &network, -1);

  if (network == NULL)
    {
      /* TODO Open new network dialog */
    }
  else
    {
      GSList *servers;

      servers = empathy_irc_network_get_servers (network);
      if (g_slist_length (servers) > 0)
        {
          /* set the first server as CM server */
          EmpathyIrcServer *server = servers->data;
          gchar *address;
          guint port;
          gboolean ssl;

          g_object_get (server,
              "address", &address,
              "port", &port,
              "ssl", &ssl,
              NULL);

          empathy_debug (DEBUG_DOMAIN, "Setting server to %s", address);
          mc_account_set_param_string (settings->account, "server", address);
          mc_account_set_param_int (settings->account, "port", port);
          mc_account_set_param_boolean (settings->account, "use-ssl", ssl);
          /* TODO: charset */

          g_free (address);
        }
      else
        {
          /* No server. Unset values */
          unset_server_values (settings);
        }

      g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
      g_slist_free (servers);
      g_object_unref (network);
    }
}

static void
fill_networks_model (EmpathyAccountWidgetIrc *settings,
                     EmpathyIrcNetwork *network_to_select)
{
  GSList *networks, *l;
  GtkTreeModel *model;
  GtkListStore *store;

  networks = empathy_irc_network_manager_get_networks (
      settings->network_manager);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (settings->combobox_network));
  store = GTK_LIST_STORE (model);

  for (l = networks; l != NULL; l = g_slist_next (l))
    {
      gchar *name;
      EmpathyIrcNetwork *network = l->data;
      GtkTreeIter iter;

      g_object_get (network, "name", &name, NULL);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, COL_NETWORK_OBJ, network,
          COL_NETWORK_NAME, name, -1);

       if (network == network_to_select)
         {
           gtk_combo_box_set_active_iter (
               GTK_COMBO_BOX (settings->combobox_network), &iter);
         }

      g_free (name);
      g_object_unref (network);
    }

  if (network_to_select == NULL)
    {
      /* Select the first network. So skip the "New..." */
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter_first (model, &iter) &&
          gtk_tree_model_iter_next (model, &iter))
        {
          gtk_combo_box_set_active_iter (
              GTK_COMBO_BOX (settings->combobox_network), &iter);
        }
    }

  g_slist_free (networks);
}

static void
account_widget_irc_setup (EmpathyAccountWidgetIrc *settings)
{
  gchar *nick = NULL;
  gchar *password = NULL;
  gchar *fullname = NULL;
  gchar *quit_message= NULL;
  gchar *server = NULL;
  gint port = 6667;
  gchar *charset;
  gboolean ssl = FALSE;
  EmpathyIrcNetwork *network = NULL;

  mc_account_get_param_string (settings->account, "account", &nick);
  mc_account_get_param_string (settings->account, "fullname", &fullname);
  mc_account_get_param_string (settings->account, "password", &password);
  mc_account_get_param_string (settings->account, "quit-message",
      &quit_message);
  mc_account_get_param_string (settings->account, "server", &server);
  mc_account_get_param_string (settings->account, "charset", &charset);
  mc_account_get_param_int (settings->account, "port", &port);
  mc_account_get_param_boolean (settings->account, "use-ssl", &ssl);

  if (!nick)
    {
      nick = g_strdup (g_get_user_name ());
      mc_account_set_param_string (settings->account, "account", nick);
    }

  if (!fullname)
    {
      fullname = g_strdup (g_get_real_name ());
      if (!fullname)
        {
          fullname = g_strdup (nick);
        }
      mc_account_set_param_string (settings->account, "fullname", fullname);
    }

  gtk_entry_set_text (GTK_ENTRY (settings->entry_nick), nick ? nick : "");
  gtk_entry_set_text (GTK_ENTRY (settings->entry_password),
      password ? password : "");
  gtk_entry_set_text (GTK_ENTRY (settings->entry_fullname),
      fullname ? fullname : "");
  gtk_entry_set_text (GTK_ENTRY (settings->entry_quit_message),
      quit_message ? quit_message : "");


  if (server != NULL)
    {
      GtkListStore *store;

      network = empathy_irc_network_manager_find_network_by_address (
          settings->network_manager, server);

      store = GTK_LIST_STORE (gtk_combo_box_get_model (
            GTK_COMBO_BOX (settings->combobox_network)));

      if (network != NULL)
        {
          gchar *name;

          g_object_get (network, "name", &name, NULL);
          empathy_debug (DEBUG_DOMAIN, "Account use network %s", name);

          g_free (name);
        }
      else
        {
          /* We don't have this network. Let's create it */
          EmpathyIrcServer *srv;
          GtkTreeIter iter;

          empathy_debug (DEBUG_DOMAIN, "Create a network %s", server);
          network = empathy_irc_network_new (server);
          srv = empathy_irc_server_new (server, port, ssl);

          empathy_irc_network_add_server (network, srv);
          empathy_irc_network_manager_add (settings->network_manager, network);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter, COL_NETWORK_OBJ, network,
              COL_NETWORK_NAME, server, -1);
          gtk_combo_box_set_active_iter (
              GTK_COMBO_BOX (settings->combobox_network), &iter);

          g_object_unref (srv);
          g_object_unref (network);
        }
    }

  fill_networks_model (settings, network);

  g_free (nick);
  g_free (fullname);
  g_free (password);
  g_free (quit_message);
  g_free (server);
  g_free (charset);
}

GtkWidget *
empathy_account_widget_irc_new (McAccount *account)
{
  EmpathyAccountWidgetIrc *settings;
  gchar *dir, *user_file_with_path, *global_file_with_path;
  GladeXML *glade;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkSizeGroup *size_group;
  GtkWidget *label_network, *label_nick, *label_fullname;
  GtkWidget *label_password, *label_quit_message;

  settings = g_slice_new0 (EmpathyAccountWidgetIrc);
  settings->account = g_object_ref (account);

  dir = g_build_filename (g_get_home_dir (), ".gnome2", PACKAGE_NAME, NULL);
  g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
  user_file_with_path = g_build_filename (dir, IRC_NETWORKS_FILENAME, NULL);
  g_free (dir);

  global_file_with_path = g_build_filename (UNINSTALLED_IRC_DIR,
      IRC_NETWORKS_FILENAME, NULL);

  settings->network_manager = empathy_irc_network_manager_new (
      global_file_with_path,
      user_file_with_path);

  g_free (global_file_with_path);
  g_free (user_file_with_path);

  glade = empathy_glade_get_file ("empathy-account-widget-irc.glade",
      "vbox_irc_settings",
      NULL,
      "vbox_irc_settings", &settings->vbox_settings,
      "combobox_network", &settings->combobox_network,
      "button_network", &settings->button_network,
      "button_remove", &settings->button_remove,
      "label_network", &label_network,
      "label_nick", &label_nick,
      "label_fullname", &label_fullname,
      "label_password", &label_password,
      "label_quit_message", &label_quit_message,
      "entry_nick", &settings->entry_nick,
      "entry_fullname", &settings->entry_fullname,
      "entry_password", &settings->entry_password,
      "entry_quit_message", &settings->entry_quit_message,
      NULL);

  /* Fill the networks combobox */
  store = gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_STRING);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, COL_NETWORK_OBJ, NULL,
      COL_NETWORK_NAME, _("New..."), -1);

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (settings->combobox_network)); 
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (settings->combobox_network),
      renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (settings->combobox_network),
      renderer,
      "text", COL_NETWORK_NAME,
      NULL);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
      COL_NETWORK_NAME,
      GTK_SORT_ASCENDING);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
      COL_NETWORK_NAME,
      account_widget_irc_sort,
      NULL, NULL);

  gtk_combo_box_set_row_separator_func (
      GTK_COMBO_BOX (settings->combobox_network),
      account_widget_irc_is_separator,
      NULL, NULL);

  gtk_combo_box_set_model (GTK_COMBO_BOX (settings->combobox_network),
      GTK_TREE_MODEL (store));
  g_object_unref (store);

  account_widget_irc_setup (settings);

  empathy_glade_connect (glade, settings,
      "vbox_irc_settings", "destroy", account_widget_irc_destroy_cb,
      "entry_nick", "focus-out-event", account_widget_irc_entry_focus_cb,
      "entry_fullname", "focus-out-event", account_widget_irc_entry_focus_cb,
      "entry_password", "focus-out-event", account_widget_irc_entry_focus_cb,
      "entry_quit_message", "focus-out-event", account_widget_irc_entry_focus_cb,
      "button_network", "clicked", account_widget_irc_button_network_clicked_cb,
      "button_remove", "clicked", account_widget_irc_button_remove_clicked_cb,
      "combobox_network", "changed", account_widget_irc_combobox_network_changed_cb,
      NULL);

  g_object_unref (glade);

  /* Set up remaining widgets */
  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_size_group_add_widget (size_group, label_nick);
  gtk_size_group_add_widget (size_group, label_fullname);
  gtk_size_group_add_widget (size_group, label_password);
  gtk_size_group_add_widget (size_group, label_quit_message);

  g_object_unref (size_group);

  return settings->vbox_settings;
}
