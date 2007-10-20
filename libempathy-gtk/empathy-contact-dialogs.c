/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Collabora Ltd.
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
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>

#include <libmissioncontrol/mission-control.h>

#include <libempathy/empathy-contact-manager.h>
#include <libempathy/empathy-contact-list.h>
#include <libempathy/empathy-utils.h>

#include "empathy-contact-dialogs.h"
#include "empathy-contact-widget.h"
#include "empathy-ui-utils.h"

static GList *subscription_dialogs = NULL;
static GList *information_dialogs = NULL;
static GtkWidget *new_contact_dialog = NULL;


static gint
contact_dialogs_find (GtkDialog      *dialog,
		      EmpathyContact *contact)
{
	GtkWidget     *contact_widget;
	EmpathyContact *this_contact;

	contact_widget = g_object_get_data (G_OBJECT (dialog), "contact_widget");
	this_contact = empathy_contact_widget_get_contact (contact_widget);

	return !empathy_contact_equal (contact, this_contact);
}

/*
 *  Subscription dialog
 */

static void
subscription_dialog_response_cb (GtkDialog *dialog,
				 gint       response,
				 GtkWidget *contact_widget)
{
	EmpathyContactManager *manager;
	EmpathyContact        *contact;

	manager = empathy_contact_manager_new ();
	contact = empathy_contact_widget_get_contact (contact_widget);

	if (response == GTK_RESPONSE_YES) {
		empathy_contact_list_add (EMPATHY_CONTACT_LIST (manager),
					  contact, "");
	}
	else if (response == GTK_RESPONSE_NO) {
		empathy_contact_list_remove (EMPATHY_CONTACT_LIST (manager),
					     contact, "");
	}

	subscription_dialogs = g_list_remove (subscription_dialogs, dialog);
	gtk_widget_destroy (GTK_WIDGET (dialog));
	g_object_unref (manager);
}

void
empathy_subscription_dialog_show (EmpathyContact *contact,
				  GtkWindow     *parent)
{
	GtkWidget *dialog;
	GtkWidget *hbox_subscription;
	GtkWidget *contact_widget;
	GList     *l;

	g_return_if_fail (EMPATHY_IS_CONTACT (contact));

	l = g_list_find_custom (subscription_dialogs,
				contact,
				(GCompareFunc) contact_dialogs_find);
	if (l) {
		gtk_window_present (GTK_WINDOW (l->data));
		return;
	}

	empathy_glade_get_file_simple ("empathy-contact-dialogs.glade",
				      "subscription_request_dialog",
				      NULL,
				      "subscription_request_dialog", &dialog,
				      "hbox_subscription", &hbox_subscription,
				      NULL);

	contact_widget = empathy_contact_widget_new (contact,
						     EMPATHY_CONTACT_WIDGET_EDIT_ALIAS |
						     EMPATHY_CONTACT_WIDGET_EDIT_GROUPS);
	gtk_box_pack_end (GTK_BOX (hbox_subscription),
			  contact_widget,
			  TRUE, TRUE,
			  0);

	g_object_set_data (G_OBJECT (dialog), "contact_widget", contact_widget);
	subscription_dialogs = g_list_prepend (subscription_dialogs, dialog);

	g_signal_connect (dialog, "response",
			  G_CALLBACK (subscription_dialog_response_cb),
			  contact_widget);

	if (parent) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	}

	gtk_widget_show (dialog);
}

/*
 *  Information dialog
 */

static void
contact_information_response_cb (GtkDialog *dialog,
				 gint       response,
				 GtkWidget *contact_widget)
{
	information_dialogs = g_list_remove (information_dialogs, dialog);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
empathy_contact_information_dialog_show (EmpathyContact *contact,
					 GtkWindow      *parent,
					 gboolean        edit,
					 gboolean        edit_groups)
{
	GtkWidget                *dialog;
	GtkWidget                *button;
	GtkWidget                *contact_widget;
	GList                    *l;
	EmpathyContactWidgetFlags flags = 0;

	g_return_if_fail (EMPATHY_IS_CONTACT (contact));

	l = g_list_find_custom (information_dialogs,
				contact,
				(GCompareFunc) contact_dialogs_find);
	if (l) {
		gtk_window_present (GTK_WINDOW (l->data));
		return;
	}

	/* Create dialog */
	dialog = gtk_dialog_new ();
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Contact information"));

	/* Close button */
	button = gtk_button_new_with_label (GTK_STOCK_CLOSE);
	gtk_button_set_use_stock (GTK_BUTTON (button), TRUE);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      button,
				      GTK_RESPONSE_CLOSE);
	gtk_widget_show (button);

	/* Contact info widget */
	if (edit) {
		flags |= EMPATHY_CONTACT_WIDGET_EDIT_ALIAS;
		if (empathy_contact_is_user (contact)) {
			flags |= EMPATHY_CONTACT_WIDGET_EDIT_ACCOUNT |
				 EMPATHY_CONTACT_WIDGET_EDIT_AVATAR;
		}
	}
	if (edit_groups) {
		flags |= EMPATHY_CONTACT_WIDGET_EDIT_GROUPS;
	}
	contact_widget = empathy_contact_widget_new (contact, flags);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    contact_widget,
			    TRUE, TRUE, 0);
	if (flags & EMPATHY_CONTACT_WIDGET_EDIT_ACCOUNT) {
		empathy_contact_widget_set_account_filter (contact_widget,
							   empathy_account_chooser_filter_is_connected,
							   NULL);
	}

	g_object_set_data (G_OBJECT (dialog), "contact_widget", contact_widget);
	information_dialogs = g_list_prepend (information_dialogs, dialog);

	g_signal_connect (dialog, "response",
			  G_CALLBACK (contact_information_response_cb),
			  contact_widget);

	if (parent) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	}

	gtk_widget_show (dialog);
}

/*
 *  New contact dialog
 */

static gboolean
can_add_contact_to_account (McAccount *account,
			    gpointer   user_data)
{
	MissionControl            *mc;
	TelepathyConnectionStatus  status;
	McProfile                 *profile;
	const gchar               *protocol_name;

	mc = empathy_mission_control_new ();
	status = mission_control_get_connection_status (mc, account, NULL);
	g_object_unref (mc);
	if (status != TP_CONN_STATUS_CONNECTED) {
		/* Account is disconnected */
		return FALSE;
	}

	profile = mc_account_get_profile (account);
	protocol_name = mc_profile_get_protocol_name (profile);
	if (strcmp (protocol_name, "local-xmpp") == 0) {
		/* We can't add accounts to a XMPP LL connection
		 * FIXME: We should inspect the flags of the contact list group interface
		 */
		g_object_unref (profile);
		return FALSE;
	}

	g_object_unref (profile);
	return TRUE;
}

static void
new_contact_response_cb (GtkDialog *dialog,
			 gint       response,
			 GtkWidget *contact_widget)
{
	EmpathyContactManager *manager;
	EmpathyContact         *contact;

	manager = empathy_contact_manager_new ();
	contact = empathy_contact_widget_get_contact (contact_widget);

	if (contact && response == GTK_RESPONSE_OK) {
		empathy_contact_list_add (EMPATHY_CONTACT_LIST (manager),
					  contact,
					  _("I would like to add you to my contact list."));
	}

	new_contact_dialog = NULL;
	gtk_widget_destroy (GTK_WIDGET (dialog));
	g_object_unref (manager);
}

void
empathy_new_contact_dialog_show (GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *contact_widget;

	if (new_contact_dialog) {
		gtk_window_present (GTK_WINDOW (new_contact_dialog));
		return;
	}

	/* Create dialog */
	dialog = gtk_dialog_new ();
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_title (GTK_WINDOW (dialog), _("New contact"));

	/* Cancel button */
	button = gtk_button_new_with_label (GTK_STOCK_CANCEL);
	gtk_button_set_use_stock (GTK_BUTTON (button), TRUE);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      button,
				      GTK_RESPONSE_CANCEL);
	gtk_widget_show (button);
	
	/* Add button */
	button = gtk_button_new_with_label (GTK_STOCK_ADD);
	gtk_button_set_use_stock (GTK_BUTTON (button), TRUE);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      button,
				      GTK_RESPONSE_OK);
	gtk_widget_show (button);

	/* Contact info widget */
	contact_widget = empathy_contact_widget_new (NULL,
						     EMPATHY_CONTACT_WIDGET_EDIT_ALIAS |
						     EMPATHY_CONTACT_WIDGET_EDIT_ACCOUNT |
						     EMPATHY_CONTACT_WIDGET_EDIT_ID |
						     EMPATHY_CONTACT_WIDGET_EDIT_GROUPS);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    contact_widget,
			    TRUE, TRUE, 0);
	empathy_contact_widget_set_account_filter (contact_widget,
						   can_add_contact_to_account,
						   NULL);

	new_contact_dialog = dialog;

	g_signal_connect (dialog, "response",
			  G_CALLBACK (new_contact_response_cb),
			  contact_widget);

	if (parent) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	}

	gtk_widget_show (dialog);
}

