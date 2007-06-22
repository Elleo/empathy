/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2007 Imendio AB
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
 * Authors: Martyn Russell <martyn@imendio.com>
 *          Richard Hult <richard@imendio.com>
 */

#ifndef __EMPATHY_SPELL_H__
#define __EMPATHY_SPELL_H__

G_BEGIN_DECLS

gboolean     empathy_spell_supported           (void);
const gchar *empathy_spell_get_language_name   (const gchar *code);
GList       *empathy_spell_get_language_codes  (void);
void         empathy_spell_free_language_codes (GList       *codes);
gboolean     empathy_spell_check               (const gchar *word);
GList *      empathy_spell_get_suggestions     (const gchar *word);
void         empathy_spell_free_suggestions    (GList       *suggestions);

G_END_DECLS

#endif /* __EMPATHY_SPELL_H__ */
