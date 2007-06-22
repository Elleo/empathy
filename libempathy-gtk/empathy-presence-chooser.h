/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005-2007 Imendio AB
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
 * Authors: Richard Hult <richard@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
 */

#ifndef __EMPATHY_PRESENCE_CHOOSER_H__
#define __EMPATHY_PRESENCE_CHOOSER_H__

#include <gtk/gtk.h>

#include <libempathy/empathy-presence.h>

G_BEGIN_DECLS

#define EMPATHY_TYPE_PRESENCE_CHOOSER         (empathy_presence_chooser_get_type ())
#define EMPATHY_PRESENCE_CHOOSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_PRESENCE_CHOOSER, EmpathyPresenceChooser))
#define EMPATHY_PRESENCE_CHOOSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), EMPATHY_TYPE_PRESENCE_CHOOSER, EmpathyPresenceChooserClass))
#define EMPATHY_IS_PRESENCE_CHOOSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_PRESENCE_CHOOSER))
#define EMPATHY_IS_PRESENCE_CHOOSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EMPATHY_TYPE_PRESENCE_CHOOSER))
#define EMPATHY_PRESENCE_CHOOSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EMPATHY_TYPE_PRESENCE_CHOOSER, EmpathyPresenceChooserClass))

typedef struct _EmpathyPresenceChooser      EmpathyPresenceChooser;
typedef struct _EmpathyPresenceChooserClass EmpathyPresenceChooserClass;

struct _EmpathyPresenceChooser {
	GtkToggleButton parent;
};

struct _EmpathyPresenceChooserClass {
	GtkToggleButtonClass parent_class;
};

GType      empathy_presence_chooser_get_type           (void) G_GNUC_CONST;
GtkWidget *empathy_presence_chooser_new                (void);
GtkWidget *empathy_presence_chooser_create_menu        (void);

G_END_DECLS

#endif /* __EMPATHY_PRESENCE_CHOOSER_H__ */

