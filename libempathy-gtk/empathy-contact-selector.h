/*
*  Copyright (C) 2007 Marco Barisione <marco@barisione.org>
*  Copyright (C) 2008 Collabora Ltd.
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*
*  Authors: Elliot Fairweather <elliot.fairweather@collabora.co.uk>
*           Marco Barisione <marco@barisione.org>
*/

#ifndef __EMPATHY_CONTACT_SELECTOR_H__
#define __EMPATHY_CONTACT_SELECTOR_H__

G_BEGIN_DECLS

#include <glib-object.h>
#include <gtk/gtk.h>

#include <libempathy/empathy-contact.h>

#define EMPATHY_TYPE_CONTACT_SELECTOR (empathy_contact_selector_get_type ())
#define EMPATHY_CONTACT_SELECTOR(object) (G_TYPE_CHECK_INSTANCE_CAST \
        ((object), EMPATHY_TYPE_CONTACT_SELECTOR, EmpathyContactSelector))
#define EMPATHY_CONTACT_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
        EMPATHY_TYPE_CONTACT_SELECTOR, EmpathyContactSelectorClass))
#define EMPATHY_IS_CONTACT_SELECTOR(object) (G_TYPE_CHECK_INSTANCE_TYPE \
    ((object), EMPATHY_TYPE_CONTACT_SELECTOR))
#define EMPATHY_IS_CONTACT_SELECTOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), EMPATHY_TYPE_CONTACT_SELECTOR))
#define EMPATHY_CONTACT_SELECTOR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS \
    ((object), EMPATHY_TYPE_CONTACT_SELECTOR, EmpathyContactSelectorClass))

typedef struct _EmpathyContactSelector EmpathyContactSelector;
typedef struct _EmpathyContactSelectorClass EmpathyContactSelectorClass;

struct _EmpathyContactSelector
{
  GtkComboBox parent;
};

struct _EmpathyContactSelectorClass
{
  GtkComboBoxClass parent_class;
};

GType empathy_contact_selector_get_type (void) G_GNUC_CONST;
EmpathyContactSelector *
empathy_contact_selector_new (EmpathyContactListStore *store);

EmpathyContact *
empathy_contact_selector_get_selected (EmpathyContactSelector *selector);

G_END_DECLS

#endif /* __EMPATHY_CONTACT_SELECTOR_H__ */
