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
#include "empathy-ui-utils.h"

#include "empathy-irc-network-dialog.h"

#define DEBUG_DOMAIN "AccountWidgetIRC"

typedef struct {
  EmpathyIrcNetwork *network;

  GtkWidget *dialog;
  GtkWidget *button_close;

  GtkWidget *entry_network;
  GtkWidget *combobox_charset;

  GtkWidget *treeview_servers;
  GtkWidget *button_add;
  GtkWidget *button_remove;
  GtkWidget *button_up;
  GtkWidget *button_down;
} EmpathyIrcNetworkDialog;

static void
irc_network_dialog_destroy_cb (GtkWidget *widget,
                               EmpathyIrcNetworkDialog *dialog)
{
  g_object_unref (dialog->network);

  g_slice_free (EmpathyIrcNetworkDialog, dialog);
}

static void
irc_network_dialog_close_clicked_cb (GtkWidget *widget,
                                     EmpathyIrcNetworkDialog *dialog)
{
  gtk_widget_destroy (dialog->dialog);
}

enum {
  COL_SRV_OBJ,
  COL_ADR,
  COL_PORT,
  COL_SSL
};

static void
add_server_to_store (GtkListStore *store,
                     EmpathyIrcServer *server)
{
  GtkTreeIter iter;
  gchar *address;
  guint port;
  gboolean ssl;

  g_object_get (server,
      "address", &address,
      "port", &port,
      "ssl", &ssl,
      NULL);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, COL_SRV_OBJ, server,
      COL_ADR, address, COL_PORT, port, COL_SSL, ssl, -1);

  g_free (address);
}

static void
irc_network_dialog_setup (EmpathyIrcNetworkDialog *dialog)
{
  gchar *name;
  GSList *servers, *l;
  GtkListStore *store;

  g_object_get (dialog->network, "name", &name, NULL);
  gtk_entry_set_text (GTK_ENTRY (dialog->entry_network), name);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (
        GTK_TREE_VIEW (dialog->treeview_servers)));

  servers = empathy_irc_network_get_servers (dialog->network);
  for (l = servers; l != NULL; l = g_slist_next (l))
    {
      EmpathyIrcServer *server = l->data;

      add_server_to_store (store, server);
    }

  /* TODO charset */

  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);
  g_free (name);
}

static void
irc_network_dialog_address_edited_cb (GtkCellRendererText *renderer,
                                      gchar *path,
                                      gchar *new_text,
                                      EmpathyIrcNetworkDialog *dialog)
{
  EmpathyIrcServer *server;
  GtkTreeModel *model;
  GtkTreePath  *treepath;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->treeview_servers));
  treepath = gtk_tree_path_new_from_string (path);
  gtk_tree_model_get_iter (model, &iter, treepath);
  gtk_tree_model_get (model, &iter,
      COL_SRV_OBJ, &server,
      -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      COL_ADR, new_text,
      -1);

  g_object_set (server, "address", new_text, NULL);

  gtk_tree_path_free (treepath);
  g_object_unref (server);
}

static void
irc_network_dialog_port_edited_cb (GtkCellRendererText *renderer,
                                   gchar *path,
                                   gchar *new_text,
                                   EmpathyIrcNetworkDialog *dialog)
{
  EmpathyIrcServer *server;
  GtkTreeModel *model;
  GtkTreePath  *treepath;
  GtkTreeIter iter;
  guint port;

  port = strtoul (new_text, NULL, 10);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->treeview_servers));
  treepath = gtk_tree_path_new_from_string (path);
  gtk_tree_model_get_iter (model, &iter, treepath);
  gtk_tree_model_get (model, &iter,
      COL_SRV_OBJ, &server,
      -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      COL_PORT, port,
      -1);

  g_object_set (server, "port", port, NULL);

  gtk_tree_path_free (treepath);
  g_object_unref (server);
}

static void
irc_network_dialog_ssl_toggled_cb (GtkCellRendererText *renderer,
                                   gchar *path,
                                   EmpathyIrcNetworkDialog *dialog)
{
  EmpathyIrcServer *server;
  GtkTreeModel *model;
  GtkTreePath  *treepath;
  GtkTreeIter iter;
  gboolean ssl;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->treeview_servers));
  treepath = gtk_tree_path_new_from_string (path);
  gtk_tree_model_get_iter (model, &iter, treepath);
  gtk_tree_model_get (model, &iter,
      COL_SRV_OBJ, &server,
      COL_SSL, &ssl,
      -1);
  ssl = !ssl;
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      COL_SSL, ssl,
      -1);

  g_object_set (server, "ssl", ssl, NULL);

  gtk_tree_path_free (treepath);
  g_object_unref (server);
}

static gboolean
irc_network_dialog_network_focus_cb (GtkWidget *widget,
                                     GdkEventFocus *event,
                                     EmpathyIrcNetworkDialog *dialog)
{
  const gchar *str;

  str = gtk_entry_get_text (GTK_ENTRY (widget));

  g_object_set (dialog->network, "name", str, NULL);

  return FALSE;
}

static void
irc_network_dialog_button_add_clicked_cb (GtkWidget *widget,
                                          EmpathyIrcNetworkDialog *dialog)
{
  EmpathyIrcServer *server;
  GtkListStore *store;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (
        GTK_TREE_VIEW (dialog->treeview_servers)));

  server = empathy_irc_server_new (_("new server"), 6667, FALSE);
  empathy_irc_network_add_server (dialog->network, server);
  add_server_to_store (store, server);

  /* TODO: Set the focus in the address cell of this new server */

  g_object_unref (server);
}

static void
irc_network_dialog_button_remove_clicked_cb (GtkWidget *widget,
                                             EmpathyIrcNetworkDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  EmpathyIrcServer *server;

  selection = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (dialog->treeview_servers));

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, COL_SRV_OBJ, &server, -1);

  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  empathy_irc_network_remove_server (dialog->network, server);

  g_object_unref (server);
}

static void
irc_network_dialog_button_up_down_clicked_cb (GtkWidget *widget,
                                              EmpathyIrcNetworkDialog *dialog)
{
  /* TODO */
  g_print ("up-down\n");
}

static void
irc_network_dialog_selection_changed_cb (GtkTreeSelection  *treeselection,
                                         EmpathyIrcNetworkDialog *dialog)
{
  /* TODO: sensitive / unsensitive buttons according current
   * selection/configuration */
  g_print ("selection changed\n");
}

GtkWidget *
irc_network_dialog_show (EmpathyIrcNetwork *network,
                         GtkWidget *parent)
{
  static EmpathyIrcNetworkDialog *dialog = NULL;
  GladeXML *glade;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkAdjustment *adjustment;
  GtkTreeSelection *selection;

  g_return_val_if_fail (network != NULL, NULL);

  if (dialog != NULL)
    {
      gtk_window_present (GTK_WINDOW (dialog->dialog));
      /* TODO: set the right network */

      return dialog->dialog;
    }

  dialog = g_slice_new0 (EmpathyIrcNetworkDialog);

  dialog->network = network;
  g_object_ref (dialog->network);

  glade = empathy_glade_get_file ("empathy-account-widget-irc.glade",
      "irc_network_dialog",
      NULL,
      "irc_network_dialog", &dialog->dialog,
      "button_close", &dialog->button_close,
      "entry_network", &dialog->entry_network,
      "combobox_charset", &dialog->combobox_charset,
      "treeview_servers", &dialog->treeview_servers,
      "button_add", &dialog->button_add,
      "button_remove", &dialog->button_remove,
      "button_up", &dialog->button_up,
      "button_down", &dialog->button_down,
      NULL);

  store = gtk_list_store_new (4, G_TYPE_OBJECT, G_TYPE_STRING,
      G_TYPE_UINT, G_TYPE_BOOLEAN);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->treeview_servers),
      GTK_TREE_MODEL (store));
  g_object_unref (store);

  /* address */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "editable", TRUE, NULL);
  g_signal_connect (renderer, "edited",
      G_CALLBACK (irc_network_dialog_address_edited_cb), dialog);
  gtk_tree_view_insert_column_with_attributes (
      GTK_TREE_VIEW (dialog->treeview_servers),
      -1, _("Server"), renderer, "text", COL_ADR,
      NULL);

  /* port */
  adjustment = (GtkAdjustment *) gtk_adjustment_new (6667, 1, G_MAXUINT16,
      1, 10, 0);
  renderer = gtk_cell_renderer_spin_new ();
  g_object_set (renderer,
      "editable", TRUE, 
      "adjustment", adjustment,
      NULL);
  g_signal_connect (renderer, "edited",
      G_CALLBACK (irc_network_dialog_port_edited_cb), dialog);
  gtk_tree_view_insert_column_with_attributes (
      GTK_TREE_VIEW (dialog->treeview_servers),
      -1, _("Port"), renderer, "text", COL_PORT,
      NULL);

  /* SSL */
  renderer = gtk_cell_renderer_toggle_new ();
  g_object_set (renderer, "activatable", TRUE, NULL);
  g_signal_connect (renderer, "toggled",
      G_CALLBACK (irc_network_dialog_ssl_toggled_cb), dialog);
  gtk_tree_view_insert_column_with_attributes (
      GTK_TREE_VIEW (dialog->treeview_servers),
      -1, _("SSL"), renderer, "active", COL_SSL,
      NULL);

  selection = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (dialog->treeview_servers));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  irc_network_dialog_setup (dialog);

  empathy_glade_connect (glade, dialog,
      "irc_network_dialog", "destroy", irc_network_dialog_destroy_cb,
      "button_close", "clicked", irc_network_dialog_close_clicked_cb,
      "entry_network", "focus-out-event", irc_network_dialog_network_focus_cb,
      "button_add", "clicked", irc_network_dialog_button_add_clicked_cb,
      "button_remove", "clicked", irc_network_dialog_button_remove_clicked_cb,
      "button_up", "clicked", irc_network_dialog_button_up_down_clicked_cb,
      "button_down", "clicked", irc_network_dialog_button_up_down_clicked_cb,
      NULL);

  g_object_unref (glade);

  g_object_add_weak_pointer (G_OBJECT (dialog->dialog),
      (gpointer) &dialog);

  g_signal_connect (selection, "changed",
      G_CALLBACK (irc_network_dialog_selection_changed_cb),
      dialog);

  gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
      GTK_WINDOW (parent));
  gtk_window_set_modal (GTK_WINDOW (dialog->dialog), TRUE);

  gtk_widget_show_all (dialog->dialog);

  return dialog->dialog;
}
