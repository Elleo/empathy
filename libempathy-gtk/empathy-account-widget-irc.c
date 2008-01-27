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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include <libmissioncontrol/mc-account.h>
#include <libmissioncontrol/mc-protocol.h>

#include <libempathy/empathy-irc-network-manager.h>
#include "empathy-account-widget-irc.h"
#include "empathy-ui-utils.h"


typedef struct {
  McAccount *account;
  gboolean account_changed;
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

  /*
static void
account_widget_irc_save (EmpathyAccountWidgetIrc *settings)
{
  EmpathySession        *session;
  EmpathyAccountManager *manager;
  const gchar          *nick;
  const gchar          *password;
  const gchar          *fullname;
  const gchar          *quit_message;

  session = empathy_app_get_session ();
  manager = empathy_session_get_account_manager (session);

  nick = gtk_entry_get_text (GTK_ENTRY (settings->entry_nick));
  password = gtk_entry_get_text (GTK_ENTRY (settings->entry_password));
  fullname = gtk_entry_get_text (GTK_ENTRY (settings->entry_fullname));
  quit_message = gtk_entry_get_text (GTK_ENTRY (settings->entry_quit_message));

  empathy_account_param_set (settings->account,
      "account", nick,
      "password", password,
      "fullname", fullname,
      "quit-message", quit_message,
      "server", "irc.freenode.net",
      "port", 6667,
      "charset", "UTF-8"
      "use_ssl", FALSE,
      NULL);

  empathy_account_manager_store (manager);

  settings->account_changed = FALSE;
}
  */

/*
static void
account_widget_irc_protocol_error_cb (EmpathySession             *session,
    EmpathyProtocol            *protocol,
    EmpathyAccount             *account,
    GError                    *error,
    EmpathyAccountWidgetIrc *settings)
{
  if (empathy_account_equal (account, settings->account)) {
  } 
}
*/

static gboolean
account_widget_irc_entry_focus_cb (GtkWidget *widget,
                                   GdkEventFocus *event,
                                   EmpathyAccountWidgetIrc *settings)
{
  /*
  const gchar *str;

  str = gtk_entry_get_text (GTK_ENTRY (widget));
  if (G_STR_EMPTY (str)) {
    if (widget == settings->entry_nick) {
      empathy_account_param_get (settings->account, "account", &str, NULL);
    } else if (widget == settings->entry_fullname) {
      empathy_account_param_get (settings->account, "fullname", &str, NULL);
    } else if (widget == settings->entry_quit_message) {
      empathy_account_param_get (settings->account, "quit-message", &str, NULL);
    }

    gtk_entry_set_text (GTK_ENTRY (widget), str ? str : "");
    settings->account_changed = FALSE;
  }

  if (settings->account_changed) {
    account_widget_irc_save (settings); 
  }

  */
  return FALSE;
}

static void
account_widget_irc_entry_changed_cb (GtkWidget *widget,
                                     EmpathyAccountWidgetIrc *settings)
{
  settings->account_changed = TRUE;
}

static void
account_widget_irc_destroy_cb (GtkWidget *widget,
                               EmpathyAccountWidgetIrc *settings)
{
  /*
  EmpathySession *session;

  if (settings->account_changed) {
    account_widget_irc_save (settings); 
  }

  session = empathy_app_get_session ();

  g_signal_handlers_disconnect_by_func (session,
      account_widget_irc_protocol_error_cb,
      settings);

  g_object_unref (settings->network_manager);

  g_object_unref (settings->account);
  g_free (settings);
  */
}

static void
account_widget_irc_button_network_clicked_cb (GtkWidget *button,
                                              EmpathyAccountWidgetIrc *settings)
{
  g_print ("edit network\n");
  //empathy_irc_network_dialog_show ();
}

static void
account_widget_irc_button_remove_clicked_cb (GtkWidget *button,
                                             EmpathyAccountWidgetIrc *settings)
{
  g_print ("remove\n");
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
account_widget_irc_combobox_network_changed_cb (GtkWidget *combobox,
                                                EmpathyAccountWidgetIrc *settings)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  EmpathyIrcNetwork *network;

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &iter);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
  gtk_tree_model_get (model, &iter, COL_NETWORK_OBJ, &network, -1);

  if (network == NULL)
    {
      /* TODO Open new network dialog */
    }
  else
    {
      /* TODO: change account setting */
      g_object_unref (network);
    }
}

static void
account_widget_irc_setup (EmpathyAccountWidgetIrc *settings)
{
  /*
  McProtocol*protocol;
  McProfile *profile;
  const gchar    *nick;
  const gchar    *password;
  const gchar    *fullname;
  const gchar    *quit_message;

  session = empathy_app_get_session ();
  protocol = empathy_session_get_protocol (session, settings->account);

  empathy_account_param_get (settings->account,
      "account", &nick,
      "fullname", &fullname,
      "password", &password,
      "quit-message", &quit_message,
      NULL);

  if (!nick)
    nick = g_get_user_name ();

  if (!fullname)
    {
      fullname = g_get_real_name ();
      if (!fullname)
        fullname = nick;
    }

  gtk_entry_set_text (GTK_ENTRY (settings->entry_nick), nick ? nick : "");
  gtk_entry_set_text (GTK_ENTRY (settings->entry_password),
      password ? password : "");
  gtk_entry_set_text (GTK_ENTRY (settings->entry_fullname),
      fullname ? fullname : "");
  gtk_entry_set_text (GTK_ENTRY (settings->entry_quit_message),
      quit_message ? quit_message : "");
      */

  /* Set up protocol signals */
  /*
  g_signal_connect (session, "protocol-error",
      G_CALLBACK (account_widget_irc_protocol_error_cb),
      settings);
      */
}

GtkWidget *
empathy_account_widget_irc_new (McAccount *account)
{
  EmpathyAccountWidgetIrc *settings;
  GladeXML *glade;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkSizeGroup *size_group;
  GtkWidget *label_network, *label_nick, *label_fullname;
  GtkWidget *label_password, *label_quit_message;
  GSList *networks, *l;

  settings = g_new0 (EmpathyAccountWidgetIrc, 1);
  settings->account = g_object_ref (account);

  /* FIXME: set the right paths */
  settings->network_manager = empathy_irc_network_manager_new (
      "/home/cassidy/gnome/empathy/tests/xml/default-irc-networks-sample.xml",
      NULL);

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

  networks = empathy_irc_network_manager_get_networks (
      settings->network_manager);

  for (l = networks; l != NULL; l = g_slist_next (l))
    {
      gchar *name;
      EmpathyIrcNetwork *network = l->data;

      g_object_get (network, "name", &name, NULL);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, COL_NETWORK_OBJ, network,
          COL_NETWORK_NAME, name, -1);

      g_free (name);
      g_object_unref (network);
    }
  g_slist_free (networks);

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
      "entry_nick", "changed", account_widget_irc_entry_changed_cb,
      "entry_password", "changed", account_widget_irc_entry_changed_cb,
      "entry_fullname", "changed", account_widget_irc_entry_changed_cb,
      "entry_quit_message", "changed", account_widget_irc_entry_changed_cb,
      "entry_nick", "focus-out-event", account_widget_irc_entry_focus_cb,
      "entry_fullname", "focus-out-event", account_widget_irc_entry_focus_cb,
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
