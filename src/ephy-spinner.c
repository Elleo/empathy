/* 
 * Copyright © 2000 Eazel, Inc.
 * Copyright © 2002-2004 Marco Pesenti Gritti
 * Copyright © 2004, 2006 Christian Persch
 *
 * Nautilus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Nautilus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 *
 * Ephy port by Marco Pesenti Gritti <marco@it.gnome.org>
 * 
 * $Id: ephy-spinner.c 2114 2006-12-25 12:15:00Z mr $
 */

#include <config.h>

#include "ephy-spinner.h"

/* #include "ephy-debug.h" */
#define LOG(msg, args...)
#define START_PROFILER(name)
#define STOP_PROFILER(name)

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtksettings.h>

/* Spinner cache implementation */

#define EPHY_TYPE_SPINNER_CACHE			(ephy_spinner_cache_get_type())
#define EPHY_SPINNER_CACHE(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), EPHY_TYPE_SPINNER_CACHE, EphySpinnerCache))
#define EPHY_SPINNER_CACHE_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST((klass), EPHY_TYPE_SPINNER_CACHE, EphySpinnerCacheClass))
#define EPHY_IS_SPINNER_CACHE(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), EPHY_TYPE_SPINNER_CACHE))
#define EPHY_IS_SPINNER_CACHE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), EPHY_TYPE_SPINNER_CACHE))
#define EPHY_SPINNER_CACHE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), EPHY_TYPE_SPINNER_CACHE, EphySpinnerCacheClass))

typedef struct _EphySpinnerCache	EphySpinnerCache;
typedef struct _EphySpinnerCacheClass	EphySpinnerCacheClass;
typedef struct _EphySpinnerCachePrivate	EphySpinnerCachePrivate;

struct _EphySpinnerCacheClass
{
	GObjectClass parent_class;
};

struct _EphySpinnerCache
{
	GObject parent_object;

	/*< private >*/
	EphySpinnerCachePrivate *priv;
};

#define EPHY_SPINNER_CACHE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), EPHY_TYPE_SPINNER_CACHE, EphySpinnerCachePrivate))

struct _EphySpinnerCachePrivate
{
	/* Hash table of GdkScreen -> EphySpinnerCacheData */
	GHashTable *hash;
};

typedef struct
{
	guint ref_count;
	GtkIconSize size;
	int width;
	int height;
	GdkPixbuf **animation_pixbufs;
	guint n_animation_pixbufs;
} EphySpinnerImages;

#define LAST_ICON_SIZE			GTK_ICON_SIZE_DIALOG + 1
#define SPINNER_ICON_NAME		"process-working"
#define SPINNER_FALLBACK_ICON_NAME	"gnome-spinner"
#define EPHY_SPINNER_IMAGES_INVALID	((EphySpinnerImages *) 0x1)

typedef struct
{
	GdkScreen *screen;
	GtkIconTheme *icon_theme;
	EphySpinnerImages *images[LAST_ICON_SIZE];
} EphySpinnerCacheData;

static void ephy_spinner_cache_class_init (EphySpinnerCacheClass *klass);
static void ephy_spinner_cache_init	  (EphySpinnerCache *cache);

static GObjectClass *ephy_spinner_cache_parent_class;

static GType
ephy_spinner_cache_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		const GTypeInfo our_info =
		{
			sizeof (EphySpinnerCacheClass),
			NULL,
			NULL,
			(GClassInitFunc) ephy_spinner_cache_class_init,
			NULL,
			NULL,
			sizeof (EphySpinnerCache),
			0,
			(GInstanceInitFunc) ephy_spinner_cache_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "EphySpinnerCache",
					       &our_info, 0);
	}

	return type;
}

static EphySpinnerImages *
ephy_spinner_images_ref (EphySpinnerImages *images)
{
	g_return_val_if_fail (images != NULL, NULL);

	images->ref_count++;

	return images;
}

static void
ephy_spinner_images_unref (EphySpinnerImages *images)
{
	g_return_if_fail (images != NULL);

	images->ref_count--;
	if (images->ref_count == 0)
	{
		guint i;

		LOG ("Freeing spinner images %p for size %d", images, images->size);

		for (i = 0; i < images->n_animation_pixbufs; ++i)
		{
			g_object_unref (images->animation_pixbufs[i]);
		}
		g_free (images->animation_pixbufs);

		g_free (images);
	}
}

static void
ephy_spinner_cache_data_unload (EphySpinnerCacheData *data)
{
	GtkIconSize size;
	EphySpinnerImages *images;

	g_return_if_fail (data != NULL);

	LOG ("EphySpinnerDataCache unload for screen %p", data->screen);

	for (size = GTK_ICON_SIZE_INVALID; size < LAST_ICON_SIZE; ++size)
	{
		images = data->images[size];
		data->images[size] = NULL;

		if (images != NULL && images != EPHY_SPINNER_IMAGES_INVALID)
		{
			ephy_spinner_images_unref (images);
		}
	}
}

static GdkPixbuf *
extract_frame (GdkPixbuf *grid_pixbuf,
	       int x,
	       int y,
	       int size)
{
	GdkPixbuf *pixbuf;

	if (x + size > gdk_pixbuf_get_width (grid_pixbuf) ||
	    y + size > gdk_pixbuf_get_height (grid_pixbuf))
	{
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_subpixbuf (grid_pixbuf,
					   x, y,
					   size, size);
	g_return_val_if_fail (pixbuf != NULL, NULL);

	return pixbuf;
}

static GdkPixbuf *
scale_to_size (GdkPixbuf *pixbuf,
	       int dw,
	       int dh)
{
	GdkPixbuf *result;
	int pw, ph;

	g_return_val_if_fail (pixbuf != NULL, NULL);

	pw = gdk_pixbuf_get_width (pixbuf);
	ph = gdk_pixbuf_get_height (pixbuf);

	if (pw != dw || ph != dh)
	{
		result = gdk_pixbuf_scale_simple (pixbuf, dw, dh,
						  GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		return result;
	}

	return pixbuf;
}

static EphySpinnerImages *
ephy_spinner_images_load (GdkScreen *screen,
			  GtkIconTheme *icon_theme,
			  GtkIconSize icon_size)
{
	EphySpinnerImages *images;
	GdkPixbuf *icon_pixbuf, *pixbuf;
	GtkIconInfo *icon_info = NULL;
	int grid_width, grid_height, x, y, requested_size, size, isw, ish, n;
	const char *icon;
	GSList *list = NULL, *l;

	LOG ("EphySpinnerCacheData loading for screen %p at size %d", screen, icon_size);

	START_PROFILER ("loading spinner animation")

	if (!gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
						icon_size, &isw, &ish)) goto loser;

	requested_size = MAX (ish, isw);

	/* Load the animation. The 'rest icon' is the 0th frame */
	icon_info = gtk_icon_theme_lookup_icon (icon_theme,
						SPINNER_ICON_NAME,
					        requested_size, 0);
	if (icon_info == NULL)
	{
		g_warning ("Throbber animation not found");
	
		/* If the icon naming spec compliant name wasn't found, try the old name */
		icon_info = gtk_icon_theme_lookup_icon (icon_theme,
							SPINNER_FALLBACK_ICON_NAME,
							requested_size, 0);
		if (icon_info == NULL)
		{
			g_warning ("Throbber fallback animation not found either");
			goto loser;
		}
	}
	g_assert (icon_info != NULL);

	size = gtk_icon_info_get_base_size (icon_info);
	icon = gtk_icon_info_get_filename (icon_info);
	if (icon == NULL) goto loser;

	icon_pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
	gtk_icon_info_free (icon_info);
	icon_info = NULL;

	if (icon_pixbuf == NULL)
	{
		g_warning ("Could not load the spinner file");
		goto loser;
	}

	grid_width = gdk_pixbuf_get_width (icon_pixbuf);
	grid_height = gdk_pixbuf_get_height (icon_pixbuf);

	n = 0;
	for (y = 0; y < grid_height; y += size)
	{
		for (x = 0; x < grid_width ; x += size)
		{
			pixbuf = extract_frame (icon_pixbuf, x, y, size);

			if (pixbuf)
			{
				list = g_slist_prepend (list, pixbuf);
				++n;
			}
			else
			{
				g_warning ("Cannot extract frame (%d, %d) from the grid\n", x, y);
			}
		}
	}

	g_object_unref (icon_pixbuf);

	if (list == NULL) goto loser;
	g_assert (n > 0);

	if (size > requested_size)
	{
		for (l = list; l != NULL; l = l->next)
		{
			l->data = scale_to_size (l->data, isw, ish);
		}
	}

	/* Now we've successfully got all the data */
	images = g_new (EphySpinnerImages, 1);
	images->ref_count = 1;

	images->size = icon_size;
	images->width = images->height = requested_size;

	images->n_animation_pixbufs = n;
	images->animation_pixbufs = g_new (GdkPixbuf *, n);

	for (l = list; l != NULL; l = l->next)
	{
		g_assert (l->data != NULL);
		images->animation_pixbufs[--n] = l->data;
	}
	g_assert (n == 0);

	g_slist_free (list);

	STOP_PROFILER ("loading spinner animation")

	return images;

loser:
	if (icon_info)
	{
		gtk_icon_info_free (icon_info);
	}
	g_slist_foreach (list, (GFunc) g_object_unref, NULL);

	STOP_PROFILER ("loading spinner animation")

	return NULL;
}

static EphySpinnerCacheData *
ephy_spinner_cache_data_new (GdkScreen *screen)
{
	EphySpinnerCacheData *data;

	data = g_new0 (EphySpinnerCacheData, 1);

	data->screen = screen;
	data->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect_swapped (data->icon_theme, "changed",
				  G_CALLBACK (ephy_spinner_cache_data_unload),
				  data);

	return data;
}

static void
ephy_spinner_cache_data_free (EphySpinnerCacheData *data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (data->icon_theme != NULL);

	g_signal_handlers_disconnect_by_func
		(data->icon_theme,
		 G_CALLBACK (ephy_spinner_cache_data_unload), data);

	ephy_spinner_cache_data_unload (data);

	g_free (data);
}

static EphySpinnerImages *
ephy_spinner_cache_get_images (EphySpinnerCache *cache,
			       GdkScreen *screen,
			       GtkIconSize icon_size)
{
	EphySpinnerCachePrivate *priv = cache->priv;
	EphySpinnerCacheData *data;
	EphySpinnerImages *images;

	LOG ("Getting animation images for screen %p at size %d", screen, icon_size);

	g_return_val_if_fail (icon_size >= 0 && icon_size < LAST_ICON_SIZE, NULL);

	/* Backward compat: "invalid" meant "native" size which doesn't exist anymore */
	if (icon_size == GTK_ICON_SIZE_INVALID)
	{
		icon_size = GTK_ICON_SIZE_DIALOG;
	}

	data = g_hash_table_lookup (priv->hash, screen);
	if (data == NULL)
	{
		data = ephy_spinner_cache_data_new (screen);
		/* FIXME: think about what happens when the screen's display is closed later on */
		g_hash_table_insert (priv->hash, screen, data);
	}

	images = data->images[icon_size];
	if (images == EPHY_SPINNER_IMAGES_INVALID)
	{
		/* Load failed, but don't try endlessly again! */
		return NULL;
	}

	if (images != NULL)
	{
		/* Return cached data */
		return ephy_spinner_images_ref (images);
	}

	images = ephy_spinner_images_load (screen, data->icon_theme, icon_size);

	if (images == NULL)
	{
		/* Mark as failed-to-load */
		data->images[icon_size] = EPHY_SPINNER_IMAGES_INVALID;

		return NULL;
	}

	data->images[icon_size] = images;

	return ephy_spinner_images_ref (images);
}

static void
ephy_spinner_cache_init (EphySpinnerCache *cache)
{
	EphySpinnerCachePrivate *priv;

	priv = cache->priv = EPHY_SPINNER_CACHE_GET_PRIVATE (cache);

	LOG ("EphySpinnerCache initialising");

	priv->hash = g_hash_table_new_full (g_direct_hash, g_direct_equal,
					    NULL,
					    (GDestroyNotify) ephy_spinner_cache_data_free);
}

static void
ephy_spinner_cache_finalize (GObject *object)
{
	EphySpinnerCache *cache = EPHY_SPINNER_CACHE (object); 
	EphySpinnerCachePrivate *priv = cache->priv;

	g_hash_table_destroy (priv->hash);

	LOG ("EphySpinnerCache finalised");

	G_OBJECT_CLASS (ephy_spinner_cache_parent_class)->finalize (object);
}

static void
ephy_spinner_cache_class_init (EphySpinnerCacheClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	ephy_spinner_cache_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = ephy_spinner_cache_finalize;

	g_type_class_add_private (object_class, sizeof (EphySpinnerCachePrivate));
}

static EphySpinnerCache *spinner_cache = NULL;

static EphySpinnerCache *
ephy_spinner_cache_ref (void)
{
	if (spinner_cache == NULL)
	{
		EphySpinnerCache **cache_ptr;

		spinner_cache = g_object_new (EPHY_TYPE_SPINNER_CACHE, NULL);
		cache_ptr = &spinner_cache;
		g_object_add_weak_pointer (G_OBJECT (spinner_cache),
					   (gpointer *) cache_ptr);

		return spinner_cache;
	}
		
	return g_object_ref (spinner_cache);
}

/* Spinner implementation */

#define SPINNER_TIMEOUT 125 /* ms */

#define EPHY_SPINNER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), EPHY_TYPE_SPINNER, EphySpinnerDetails))

struct _EphySpinnerDetails
{
	GtkIconTheme *icon_theme;
	EphySpinnerCache *cache;
	GtkIconSize size;
	EphySpinnerImages *images;
	guint current_image;
	guint timeout;
	guint timer_task;
	guint spinning : 1;
	guint need_load : 1;
};

static void ephy_spinner_class_init (EphySpinnerClass *class);
static void ephy_spinner_init	    (EphySpinner *spinner);

static GObjectClass *parent_class;

GType
ephy_spinner_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		const GTypeInfo our_info =
		{
			sizeof (EphySpinnerClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) ephy_spinner_class_init,
			NULL,
			NULL, /* class_data */
			sizeof (EphySpinner),
			0, /* n_preallocs */
			(GInstanceInitFunc) ephy_spinner_init
		};

		type = g_type_register_static (GTK_TYPE_WIDGET,
					       "EphySpinner",
					       &our_info, 0);
	}

	return type;
}

static gboolean
ephy_spinner_load_images (EphySpinner *spinner)
{
	EphySpinnerDetails *details = spinner->details;

	if (details->need_load)
	{
		START_PROFILER ("ephy_spinner_load_images")

		details->images =
			ephy_spinner_cache_get_images
				(details->cache,
				 gtk_widget_get_screen (GTK_WIDGET (spinner)),
				 details->size);

		STOP_PROFILER ("ephy_spinner_load_images")

		details->current_image = 0; /* 'rest' icon */
		details->need_load = FALSE;
	}

	return details->images != NULL;
}

static void
ephy_spinner_unload_images (EphySpinner *spinner)
{
	EphySpinnerDetails *details = spinner->details;

	if (details->images != NULL)
	{
		ephy_spinner_images_unref (details->images);
		details->images = NULL;
	}

	details->current_image = 0;
	details->need_load = TRUE;
}

static void
icon_theme_changed_cb (GtkIconTheme *icon_theme,
		       EphySpinner *spinner)
{
	ephy_spinner_unload_images (spinner);
	gtk_widget_queue_resize (GTK_WIDGET (spinner));
}

static void
ephy_spinner_init (EphySpinner *spinner)
{
	EphySpinnerDetails *details;

	details = spinner->details = EPHY_SPINNER_GET_PRIVATE (spinner);

	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (spinner), GTK_NO_WINDOW);

	details->cache = ephy_spinner_cache_ref ();
	details->size = GTK_ICON_SIZE_DIALOG;
	details->spinning = FALSE;
	details->timeout = SPINNER_TIMEOUT;
	details->need_load = TRUE;
}

static int
ephy_spinner_expose (GtkWidget *widget,
		     GdkEventExpose *event)
{
	EphySpinner *spinner = EPHY_SPINNER (widget);
	EphySpinnerDetails *details = spinner->details;
	EphySpinnerImages *images;
	GdkPixbuf *pixbuf;
	GdkGC *gc;
	int x_offset, y_offset, width, height;
	GdkRectangle pix_area, dest;

	if (!GTK_WIDGET_DRAWABLE (spinner))
	{
		return FALSE;
	}

	if (details->need_load &&
	    !ephy_spinner_load_images (spinner))
	{
		return FALSE;
	}

	images = details->images;
	if (images == NULL)
	{
		return FALSE;
	}

	/* Otherwise |images| will be NULL anyway */
	g_assert (images->n_animation_pixbufs > 0);
		
	g_assert (details->current_image >= 0 &&
		  details->current_image < images->n_animation_pixbufs);

	pixbuf = images->animation_pixbufs[details->current_image];

	g_assert (pixbuf != NULL);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	/* Compute the offsets for the image centered on our allocation */
	x_offset = (widget->allocation.width - width) / 2;
	y_offset = (widget->allocation.height - height) / 2;

	pix_area.x = x_offset + widget->allocation.x;
	pix_area.y = y_offset + widget->allocation.y;
	pix_area.width = width;
	pix_area.height = height;

	if (!gdk_rectangle_intersect (&event->area, &pix_area, &dest))
	{
		return FALSE;
	}

	gc = gdk_gc_new (widget->window);
	gdk_draw_pixbuf (widget->window, gc, pixbuf,
			 dest.x - x_offset - widget->allocation.x,
			 dest.y - y_offset - widget->allocation.y,
			 dest.x, dest.y,
			 dest.width, dest.height,
			 GDK_RGB_DITHER_MAX, 0, 0);
	g_object_unref (gc);

	return FALSE;
}

static gboolean
bump_spinner_frame_cb (EphySpinner *spinner)
{
	EphySpinnerDetails *details = spinner->details;

	/* This can happen when we've unloaded the images on a theme
	 * change, but haven't been in the queued size request yet.
	 * Just skip this update.
	 */
	if (details->images == NULL) return TRUE;

	details->current_image++;
	if (details->current_image >= details->images->n_animation_pixbufs)
	{
		/* the 0th frame is the 'rest' icon */
		details->current_image = MIN (1, details->images->n_animation_pixbufs);
	}

	gtk_widget_queue_draw (GTK_WIDGET (spinner));

	/* run again */
	return TRUE;
}

/**
 * ephy_spinner_start:
 * @spinner: a #EphySpinner
 *
 * Start the spinner animation.
 **/
void
ephy_spinner_start (EphySpinner *spinner)
{
	EphySpinnerDetails *details = spinner->details;

	details->spinning = TRUE;

	if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)) &&
	    details->timer_task == 0 &&
	    ephy_spinner_load_images (spinner))
	{
		/* the 0th frame is the 'rest' icon */
		details->current_image = MIN (1, details->images->n_animation_pixbufs);

		details->timer_task =
			g_timeout_add_full (G_PRIORITY_LOW,
			                    details->timeout,
				            (GSourceFunc) bump_spinner_frame_cb,
				            spinner,
				            NULL);
	}
}

static void
ephy_spinner_remove_update_callback (EphySpinner *spinner)
{
	EphySpinnerDetails *details = spinner->details;

	if (details->timer_task != 0)
	{
		g_source_remove (details->timer_task);
		details->timer_task = 0;
	}
}

/**
 * ephy_spinner_stop:
 * @spinner: a #EphySpinner
 *
 * Stop the spinner animation.
 **/
void
ephy_spinner_stop (EphySpinner *spinner)
{
	EphySpinnerDetails *details = spinner->details;

	details->spinning = FALSE;
	details->current_image = 0;

	if (details->timer_task != 0)
	{
		ephy_spinner_remove_update_callback (spinner);

		if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)))
		{
			gtk_widget_queue_draw (GTK_WIDGET (spinner));
		}
	}
}

/*
 * ephy_spinner_set_size:
 * @spinner: a #EphySpinner
 * @size: the size of type %GtkIconSize
 *
 * Set the size of the spinner.
 **/
void
ephy_spinner_set_size (EphySpinner *spinner,
		       GtkIconSize size)
{
	if (size == GTK_ICON_SIZE_INVALID)
	{
		size = GTK_ICON_SIZE_DIALOG;
	}

	if (size != spinner->details->size)
	{
		ephy_spinner_unload_images (spinner);

		spinner->details->size = size;

		gtk_widget_queue_resize (GTK_WIDGET (spinner));
	}
}

#if 0
/*
 * ephy_spinner_set_timeout:
 * @spinner: a #EphySpinner
 * @timeout: time delay between updates to the spinner.
 *
 * Sets the timeout delay for spinner updates.
 **/
void
ephy_spinner_set_timeout (EphySpinner *spinner,
			  guint timeout)
{
	EphySpinnerDetails *details = spinner->details;

	if (timeout != details->timeout)
	{
		ephy_spinner_stop (spinner);

		details->timeout = timeout;

		if (details->spinning)
		{
			ephy_spinner_start (spinner);
		}
	}
}
#endif

static void
ephy_spinner_size_request (GtkWidget *widget,
			   GtkRequisition *requisition)
{
	EphySpinner *spinner = EPHY_SPINNER (widget);
	EphySpinnerDetails *details = spinner->details;

	if ((details->need_load &&
	     !ephy_spinner_load_images (spinner)) ||
            details->images == NULL)
	{
		requisition->width = requisition->height = 0;
		gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
						   details->size,
						   &requisition->width,
					           &requisition->height);
		return;
	}

	requisition->width = details->images->width;
	requisition->height = details->images->height;

	/* FIXME fix this hack */
	/* allocate some extra margin so we don't butt up against toolbar edges */
	if (details->size != GTK_ICON_SIZE_MENU)
	{
		requisition->width += 2;
		requisition->height += 2;
	}
}

static void
ephy_spinner_map (GtkWidget *widget)
{
	EphySpinner *spinner = EPHY_SPINNER (widget);
	EphySpinnerDetails *details = spinner->details;

	GTK_WIDGET_CLASS (parent_class)->map (widget);

	if (details->spinning)
	{
		ephy_spinner_start (spinner);
	}
}

static void
ephy_spinner_unmap (GtkWidget *widget)
{
	EphySpinner *spinner = EPHY_SPINNER (widget);

	ephy_spinner_remove_update_callback (spinner);

	GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ephy_spinner_dispose (GObject *object)
{
	EphySpinner *spinner = EPHY_SPINNER (object);

	g_signal_handlers_disconnect_by_func
			(spinner->details->icon_theme,
		 G_CALLBACK (icon_theme_changed_cb), spinner);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ephy_spinner_finalize (GObject *object)
{
	EphySpinner *spinner = EPHY_SPINNER (object);

	ephy_spinner_remove_update_callback (spinner);
	ephy_spinner_unload_images (spinner);

	g_object_unref (spinner->details->cache);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ephy_spinner_screen_changed (GtkWidget *widget,
			     GdkScreen *old_screen)
{
	EphySpinner *spinner = EPHY_SPINNER (widget);
	EphySpinnerDetails *details = spinner->details;
	GdkScreen *screen;

	if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
	{
		GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, old_screen);
	}

	screen = gtk_widget_get_screen (widget);

	/* FIXME: this seems to be happening when then spinner is destroyed!? */
	if (old_screen == screen) return;

	/* We'll get mapped again on the new screen, but not unmapped from
	 * the old screen, so remove timeout here.
	 */
	ephy_spinner_remove_update_callback (spinner);

	ephy_spinner_unload_images (spinner);

	if (old_screen != NULL)
	{
		g_signal_handlers_disconnect_by_func
			(gtk_icon_theme_get_for_screen (old_screen),
			 G_CALLBACK (icon_theme_changed_cb), spinner);
	}

	details->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect (details->icon_theme, "changed",
			  G_CALLBACK (icon_theme_changed_cb), spinner);
}

static void
ephy_spinner_class_init (EphySpinnerClass *class)
{
	GObjectClass *object_class =  G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->dispose = ephy_spinner_dispose;
	object_class->finalize = ephy_spinner_finalize;

	widget_class->expose_event = ephy_spinner_expose;
	widget_class->size_request = ephy_spinner_size_request;
	widget_class->map = ephy_spinner_map;
	widget_class->unmap = ephy_spinner_unmap;
	widget_class->screen_changed = ephy_spinner_screen_changed;

	g_type_class_add_private (object_class, sizeof (EphySpinnerDetails));
}

/*
 * ephy_spinner_new:
 *
 * Create a new #EphySpinner. The spinner is a widget
 * that gives the user feedback about network status with
 * an animated image.
 *
 * Return Value: the spinner #GtkWidget
 **/
GtkWidget *
ephy_spinner_new (void)
{
	return GTK_WIDGET (g_object_new (EPHY_TYPE_SPINNER, NULL));
}
