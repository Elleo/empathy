/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Collabora Ltd.
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

#include "empathy-contact-factory.h"
#include "empathy-tp-contact-factory.h"
#include "empathy-utils.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
		       EMPATHY_TYPE_CONTACT_FACTORY, EmpathyContactFactoryPriv))

struct _EmpathyContactFactoryPriv {
	GHashTable *accounts;
};

static void empathy_contact_factory_class_init (EmpathyContactFactoryClass *klass);
static void empathy_contact_factory_init       (EmpathyContactFactory      *factory);

G_DEFINE_TYPE (EmpathyContactFactory, empathy_contact_factory, G_TYPE_OBJECT);

static EmpathyTpContactFactory *
contact_factory_get_tp_factory (EmpathyContactFactory *factory,
				McAccount             *account)
{
	EmpathyContactFactoryPriv *priv = GET_PRIV (factory);
	EmpathyTpContactFactory   *tp_factory;

	tp_factory = g_hash_table_lookup (priv->accounts, account);
	if (!tp_factory) {
		tp_factory = empathy_tp_contact_factory_new (account);
		g_hash_table_insert (priv->accounts,
				     g_object_ref (account),
				     tp_factory);
	}

	return tp_factory;
}

EmpathyContact *
empathy_contact_factory_get_user (EmpathyContactFactory *factory,
				  McAccount             *account)
{
	EmpathyTpContactFactory *tp_factory;

	tp_factory = contact_factory_get_tp_factory (factory, account);

	return empathy_tp_contact_factory_get_user (tp_factory);
}

EmpathyContact *
empathy_contact_factory_get_from_id (EmpathyContactFactory *factory,
				     McAccount             *account,
				     const gchar           *id)
{
	EmpathyTpContactFactory *tp_factory;

	tp_factory = contact_factory_get_tp_factory (factory, account);

	return empathy_tp_contact_factory_get_from_id (tp_factory, id);
}

EmpathyContact *
empathy_contact_factory_get_from_handle (EmpathyContactFactory *factory,
					 McAccount             *account,
					 guint                  handle)
{
	EmpathyTpContactFactory *tp_factory;

	tp_factory = contact_factory_get_tp_factory (factory, account);

	return empathy_tp_contact_factory_get_from_handle (tp_factory, handle);
}

GList *
empathy_contact_factory_get_from_handles (EmpathyContactFactory *factory,
					  McAccount             *account,
					  GArray                *handles)
{
	EmpathyTpContactFactory *tp_factory;

	tp_factory = contact_factory_get_tp_factory (factory, account);

	return empathy_tp_contact_factory_get_from_handles (tp_factory, handles);
}

void
empathy_contact_factory_set_alias (EmpathyContactFactory *factory,
				   EmpathyContact        *contact,
				   const gchar           *alias)
{
	EmpathyTpContactFactory *tp_factory;
	McAccount               *account;

	account = empathy_contact_get_account (contact);
	tp_factory = contact_factory_get_tp_factory (factory, account);

	return empathy_tp_contact_factory_set_alias (tp_factory, contact, alias);
}

void
empathy_contact_factory_set_avatar (EmpathyContactFactory *factory,
				    McAccount             *account,
				    const gchar           *data,
				    gsize                  size,
				    const gchar           *mime_type)
{
	EmpathyTpContactFactory *tp_factory;

	tp_factory = contact_factory_get_tp_factory (factory, account);

	return empathy_tp_contact_factory_set_avatar (tp_factory,
						      data, size, mime_type);
}

static void
contact_factory_finalize (GObject *object)
{
	EmpathyContactFactoryPriv *priv = GET_PRIV (object);

	g_hash_table_destroy (priv->accounts);

	G_OBJECT_CLASS (empathy_contact_factory_parent_class)->finalize (object);
}

static void
empathy_contact_factory_class_init (EmpathyContactFactoryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = contact_factory_finalize;

	g_type_class_add_private (object_class, sizeof (EmpathyContactFactoryPriv));
}

static void
empathy_contact_factory_init (EmpathyContactFactory *factory)
{
	EmpathyContactFactoryPriv *priv = GET_PRIV (factory);

	priv->accounts = g_hash_table_new_full (empathy_account_hash,
						empathy_account_equal,
						g_object_unref,
						g_object_unref);
}

EmpathyContactFactory *
empathy_contact_factory_new (void)
{
	static EmpathyContactFactory *factory = NULL;

	if (!factory) {
		factory = g_object_new (EMPATHY_TYPE_CONTACT_FACTORY, NULL);
		g_object_add_weak_pointer (G_OBJECT (factory), (gpointer) &factory);
	} else {
		g_object_ref (factory);
	}

	return factory;
}

