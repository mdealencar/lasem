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

#include <gdomdebug.h>
#include <gdomdocument.h>
#include <gsvgelement.h>
#include <gsvgview.h>
#include <string.h>

static GObjectClass *parent_class;

/* GDomNode implementation */

static gboolean
gsvg_element_can_append_child (GDomNode *self, GDomNode *child)
{
	return (GSVG_IS_ELEMENT (child));
}

static gboolean
gsvg_element_child_changed (GDomNode *parent, GDomNode *child)
{
	GSvgElement *element = GSVG_ELEMENT (parent);

	if (element->need_children_update)
		return FALSE;

	element->need_children_update = TRUE;

	return TRUE;
}

/* GDomElement implementation */

static void
gsvg_element_set_attribute (GDomElement *self, const char* name, const char *value)
{
	GSvgElementClass *s_element_class = GSVG_ELEMENT_GET_CLASS(self);

	gdom_debug ("[GSvgElement::set_attribute] node = %s, name = %s, value = %s",
		    gdom_node_get_node_name (GDOM_NODE (self)), name, value);

	gdom_attribute_map_set_attribute (s_element_class->attributes, self,
					  name, value);
}

const char *
gsvg_element_get_attribute (GDomElement *self, const char *name)
{
	GSvgElementClass *s_element_class = GSVG_ELEMENT_GET_CLASS(self);

	return gdom_attribute_map_get_attribute (s_element_class->attributes, self, name);
}

/* GSvgElement implementation */

void
gsvg_element_update (GSvgElement *self, const GSvgStyle *parent_style)
{
	GSvgElementClass *element_class;
	GSvgStyle *style;
	GDomDocument *document;
	GDomNode *node;

	g_return_if_fail (GSVG_IS_ELEMENT (self));
	g_return_if_fail (parent_style != NULL);

	if (!self->need_update && !self->need_children_update) {
		gdom_debug ("[Element::update] %s already up to date",
			    gdom_node_get_node_name (GDOM_NODE (self)));
		return;
	}

	document = gdom_node_get_owner_document (GDOM_NODE (self));
	if (document != NULL)
		gdom_document_register_element (document, GDOM_ELEMENT (self), self->id.value);

	element_class = GSVG_ELEMENT_GET_CLASS (self);

	style = gsvg_style_duplicate (parent_style);
	g_return_if_fail (style != NULL);

	gdom_debug ("[Element::update] update %s",
		    gdom_node_get_node_name (GDOM_NODE (self)));

	if (element_class->update != NULL)
		element_class->update (self, style);

	for (node = GDOM_NODE (self)->first_child; node != NULL; node = node->next_sibling)
		if (GSVG_IS_ELEMENT (node)) {
			if (self->need_update)
				GSVG_ELEMENT (node)->need_update = TRUE;
			gsvg_element_update (GSVG_ELEMENT (node), style);
		}

	gsvg_style_free (style);

	self->need_update = FALSE;
	self->need_children_update = FALSE;
}

/* GSvgElement implementation */

static void
_render (GSvgElement *element, GSvgView *view)
{
	GDomNode *node;

	gdom_debug ("[GSvgElement::_render");

	for (node = GDOM_NODE (element)->first_child; node != NULL; node = node->next_sibling)
		if (GSVG_IS_ELEMENT (node))
		    gsvg_element_render (GSVG_ELEMENT (node), view);
}

void
gsvg_element_render (GSvgElement *element, GSvgView *view)
{
	GSvgElementClass *element_class;

	g_return_if_fail (GSVG_IS_ELEMENT (element));

	element_class = GSVG_ELEMENT_GET_CLASS (element);
	if (element_class->render != NULL) {
		gdom_debug ("[GSvgElement::render] Render %s (%s)",
			    gdom_node_get_node_name (GDOM_NODE (element)),
			    element->id.value != NULL ? element->id.value : "no id");

		element_class->render (element, view);
	}
}

void
gsvg_element_render_paint (GSvgElement *element, GSvgView *view)
{
	GSvgElementClass *element_class;

	g_return_if_fail (GSVG_IS_ELEMENT (element));

	element_class = GSVG_ELEMENT_GET_CLASS (element);
	if (element_class->render_paint != NULL) {
		gdom_debug ("[GSvgElement::render_paint] Render %s (%s)",
			    gdom_node_get_node_name (GDOM_NODE (element)),
			    element->id.value != NULL ? element->id.value : "no id");

		element_class->render_paint (element, view);
	}
}

static void
gsvg_element_init (GSvgElement *element)
{
	element->need_update = TRUE;
}

static void
gsvg_element_finalize (GObject *object)
{
	parent_class->finalize (object);
}

/* GSvgElement class */

static void
gsvg_element_class_init (GSvgElementClass *s_element_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (s_element_class);
	GDomNodeClass *d_node_class = GDOM_NODE_CLASS (s_element_class);
	GDomElementClass *d_element_class = GDOM_ELEMENT_CLASS (s_element_class);

	parent_class = g_type_class_peek_parent (s_element_class);

	object_class->finalize = gsvg_element_finalize;

	d_node_class->can_append_child = gsvg_element_can_append_child;
	d_node_class->child_changed = gsvg_element_child_changed;

	d_element_class->get_attribute = gsvg_element_get_attribute;
	d_element_class->set_attribute = gsvg_element_set_attribute;

	s_element_class->update = NULL;
	s_element_class->render = _render;
	s_element_class->render_paint = NULL;

	s_element_class->attributes = gdom_attribute_map_new ();

	gdom_attribute_map_add_attribute (s_element_class->attributes, "id",
					  offsetof (GSvgElement, id));
}

G_DEFINE_ABSTRACT_TYPE (GSvgElement, gsvg_element, GDOM_TYPE_ELEMENT)