/*
 * Copyright © 2009 Emmanuel Pacaud
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <lsmdebug.h>
#include <lsmdomdocument.h>
#include <lsmdomview.h>

static GObjectClass *parent_class;

void
lsm_dom_view_measure (LsmDomView *view, double *width, double *height)
{
	LsmDomViewClass *view_class;

	if (width != NULL)
		*width = 0.0;
	if (height != NULL)
		*height = 0.0;

	g_return_if_fail (LSM_DOM_IS_VIEW (view));

	view_class = LSM_DOM_VIEW_GET_CLASS (view);
	if (view_class->measure != NULL)
		view_class->measure (view, width, height);
}

void
lsm_dom_view_render (LsmDomView *view, double x, double y)
{
	LsmDomViewClass *view_class;

	g_return_if_fail (LSM_DOM_IS_VIEW (view));
	g_return_if_fail (view->cairo != NULL);

	view_class = LSM_DOM_VIEW_GET_CLASS (view);
	if (view_class->measure != NULL)
		view_class->render (view, x, y);
}

void
lsm_dom_view_set_debug (LsmDomView *view, gboolean debug)
{
	g_return_if_fail (LSM_DOM_IS_VIEW (view));

	view->debug = debug;
}

void
lsm_dom_view_set_cairo (LsmDomView *view, cairo_t *cairo)
{
	PangoContext *context;
	PangoFontDescription *font_description;
	cairo_font_options_t *font_options;
	const cairo_font_options_t *current_font_options;
	cairo_surface_t *surface;
	cairo_surface_type_t type;

	g_return_if_fail (LSM_DOM_IS_VIEW (view));

	if (view->cairo == cairo)
		return;

	if (view->cairo != NULL) {
		cairo_destroy (view->cairo);
		g_object_unref (view->pango_layout);
	}

	if (cairo == NULL) {
		view->cairo = NULL;
		view->pango_layout = NULL;

		return;
	}

	font_description = view->font_description;

	cairo_reference (cairo);
	view->cairo = cairo;
	view->pango_layout = pango_cairo_create_layout (cairo);

	surface = cairo_get_target (cairo);

	type = cairo_surface_get_type (surface);

	view->is_vector = (type == CAIRO_SURFACE_TYPE_SVG ||
			   type == CAIRO_SURFACE_TYPE_PDF ||
			   type == CAIRO_SURFACE_TYPE_PS);

	context = pango_layout_get_context (view->pango_layout);
	pango_cairo_context_set_resolution (context, 72);

	current_font_options = pango_cairo_context_get_font_options (context);
	if (current_font_options == NULL)
		font_options = cairo_font_options_create ();
	else
		font_options = cairo_font_options_copy (current_font_options);
	cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_OFF);
	pango_cairo_context_set_font_options (context, font_options);
	cairo_font_options_destroy (font_options);
}

void
lsm_dom_view_set_document (LsmDomView *view, LsmDomDocument *document)
{
	g_return_if_fail (LSM_DOM_IS_VIEW (view));
	g_return_if_fail (document == NULL || LSM_DOM_IS_DOCUMENT (document));

	if (view->document == document)
		return;

	if (view->document != NULL)
		g_object_unref (view->document);

	if (document != NULL)
	    g_object_ref (document);

	view->document = document;
}

static void
lsm_dom_view_init (LsmDomView *view)
{
	PangoFontMap *font_map;
	PangoContext *pango_context;
	cairo_font_options_t *font_options;

	view->font_description = pango_font_description_new ();

	font_map = pango_cairo_font_map_get_default ();

#if PANGO_VERSION_CHECK(1,22,0)
	pango_context = pango_font_map_create_context (font_map);
#else
	pango_context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (font_map));
#endif
	pango_cairo_context_set_resolution (pango_context, 72.0);

	view->measure_pango_layout = pango_layout_new (pango_context);

	font_options = cairo_font_options_create ();

	cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_OFF);
	cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);

	pango_cairo_context_set_font_options (pango_context, font_options);

	cairo_font_options_destroy (font_options);

	g_object_unref (pango_context);

	view->pango_layout = NULL;
	view->cairo = NULL;
	view->is_vector = FALSE;
}

static void
lsm_dom_view_finalize (GObject *object)
{
	LsmDomView *view = LSM_DOM_VIEW (object);

	g_object_unref (view->document);

	if (view->pango_layout != NULL)
		g_object_unref (view->pango_layout);
	if (view->cairo != NULL)
		cairo_destroy (view->cairo);

	g_object_unref (view->measure_pango_layout);

	pango_font_description_free (view->font_description);

	parent_class->finalize (object);
}

static void
lsm_dom_view_class_init (LsmDomViewClass *view_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (view_class);

	parent_class = g_type_class_peek_parent (view_class);

	object_class->finalize = lsm_dom_view_finalize;
}

G_DEFINE_ABSTRACT_TYPE (LsmDomView, lsm_dom_view, G_TYPE_OBJECT)