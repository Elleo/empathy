/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007-2008 Collabora Ltd.
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
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 */

#include <config.h>

#include <string.h>

#include <telepathy-glib/channel.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/util.h>

#include "empathy-tp-chat.h"
#include "empathy-contact-factory.h"
#include "empathy-contact-list.h"
#include "empathy-marshal.h"
#include "empathy-time.h"
#include "empathy-utils.h"

#define DEBUG_FLAG EMPATHY_DEBUG_TP | EMPATHY_DEBUG_CHAT
#include "empathy-debug.h"

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyTpChat)
typedef struct {
	EmpathyContactFactory *factory;
	EmpathyContact        *user;
	EmpathyContact        *remote_contact;
	EmpathyTpGroup        *group;
	McAccount             *account;
	TpChannel             *channel;
	gchar                 *id;
	gboolean               acknowledge;
	gboolean               listing_pending_messages;
	GSList                *message_queue;
	gboolean               had_properties_list;
	GPtrArray             *properties;
	gboolean               ready;
	guint                  members_count;
} EmpathyTpChatPriv;

typedef struct {
	gchar          *name;
	guint           id;
	TpPropertyFlags flags;
	GValue         *value;
} TpChatProperty;

static void tp_chat_iface_init         (EmpathyContactListIface *iface);

enum {
	PROP_0,
	PROP_CHANNEL,
	PROP_ACKNOWLEDGE,
	PROP_REMOTE_CONTACT,
	PROP_READY,
};

enum {
	MESSAGE_RECEIVED,
	SEND_ERROR,
	CHAT_STATE_CHANGED,
	PROPERTY_CHANGED,
	DESTROY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_CODE (EmpathyTpChat, empathy_tp_chat, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (EMPATHY_TYPE_CONTACT_LIST,
						tp_chat_iface_init));

static void
tp_chat_invalidated_cb (TpProxy       *proxy,
			guint          domain,
			gint           code,
			gchar         *message,
			EmpathyTpChat *chat)
{
	DEBUG ("Channel invalidated: %s", message);
	g_signal_emit (chat, signals[DESTROY], 0);
}

static void
tp_chat_async_cb (TpChannel *proxy,
		  const GError *error,
		  gpointer user_data,
		  GObject *weak_object)
{
	if (error) {
		DEBUG ("Error %s: %s", (gchar*) user_data, error->message);
	}
}

static void
tp_chat_member_added_cb (EmpathyTpGroup *group,
			 EmpathyContact *contact,
			 EmpathyContact *actor,
			 guint           reason,
			 const gchar    *message,
			 EmpathyTpChat  *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	guint              handle_type = 0;

	priv->members_count++;
	g_signal_emit_by_name (chat, "members-changed",
			       contact, actor, reason, message,
			       TRUE);

	g_object_get (priv->channel, "handle-type", &handle_type, NULL);
	if (handle_type == TP_HANDLE_TYPE_ROOM) {
		return;
	}

	if (priv->members_count > 2 && priv->remote_contact) {
		/* We now have more than 2 members, this is not a p2p chat
		 * anymore. Remove the remote-contact as it makes no sense, the
		 * EmpathyContactList interface must be used now. */
		g_object_unref (priv->remote_contact);
		priv->remote_contact = NULL;
		g_object_notify (G_OBJECT (chat), "remote-contact");
	}
	if (priv->members_count <= 2 && !priv->remote_contact &&
	    !empathy_contact_is_user (contact)) {
		/* This is a p2p chat, if it's not ourself that means this is
		 * the remote contact with who we are chatting. This is to
		 * avoid forcing the usage of the EmpathyContactList interface
		 * for p2p chats. */
		priv->remote_contact = g_object_ref (contact);
		g_object_notify (G_OBJECT (chat), "remote-contact");
	}
}

static void
tp_chat_member_removed_cb (EmpathyTpGroup *group,
			   EmpathyContact *contact,
			   EmpathyContact *actor,
			   guint           reason,
			   const gchar    *message,
			   EmpathyTpChat  *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	guint              handle_type = 0;

	priv->members_count--;
	g_signal_emit_by_name (chat, "members-changed",
			       contact, actor, reason, message,
			       FALSE);

	g_object_get (priv->channel, "handle-type", &handle_type, NULL);
	if (handle_type == TP_HANDLE_TYPE_ROOM) {
		return;
	}

	if (priv->members_count <= 2 && !priv->remote_contact) {
		GList *members, *l;

		/* We are not a MUC anymore, get the remote contact back */
		members = empathy_tp_group_get_members (group);
		for (l = members; l; l = l->next) {
			if (!empathy_contact_is_user (l->data)) {
				priv->remote_contact = g_object_ref (l->data);
				g_object_notify (G_OBJECT (chat), "remote-contact");
				break;
			}
		}
		g_list_foreach (members, (GFunc) g_object_unref, NULL);
		g_list_free (members);
	}
}

static void
tp_chat_local_pending_cb  (EmpathyTpGroup *group,
			   EmpathyContact *contact,
			   EmpathyContact *actor,
			   guint           reason,
			   const gchar    *message,
			   EmpathyTpChat  *chat)
{
	g_signal_emit_by_name (chat, "pendings-changed",
			       contact, actor, reason, message,
			       TRUE);
}

static void
tp_chat_add (EmpathyContactList *list,
	     EmpathyContact     *contact,
	     const gchar        *message)
{
	EmpathyTpChatPriv *priv = GET_PRIV (list);

	g_return_if_fail (EMPATHY_IS_TP_CHAT (list));
	g_return_if_fail (EMPATHY_IS_CONTACT (contact));

	if (priv->group) {
		empathy_tp_group_add_member (priv->group, contact, message);
	}
}

static void
tp_chat_remove (EmpathyContactList *list,
		EmpathyContact     *contact,
		const gchar        *message)
{
	EmpathyTpChatPriv *priv = GET_PRIV (list);

	g_return_if_fail (EMPATHY_IS_TP_CHAT (list));
	g_return_if_fail (EMPATHY_IS_CONTACT (contact));

	if (priv->group) {
		empathy_tp_group_remove_member (priv->group, contact, message);
	}
}

static GList *
tp_chat_get_members (EmpathyContactList *list)
{
	EmpathyTpChatPriv *priv = GET_PRIV (list);
	GList             *members = NULL;

	g_return_val_if_fail (EMPATHY_IS_TP_CHAT (list), NULL);

	if (priv->group) {
		members = empathy_tp_group_get_members (priv->group);
	} else {
		members = g_list_prepend (members, g_object_ref (priv->user));
		members = g_list_prepend (members, g_object_ref (priv->remote_contact));
	}

	return members;
}

static EmpathyMessage *
tp_chat_build_message (EmpathyTpChat *chat,
		       guint          type,
		       guint          timestamp,
		       guint          from_handle,
		       const gchar   *message_body)
{
	EmpathyTpChatPriv *priv;
	EmpathyMessage    *message;
	EmpathyContact    *sender;

	priv = GET_PRIV (chat);

	if (from_handle == 0) {
		sender = g_object_ref (priv->user);
	} else {
		sender = empathy_contact_factory_get_from_handle (priv->factory,
								  priv->account,
								  from_handle);
	}

	message = empathy_message_new (message_body);
	empathy_message_set_tptype (message, type);
	empathy_message_set_sender (message, sender);
	empathy_message_set_receiver (message, priv->user);
	empathy_message_set_timestamp (message, timestamp);

	g_object_unref (sender);

	return message;
}

static void
tp_chat_sender_ready_notify_cb (EmpathyContact *contact,
				GParamSpec     *param_spec,
				EmpathyTpChat  *chat)
{
	EmpathyTpChatPriv   *priv = GET_PRIV (chat);
	EmpathyMessage      *message;
	EmpathyContactReady  ready;
	EmpathyContact      *sender;
	gboolean             removed = FALSE;

	/* Emit all messages queued until we find a message with not
	 * ready sender. When leaving this loop, sender is the first not ready
	 * contact queued and removed tells if at least one message got removed
	 * from the queue. */
	while (priv->message_queue) {
		message = priv->message_queue->data;
		sender = empathy_message_get_sender (message);
		ready = empathy_contact_get_ready (sender);

		if (!(ready & EMPATHY_CONTACT_READY_NAME)) {
			break;
		}

		DEBUG ("Queued message ready");
		g_signal_emit (chat, signals[MESSAGE_RECEIVED], 0, message);
		priv->message_queue = g_slist_remove (priv->message_queue,
						      message);
		g_object_unref (message);
		removed = TRUE;
	}

	if (removed) {
		g_signal_handlers_disconnect_by_func (contact,
						      tp_chat_sender_ready_notify_cb,
						      chat);

		if (priv->message_queue) {
			g_signal_connect (sender, "notify::ready",
					  G_CALLBACK (tp_chat_sender_ready_notify_cb),
					  chat);
		}
	}
}

static void
tp_chat_emit_or_queue_message (EmpathyTpChat  *chat,
			       EmpathyMessage *message)
{
	EmpathyTpChatPriv   *priv = GET_PRIV (chat);
	EmpathyContact      *sender;
	EmpathyContactReady  ready;

	if (priv->message_queue != NULL) {
		DEBUG ("Message queue not empty");
		priv->message_queue = g_slist_append (priv->message_queue,
						      g_object_ref (message));
		return;
	}

	sender = empathy_message_get_sender (message);
	ready = empathy_contact_get_ready (sender);
	if (ready & EMPATHY_CONTACT_READY_NAME) {
		DEBUG ("Message queue empty and sender ready");
		g_signal_emit (chat, signals[MESSAGE_RECEIVED], 0, message);
		return;
	}

	DEBUG ("Sender not ready");
	priv->message_queue = g_slist_append (priv->message_queue, 
					      g_object_ref (message));
	g_signal_connect (sender, "notify::ready",
			  G_CALLBACK (tp_chat_sender_ready_notify_cb),
			  chat);
}

static void
tp_chat_received_cb (TpChannel   *channel,
		     guint        message_id,
		     guint        timestamp,
		     guint        from_handle,
		     guint        message_type,
		     guint        message_flags,
		     const gchar *message_body,
		     gpointer     user_data,
		     GObject     *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	EmpathyMessage    *message;

	if (priv->listing_pending_messages) {
		return;
	}
 
 	DEBUG ("Message received: %s", message_body);

	message = tp_chat_build_message (EMPATHY_TP_CHAT (chat),
					 message_type,
					 timestamp,
					 from_handle,
					 message_body);

	tp_chat_emit_or_queue_message (EMPATHY_TP_CHAT (chat), message);
	g_object_unref (message);

	if (priv->acknowledge) {
		GArray *message_ids;

		message_ids = g_array_new (FALSE, FALSE, sizeof (guint));
		g_array_append_val (message_ids, message_id);
		tp_cli_channel_type_text_call_acknowledge_pending_messages (priv->channel,
									    -1,
									    message_ids,
									    tp_chat_async_cb,
									    "acknowledging received message",
									    NULL,
									    chat);
		g_array_free (message_ids, TRUE);
	}
}

static void
tp_chat_sent_cb (TpChannel   *channel,
		 guint        timestamp,
		 guint        message_type,
		 const gchar *message_body,
		 gpointer     user_data,
		 GObject     *chat)
{
	EmpathyMessage *message;

	DEBUG ("Message sent: %s", message_body);

	message = tp_chat_build_message (EMPATHY_TP_CHAT (chat),
					 message_type,
					 timestamp,
					 0,
					 message_body);

	tp_chat_emit_or_queue_message (EMPATHY_TP_CHAT (chat), message);
	g_object_unref (message);
}

static void
tp_chat_send_error_cb (TpChannel   *channel,
		       guint        error_code,
		       guint        timestamp,
		       guint        message_type,
		       const gchar *message_body,
		       gpointer     user_data,
		       GObject     *chat)
{
	EmpathyMessage *message;

	DEBUG ("Message sent error: %s (%d)", message_body, error_code);

	message = tp_chat_build_message (EMPATHY_TP_CHAT (chat),
					 message_type,
					 timestamp,
					 0,
					 message_body);

	g_signal_emit (chat, signals[SEND_ERROR], 0, message, error_code);
	g_object_unref (message);
}

static void
tp_chat_send_cb (TpChannel    *proxy,
		 const GError *error,
		 gpointer      user_data,
		 GObject      *chat)
{
	EmpathyMessage *message = EMPATHY_MESSAGE (user_data);

	if (error) {
		DEBUG ("Error: %s", error->message);
		g_signal_emit (chat, signals[SEND_ERROR], 0, message,
			       TP_CHANNEL_TEXT_SEND_ERROR_UNKNOWN);
	}
}

static void
tp_chat_state_changed_cb (TpChannel *channel,
			  guint      handle,
			  guint      state,
			  gpointer   user_data,
			  GObject   *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	EmpathyContact    *contact;

	contact = empathy_contact_factory_get_from_handle (priv->factory,
							   priv->account,
							   handle);

	DEBUG ("Chat state changed for %s (%d): %d",
		empathy_contact_get_name (contact), handle, state);

	g_signal_emit (chat, signals[CHAT_STATE_CHANGED], 0, contact, state);
	g_object_unref (contact);
}

static void
tp_chat_list_pending_messages_cb (TpChannel       *channel,
				  const GPtrArray *messages_list,
				  const GError    *error,
				  gpointer         user_data,
				  GObject         *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	guint              i;
	GArray            *message_ids = NULL;

	priv->listing_pending_messages = FALSE;

	if (error) {
		DEBUG ("Error listing pending messages: %s", error->message);
		return;
	}

	if (priv->acknowledge) {
		message_ids = g_array_sized_new (FALSE, FALSE, sizeof (guint),
						 messages_list->len);
	}

	for (i = 0; i < messages_list->len; i++) {
		EmpathyMessage *message;
		GValueArray    *message_struct;
		const gchar    *message_body;
		guint           message_id;
		guint           timestamp;
		guint           from_handle;
		guint           message_type;
		guint           message_flags;

		message_struct = g_ptr_array_index (messages_list, i);

		message_id = g_value_get_uint (g_value_array_get_nth (message_struct, 0));
		timestamp = g_value_get_uint (g_value_array_get_nth (message_struct, 1));
		from_handle = g_value_get_uint (g_value_array_get_nth (message_struct, 2));
		message_type = g_value_get_uint (g_value_array_get_nth (message_struct, 3));
		message_flags = g_value_get_uint (g_value_array_get_nth (message_struct, 4));
		message_body = g_value_get_string (g_value_array_get_nth (message_struct, 5));

		DEBUG ("Message pending: %s", message_body);

		if (message_ids) {
			g_array_append_val (message_ids, message_id);
		}

		message = tp_chat_build_message (EMPATHY_TP_CHAT (chat),
						 message_type,
						 timestamp,
						 from_handle,
						 message_body);

		tp_chat_emit_or_queue_message (EMPATHY_TP_CHAT (chat), message);
		g_object_unref (message);
	}

	if (message_ids) {
		tp_cli_channel_type_text_call_acknowledge_pending_messages (priv->channel,
									    -1,
									    message_ids,
									    tp_chat_async_cb,
									    "acknowledging pending messages",
									    NULL,
									    chat);
		g_array_free (message_ids, TRUE);
	}
}

static void
tp_chat_property_flags_changed_cb (TpProxy         *proxy,
				   const GPtrArray *properties,
				   gpointer         user_data,
				   GObject         *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	guint              i, j;

	if (!priv->had_properties_list || !properties) {
		return;
	}

	for (i = 0; i < properties->len; i++) {
		GValueArray    *prop_struct;
		TpChatProperty *property;
		guint           id;
		guint           flags;

		prop_struct = g_ptr_array_index (properties, i);
		id = g_value_get_uint (g_value_array_get_nth (prop_struct, 0));
		flags = g_value_get_uint (g_value_array_get_nth (prop_struct, 1));

		for (j = 0; j < priv->properties->len; j++) {
			property = g_ptr_array_index (priv->properties, j);
			if (property->id == id) {
				property->flags = flags;
				DEBUG ("property %s flags changed: %d",
					property->name, property->flags);
				break;
			}
		}
	}
}

static void
tp_chat_properties_changed_cb (TpProxy         *proxy,
			       const GPtrArray *properties,
			       gpointer         user_data,
			       GObject         *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	guint              i, j;

	if (!priv->had_properties_list || !properties) {
		return;
	}

	for (i = 0; i < properties->len; i++) {
		GValueArray    *prop_struct;
		TpChatProperty *property;
		guint           id;
		GValue         *src_value;

		prop_struct = g_ptr_array_index (properties, i);
		id = g_value_get_uint (g_value_array_get_nth (prop_struct, 0));
		src_value = g_value_get_boxed (g_value_array_get_nth (prop_struct, 1));

		for (j = 0; j < priv->properties->len; j++) {
			property = g_ptr_array_index (priv->properties, j);
			if (property->id == id) {
				if (property->value) {
					g_value_copy (src_value, property->value);
				} else {
					property->value = tp_g_value_slice_dup (src_value);
				}

				DEBUG ("property %s changed", property->name);
				g_signal_emit (chat, signals[PROPERTY_CHANGED], 0,
					       property->name, property->value);
				break;
			}
		}
	}
}

static void
tp_chat_get_properties_cb (TpProxy         *proxy,
			   const GPtrArray *properties,
			   const GError    *error,
			   gpointer         user_data,
			   GObject         *chat)
{
	if (error) {
		DEBUG ("Error getting properties: %s", error->message);
		return;
	}

	tp_chat_properties_changed_cb (proxy, properties, user_data, chat);
}

static void
tp_chat_list_properties_cb (TpProxy         *proxy,
			    const GPtrArray *properties,
			    const GError    *error,
			    gpointer         user_data,
			    GObject         *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	GArray            *ids;
	guint              i;

	priv->had_properties_list = TRUE;

	if (error) {
		DEBUG ("Error listing properties: %s", error->message);
		return;
	}

	ids = g_array_sized_new (FALSE, FALSE, sizeof (guint), properties->len);
	priv->properties = g_ptr_array_sized_new (properties->len);
	for (i = 0; i < properties->len; i++) {
		GValueArray    *prop_struct;
		TpChatProperty *property;

		prop_struct = g_ptr_array_index (properties, i);
		property = g_slice_new0 (TpChatProperty);
		property->id = g_value_get_uint (g_value_array_get_nth (prop_struct, 0));
		property->name = g_value_dup_string (g_value_array_get_nth (prop_struct, 1));
		property->flags = g_value_get_uint (g_value_array_get_nth (prop_struct, 3));

		DEBUG ("Adding property name=%s id=%d flags=%d",
			property->name, property->id, property->flags);
		g_ptr_array_add (priv->properties, property);
		if (property->flags & TP_PROPERTY_FLAG_READ) {
			g_array_append_val (ids, property->id);
		}
	}

	tp_cli_properties_interface_call_get_properties (proxy, -1,
							 ids,
							 tp_chat_get_properties_cb,
							 NULL, NULL,
							 chat);

	g_array_free (ids, TRUE);
}

void
empathy_tp_chat_set_property (EmpathyTpChat *chat,
			      const gchar   *name,
			      const GValue  *value)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	TpChatProperty    *property;
	guint              i;

	g_return_if_fail (priv->ready);

	for (i = 0; i < priv->properties->len; i++) {
		property = g_ptr_array_index (priv->properties, i);
		if (!tp_strdiff (property->name, name)) {
			GPtrArray   *properties;
			GValueArray *prop;
			GValue       id = {0, };
			GValue       dest_value = {0, };

			if (!(property->flags & TP_PROPERTY_FLAG_WRITE)) {
				break;
			}

			g_value_init (&id, G_TYPE_UINT);
			g_value_init (&dest_value, G_TYPE_VALUE);
			g_value_set_uint (&id, property->id);
			g_value_set_boxed (&dest_value, value);

			prop = g_value_array_new (2);
			g_value_array_append (prop, &id);
			g_value_array_append (prop, &dest_value);

			properties = g_ptr_array_sized_new (1);
			g_ptr_array_add (properties, prop);

			DEBUG ("Set property %s", name);
			tp_cli_properties_interface_call_set_properties (priv->channel, -1,
									 properties,
									 (tp_cli_properties_interface_callback_for_set_properties)
									 tp_chat_async_cb,
									 "Seting property", NULL,
									 G_OBJECT (chat));

			g_ptr_array_free (properties, TRUE);
			g_value_array_free (prop);

			break;
		}
	}
}

static void
tp_chat_channel_ready_cb (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);
	TpConnection      *connection;
	guint              handle, handle_type;

	DEBUG ("Channel ready");

	g_object_get (priv->channel,
		      "connection", &connection,
		      "handle", &handle,
		      "handle_type", &handle_type,
		      NULL);

	if (handle_type != TP_HANDLE_TYPE_NONE && handle != 0) {
		GArray *handles;
		gchar **names;

		handles = g_array_new (FALSE, FALSE, sizeof (guint));
		g_array_append_val (handles, handle);
		tp_cli_connection_run_inspect_handles (connection, -1,
						       handle_type, handles,
						       &names, NULL, NULL);
		priv->id = *names;
		g_array_free (handles, TRUE);
		g_free (names);
	}

	if (handle_type == TP_HANDLE_TYPE_CONTACT && handle != 0) {
		priv->remote_contact = empathy_contact_factory_get_from_handle (priv->factory,
										priv->account,
										handle);
		g_object_notify (G_OBJECT (chat), "remote-contact");
	}

	if (tp_proxy_has_interface_by_id (priv->channel,
					  TP_IFACE_QUARK_CHANNEL_INTERFACE_GROUP)) {
		priv->group = empathy_tp_group_new (priv->channel);

		g_signal_connect (priv->group, "member-added",
				  G_CALLBACK (tp_chat_member_added_cb),
				  chat);
		g_signal_connect (priv->group, "member-removed",
				  G_CALLBACK (tp_chat_member_removed_cb),
				  chat);
		g_signal_connect (priv->group, "local-pending",
				  G_CALLBACK (tp_chat_local_pending_cb),
				  chat);
		empathy_run_until_ready (priv->group);
	} else {
		priv->members_count = 2;
	}
	
	if (tp_proxy_has_interface_by_id (priv->channel,
					  TP_IFACE_QUARK_PROPERTIES_INTERFACE)) {
		tp_cli_properties_interface_call_list_properties (priv->channel, -1,
								  tp_chat_list_properties_cb,
								  NULL, NULL,
								  G_OBJECT (chat));
		tp_cli_properties_interface_connect_to_properties_changed (priv->channel,
									   tp_chat_properties_changed_cb,
									   NULL, NULL,
									   G_OBJECT (chat), NULL);
		tp_cli_properties_interface_connect_to_property_flags_changed (priv->channel,
									       tp_chat_property_flags_changed_cb,
									       NULL, NULL,
									       G_OBJECT (chat), NULL);
	}

	priv->listing_pending_messages = TRUE;
	tp_cli_channel_type_text_call_list_pending_messages (priv->channel, -1,
							     FALSE,
							     tp_chat_list_pending_messages_cb,
							     NULL, NULL,
							     G_OBJECT (chat));

	tp_cli_channel_type_text_connect_to_received (priv->channel,
						      tp_chat_received_cb,
						      NULL, NULL,
						      G_OBJECT (chat), NULL);
	tp_cli_channel_type_text_connect_to_sent (priv->channel,
						  tp_chat_sent_cb,
						  NULL, NULL,
						  G_OBJECT (chat), NULL);
	tp_cli_channel_type_text_connect_to_send_error (priv->channel,
							tp_chat_send_error_cb,
							NULL, NULL,
							G_OBJECT (chat), NULL);
	tp_cli_channel_interface_chat_state_connect_to_chat_state_changed (priv->channel,
									   tp_chat_state_changed_cb,
									   NULL, NULL,
									   G_OBJECT (chat), NULL);
	tp_cli_channel_interface_chat_state_connect_to_chat_state_changed (priv->channel,
									   tp_chat_state_changed_cb,
									   NULL, NULL,
									   G_OBJECT (chat), NULL);

	priv->ready = TRUE;
	g_object_notify (G_OBJECT (chat), "ready");
}

static void
tp_chat_finalize (GObject *object)
{
	EmpathyTpChatPriv *priv = GET_PRIV (object);
	guint              i;

	DEBUG ("Finalize: %p", object);

	if (priv->acknowledge && priv->channel) {
		DEBUG ("Closing channel...");
		tp_cli_channel_call_close (priv->channel, -1,
					   tp_chat_async_cb,
					   "closing channel", NULL,
					   NULL);
	}

	if (priv->channel) {
		g_signal_handlers_disconnect_by_func (priv->channel,
						      tp_chat_invalidated_cb,
						      object);
		g_object_unref (priv->channel);
	}

	if (priv->properties) {
		for (i = 0; i < priv->properties->len; i++) {
			TpChatProperty *property;

			property = g_ptr_array_index (priv->properties, i);
			g_free (property->name);
			if (property->value) {
				tp_g_value_slice_free (property->value);
			}
			g_slice_free (TpChatProperty, property);
		}
		g_ptr_array_free (priv->properties, TRUE);
	}

	if (priv->remote_contact) {
		g_object_unref (priv->remote_contact);
	}
	if (priv->group) {
		g_object_unref (priv->group);
	}

	g_object_unref (priv->factory);
	g_object_unref (priv->user);
	g_object_unref (priv->account);
	g_free (priv->id);

	if (priv->message_queue) {
		EmpathyMessage *message;
		EmpathyContact *contact;

		message = priv->message_queue->data;
		contact = empathy_message_get_sender (message);
		g_signal_handlers_disconnect_by_func (contact,
						      tp_chat_sender_ready_notify_cb,
						      object);
	}
	g_slist_foreach (priv->message_queue, (GFunc) g_object_unref, NULL);
	g_slist_free (priv->message_queue);

	G_OBJECT_CLASS (empathy_tp_chat_parent_class)->finalize (object);
}

static GObject *
tp_chat_constructor (GType                  type,
		     guint                  n_props,
		     GObjectConstructParam *props)
{
	GObject           *chat;
	EmpathyTpChatPriv *priv;
	gboolean           channel_ready;

	chat = G_OBJECT_CLASS (empathy_tp_chat_parent_class)->constructor (type, n_props, props);

	priv = GET_PRIV (chat);
	priv->account = empathy_channel_get_account (priv->channel);
	priv->factory = empathy_contact_factory_new ();
	priv->user = empathy_contact_factory_get_user (priv->factory, priv->account);

	g_signal_connect (priv->channel, "invalidated",
			  G_CALLBACK (tp_chat_invalidated_cb),
			  chat);

	g_object_get (priv->channel, "channel-ready", &channel_ready, NULL);
	if (channel_ready) {
		tp_chat_channel_ready_cb (EMPATHY_TP_CHAT (chat));
	} else {
		g_signal_connect_swapped (priv->channel, "notify::channel-ready",
					  G_CALLBACK (tp_chat_channel_ready_cb),
					  chat);
	}

	return chat;
}

static void
tp_chat_get_property (GObject    *object,
		      guint       param_id,
		      GValue     *value,
		      GParamSpec *pspec)
{
	EmpathyTpChatPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_CHANNEL:
		g_value_set_object (value, priv->channel);
		break;
	case PROP_ACKNOWLEDGE:
		g_value_set_boolean (value, priv->acknowledge);
		break;
	case PROP_REMOTE_CONTACT:
		g_value_set_object (value, priv->remote_contact);
		break;
	case PROP_READY:
		g_value_set_boolean (value, priv->ready);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
tp_chat_set_property (GObject      *object,
		      guint         param_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
	EmpathyTpChatPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_CHANNEL:
		priv->channel = g_object_ref (g_value_get_object (value));
		break;
	case PROP_ACKNOWLEDGE:
		priv->acknowledge = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
empathy_tp_chat_class_init (EmpathyTpChatClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tp_chat_finalize;
	object_class->constructor = tp_chat_constructor;
	object_class->get_property = tp_chat_get_property;
	object_class->set_property = tp_chat_set_property;

	g_object_class_install_property (object_class,
					 PROP_CHANNEL,
					 g_param_spec_object ("channel",
							      "telepathy channel",
							      "The text channel for the chat",
							      TP_TYPE_CHANNEL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_ACKNOWLEDGE,
					 g_param_spec_boolean ("acknowledge",
							       "acknowledge messages",
							       "Wheter or not received messages should be acknowledged",
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_REMOTE_CONTACT,
					 g_param_spec_object ("remote-contact",
							      "The remote contact",
							      "The remote contact if there is no group iface on the channel",
							      EMPATHY_TYPE_CONTACT,
							      G_PARAM_READABLE));
	g_object_class_install_property (object_class,
					 PROP_READY,
					 g_param_spec_boolean ("ready",
							       "Is the object ready",
							       "This object can't be used until this becomes true",
							       FALSE,
							       G_PARAM_READABLE));

	/* Signals */
	signals[MESSAGE_RECEIVED] =
		g_signal_new ("message-received",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1, EMPATHY_TYPE_MESSAGE);

	signals[SEND_ERROR] =
		g_signal_new ("send-error",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      _empathy_marshal_VOID__OBJECT_UINT,
			      G_TYPE_NONE,
			      2, EMPATHY_TYPE_MESSAGE, G_TYPE_UINT);

	signals[CHAT_STATE_CHANGED] =
		g_signal_new ("chat-state-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      _empathy_marshal_VOID__OBJECT_UINT,
			      G_TYPE_NONE,
			      2, EMPATHY_TYPE_CONTACT, G_TYPE_UINT);

	signals[PROPERTY_CHANGED] =
		g_signal_new ("property-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      _empathy_marshal_VOID__STRING_BOXED,
			      G_TYPE_NONE,
			      2, G_TYPE_STRING, G_TYPE_VALUE);

	signals[DESTROY] =
		g_signal_new ("destroy",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	g_type_class_add_private (object_class, sizeof (EmpathyTpChatPriv));
}

static void
empathy_tp_chat_init (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (chat,
		EMPATHY_TYPE_TP_CHAT, EmpathyTpChatPriv);

	chat->priv = priv;
}

static void
tp_chat_iface_init (EmpathyContactListIface *iface)
{
	iface->add         = tp_chat_add;
	iface->remove      = tp_chat_remove;
	iface->get_members = tp_chat_get_members;
}

EmpathyTpChat *
empathy_tp_chat_new (TpChannel *channel)
{
	return g_object_new (EMPATHY_TYPE_TP_CHAT, 
			     "channel", channel,
			     NULL);
}

const gchar *
empathy_tp_chat_get_id (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_val_if_fail (EMPATHY_IS_TP_CHAT (chat), NULL);
	g_return_val_if_fail (priv->ready, NULL);

	return priv->id;
}

EmpathyContact *
empathy_tp_chat_get_remote_contact (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_val_if_fail (EMPATHY_IS_TP_CHAT (chat), NULL);

	return priv->remote_contact;
}

McAccount *
empathy_tp_chat_get_account (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_val_if_fail (EMPATHY_IS_TP_CHAT (chat), FALSE);

	return priv->account;
}

TpChannel *
empathy_tp_chat_get_channel (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_val_if_fail (EMPATHY_IS_TP_CHAT (chat), NULL);

	return priv->channel;
}

gboolean
empathy_tp_chat_is_ready (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_val_if_fail (EMPATHY_IS_TP_CHAT (chat), FALSE);

	return priv->ready;
}

guint
empathy_tp_chat_get_members_count (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_val_if_fail (EMPATHY_IS_TP_CHAT (chat), 0);

	return priv->members_count;
}

void
empathy_tp_chat_set_acknowledge (EmpathyTpChat *chat,
				  gboolean      acknowledge)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_if_fail (EMPATHY_IS_TP_CHAT (chat));

	priv->acknowledge = acknowledge;
	g_object_notify (G_OBJECT (chat), "acknowledge");
}

void
empathy_tp_chat_emit_pendings (EmpathyTpChat *chat)
{
	EmpathyTpChatPriv  *priv = GET_PRIV (chat);

	g_return_if_fail (EMPATHY_IS_TP_CHAT (chat));
	g_return_if_fail (priv->ready);

	if (priv->listing_pending_messages) {
		return;
	}

	priv->listing_pending_messages = TRUE;
	tp_cli_channel_type_text_call_list_pending_messages (priv->channel, -1,
							     FALSE,
							     tp_chat_list_pending_messages_cb,
							     NULL, NULL,
							     G_OBJECT (chat));
}

void
empathy_tp_chat_send (EmpathyTpChat *chat,
		      EmpathyMessage *message)
{
	EmpathyTpChatPriv        *priv = GET_PRIV (chat);
	const gchar              *message_body;
	TpChannelTextMessageType  message_type;

	g_return_if_fail (EMPATHY_IS_TP_CHAT (chat));
	g_return_if_fail (EMPATHY_IS_MESSAGE (message));
	g_return_if_fail (priv->ready);

	message_body = empathy_message_get_body (message);
	message_type = empathy_message_get_tptype (message);

	DEBUG ("Sending message: %s", message_body);
	tp_cli_channel_type_text_call_send (priv->channel, -1,
					    message_type,
					    message_body,
					    tp_chat_send_cb,
					    g_object_ref (message),
					    (GDestroyNotify) g_object_unref,
					    G_OBJECT (chat));
}

void
empathy_tp_chat_set_state (EmpathyTpChat      *chat,
			   TpChannelChatState  state)
{
	EmpathyTpChatPriv *priv = GET_PRIV (chat);

	g_return_if_fail (EMPATHY_IS_TP_CHAT (chat));
	g_return_if_fail (priv->ready);

	DEBUG ("Set state: %d", state);
	tp_cli_channel_interface_chat_state_call_set_chat_state (priv->channel, -1,
								 state,
								 tp_chat_async_cb,
								 "setting chat state",
								 NULL,
								 G_OBJECT (chat));
}

