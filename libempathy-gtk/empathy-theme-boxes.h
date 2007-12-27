/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Imendio AB
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

#ifndef __EMPATHY_THEME_BOXES_H__
#define __EMPATHY_THEME_BOXES_H__

#include <glib-object.h>

#include "empathy-theme.h"

G_BEGIN_DECLS

#define EMPATHY_TYPE_THEME_BOXES            (empathy_theme_boxes_get_type ())
#define EMPATHY_THEME_BOXES(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_THEME_BOXES, EmpathyThemeBoxes))
#define EMPATHY_THEME_BOXES_CLASS(k)        (G_TYPE_CHECK_CLASS_CAST ((k), EMPATHY_TYPE_THEME_BOXES, EmpathyThemeBoxesClass))
#define EMPATHY_IS_THEME_BOXES(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_THEME_BOXES))
#define EMPATHY_IS_THEME_BOXES_CLASS(k)     (G_TYPE_CHECK_CLASS_TYPE ((k), EMPATHY_TYPE_THEME_BOXES))
#define EMPATHY_THEME_BOXES_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS ((o), EMPATHY_TYPE_THEME_BOXES, EmpathyThemeBoxesClass))

typedef struct _EmpathyThemeBoxes      EmpathyThemeBoxes;
typedef struct _EmpathyThemeBoxesClass EmpathyThemeBoxesClass;

struct _EmpathyThemeBoxes {
	EmpathyTheme parent;
};

struct _EmpathyThemeBoxesClass {
	EmpathyThemeClass parent_class;
};

GType empathy_theme_boxes_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __EMPATHY_THEME_BOXES_H__ */

