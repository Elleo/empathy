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

#ifndef __EMPATHY_DISPATCHER_H__
#define __EMPATHY_DISPATCHER_H__

#include <glib.h>
#include <gio/gio.h>

#include <telepathy-glib/channel.h>

#include "empathy-contact.h"
#include "empathy-dispatch-operation.h"

G_BEGIN_DECLS

#define EMPATHY_TYPE_DISPATCHER         (empathy_dispatcher_get_type ())
#define EMPATHY_DISPATCHER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_DISPATCHER, EmpathyDispatcher))
#define EMPATHY_DISPATCHER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), EMPATHY_TYPE_DISPATCHER, EmpathyDispatcherClass))
#define EMPATHY_IS_DISPATCHER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_DISPATCHER))
#define EMPATHY_IS_DISPATCHER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EMPATHY_TYPE_DISPATCHER))
#define EMPATHY_DISPATCHER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EMPATHY_TYPE_DISPATCHER, EmpathyDispatcherClass))

typedef struct _EmpathyDispatcher      EmpathyDispatcher;
typedef struct _EmpathyDispatcherClass EmpathyDispatcherClass;

struct _EmpathyDispatcher {
	GObject parent;
	gpointer priv;
};

struct _EmpathyDispatcherClass {
	GObjectClass parent_class;
};

#define EMPATHY_TYPE_DISPATCHER_TUBE (empathy_dispatcher_tube_get_type ())
typedef struct {
	EmpathyContact *initiator;
	TpChannel      *channel;
	guint           id;
	gboolean        activatable;
} EmpathyDispatcherTube;


/* Will be called when the channel is ready for dispatching. The requestor
 * handle the channel itself by calling empathy_dispatch_operation_handles */
typedef void (EmpathyDispatcherRequestCb) (
  EmpathyDispatchOperation *dispatch,  const GError *error,
  gpointer user_data);

GType empathy_dispatcher_get_type             (void) G_GNUC_CONST;

void empathy_dispatcher_call_with_contact (EmpathyContact *contact,
  EmpathyDispatcherRequestCb *callback, gpointer user_data);
void empathy_dispatcher_call_with_contact_id (McAccount *account,
  const gchar           *contact_id, EmpathyDispatcherRequestCb *callback,
  gpointer user_data);

void empathy_dispatcher_chat_with_contact_id (McAccount *account,
  const gchar *contact_id, EmpathyDispatcherRequestCb *callback,
  gpointer user_data);
void  empathy_dispatcher_chat_with_contact (EmpathyContact *contact,
  EmpathyDispatcherRequestCb *callback, gpointer user_data);

/* Get the dispatcher singleton */
EmpathyDispatcher *    empathy_get_dispatcher (void);
GType                  empathy_dispatcher_tube_get_type        (void);


void  empathy_dispatcher_send_file            (EmpathyContact        *contact,
								GFile                 *gfile);

/* tube stuff */
EmpathyDispatcherTube *empathy_dispatcher_tube_ref             (EmpathyDispatcherTube *tube);
void                   empathy_dispatcher_tube_unref           (EmpathyDispatcherTube *tube);
void                   empathy_dispatcher_tube_process         (EmpathyDispatcher     *dispatcher,
								EmpathyDispatcherTube *etube);

G_END_DECLS

#endif /* __EMPATHY_DISPATCHER_H__ */
