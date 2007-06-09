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

#ifndef __EMPATHY_CONTACT_DIALOGS_H__
#define __EMPATHY_CONTACT_DIALOGS_H__

#include <gtk/gtk.h>

#include <libempathy/gossip-contact.h>

G_BEGIN_DECLS

void empathy_subscription_dialog_show        (GossipContact *contact,
				              GtkWindow     *parent);
void empathy_contact_information_dialog_show (GossipContact *contact,
					      GtkWindow     *parent,
					      gboolean       edit);
void empathy_new_contact_dialog_show         (GtkWindow     *parent);

G_END_DECLS

#endif /*  __EMPATHY_CONTACT_DIALOGS_H__ */
