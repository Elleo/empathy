/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006-2007 Imendio AB
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
 * Authors: Kristian Rietveld <kris@imendio.com>
 */

#ifndef __EMPATHY_CELL_RENDERER_EXPANDER_H__
#define __EMPATHY_CELL_RENDERER_EXPANDER_H__

#include <gtk/gtkcellrenderer.h>

G_BEGIN_DECLS

#define EMPATHY_TYPE_CELL_RENDERER_EXPANDER		(empathy_cell_renderer_expander_get_type ())
#define EMPATHY_CELL_RENDERER_EXPANDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), EMPATHY_TYPE_CELL_RENDERER_EXPANDER, EmpathyCellRendererExpander))
#define EMPATHY_CELL_RENDERER_EXPANDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), EMPATHY_TYPE_CELL_RENDERER_EXPANDER, EmpathyCellRendererExpanderClass))
#define EMPATHY_IS_CELL_RENDERER_EXPANDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EMPATHY_TYPE_CELL_RENDERER_EXPANDER))
#define EMPATHY_IS_CELL_RENDERER_EXPANDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), EMPATHY_TYPE_CELL_RENDERER_EXPANDER))
#define EMPATHY_CELL_RENDERER_EXPANDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), EMPATHY_TYPE_CELL_RENDERER_EXPANDER, EmpathyCellRendererExpanderClass))

typedef struct _EmpathyCellRendererExpander EmpathyCellRendererExpander;
typedef struct _EmpathyCellRendererExpanderClass EmpathyCellRendererExpanderClass;

struct _EmpathyCellRendererExpander {
  GtkCellRenderer parent;
};

struct _EmpathyCellRendererExpanderClass {
  GtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType            empathy_cell_renderer_expander_get_type (void) G_GNUC_CONST;
GtkCellRenderer *empathy_cell_renderer_expander_new      (void);

G_END_DECLS

#endif /* __EMPATHY_CELL_RENDERER_EXPANDER_H__ */
