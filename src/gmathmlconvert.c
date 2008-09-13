/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright © 2007-2008 Emmanuel Pacaud <emmanuel@gnome.org>
 * 
 * main.c is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <gdomtext.h>
#include <gmathmloperatordictionary.h>
#include <gmathmldocument.h>
#include <gmathmlmathelement.h>
#include <gmathmlfractionelement.h>
#include <gmathmlrowelement.h>
#include <gmathmlview.h>
#include <gmathmlparser.h>
#include <glib/gmessages.h>
#include <glib/goption.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <cairo-ps.h>

static gboolean option_debug = FALSE;
static char *option_output_file_format = NULL;
static char **option_input_filenames = NULL;
static char *option_output_filename = NULL;

typedef enum {
	FORMAT_SVG,
	FORMAT_PDF,
	FORMAT_PS,
	FORMAT_UNKNOWN
} FileFormat;

static char const *file_formats[] = {
	"svg",
	"pdf",
	"ps",
	"svg"
};

static const GOptionEntry entries[] =
{
	{ G_OPTION_REMAINING,	' ', 0,	G_OPTION_ARG_FILENAME_ARRAY,
		&option_input_filenames, 	NULL, NULL},
	{ "output",		'o', 0, G_OPTION_ARG_FILENAME,
		&option_output_filename,	"Output filename", NULL},
	{ "format", 		'f', 0, G_OPTION_ARG_STRING,
		&option_output_file_format, 	"Output format", NULL },
	{ "debug", 		'd', 0, G_OPTION_ARG_NONE,
		&option_debug, 		"Debug mode", NULL },
	{ NULL }
};

int main(int argc, char **argv)
{
	GDomNode *document;
	GMathmlView *view;
	cairo_t *cairo;
	cairo_surface_t *surface;
	GError *error = NULL;
	GOptionContext *context;
	FileFormat format;
	char *output_filename;
	char *input_filename;
	double height, width;

	g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_print ("Option parsing failed: %s\n", error->message);
		return 1;
	}

	if (option_input_filenames == NULL || g_strv_length (option_input_filenames) > 1) {
		g_print ("One input file name is required\n");
		return 1;
	}

	input_filename = option_input_filenames[0];

	if (option_output_file_format != NULL) {
		for (format = 0; format < G_N_ELEMENTS(file_formats) - 1; format++)
			if (g_ascii_strcasecmp (option_output_file_format, file_formats[format]) == 0)
				break;
		if (FORMAT_UNKNOWN == format) {
			g_print ("Unknown format : %s\n", option_output_file_format);
			return 1;
		}
	} else
		format = FORMAT_UNKNOWN;

	if (option_output_filename != NULL) {
		char *extension;

		output_filename = option_output_filename;

		extension = g_strrstr (output_filename, ".");
		if (extension != NULL) {
			extension++;
			for (format = 0; format < G_N_ELEMENTS(file_formats) - 1; format++)
				if (g_ascii_strcasecmp (extension, file_formats[format]) == 0)
					break;
		}
	} else {
		char *filename;
		char *directory;
		char *extension;
		char *new_filename;

		filename = g_path_get_basename (input_filename);
		directory = g_path_get_dirname (input_filename);

		extension = g_strrstr (filename, ".");
		if (extension != NULL)
			*extension = '\0';

		new_filename = g_strconcat (filename, ".", file_formats[format], NULL);
		output_filename = g_build_filename (directory, new_filename, NULL);

		g_free (filename);
		g_free (new_filename);
		g_free (directory);
	}

	document = gmathml_document_from_file (input_filename);
	if (document == NULL) {
		g_print ("Can't load %s\n", input_filename);
		return 1;
	}

	gdom_node_dump (document);

	switch (format) {
		case FORMAT_PDF:
			surface = cairo_pdf_surface_create (output_filename, 100, 100);
			break;
		case FORMAT_PS:
			surface = cairo_ps_surface_create (output_filename, 100, 100);
			break;
		case FORMAT_SVG:
		default:
			surface = cairo_svg_surface_create (output_filename, 100, 100);
			break;
	}

	cairo = cairo_create (surface);
	cairo_surface_destroy (surface);

	view = gmathml_view_new (GMATHML_DOCUMENT (document), cairo);

	cairo_destroy (cairo);

	gmathml_view_set_debug (view, option_debug);

	gmathml_view_measure (view, &width, &height);

	switch (format) {
		case FORMAT_PDF:
			surface = cairo_pdf_surface_create (output_filename, width, height);
			break;
		case FORMAT_PS:
			surface = cairo_ps_surface_create (output_filename, width, height);
			break;
		case FORMAT_SVG:
		default:
			surface = cairo_svg_surface_create (output_filename, width, height);
			break;
	}

	cairo = cairo_create (surface);
	cairo_surface_destroy (surface);

	gmathml_view_set_cairo (view, cairo);

	cairo_destroy (cairo);

	gmathml_view_render (view);

	g_object_unref (view);

	gdom_node_dump (document);

	g_object_unref (document);

	return (0);
}
