/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include "nactv-debug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "config.h"

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "mainwindow.h"
#include "definitions.h"
#include "net.h"

GtkBuilder *Builder = NULL;


/* GTK 4 uses "activate-link" signal on GtkAboutDialog for URL handling.
 * This handler will be connected in mainwindow.c when the about dialog is created.
 */
gboolean on_about_dialog_activate_link(GtkAboutDialog *about, 
                                        const gchar *uri, 
                                        gpointer user_data)
{
	GtkUriLauncher *launcher;
	
	/* GtkUriLauncher is the modern GTK 4 way to launch URIs */
	launcher = gtk_uri_launcher_new(uri);
	gtk_uri_launcher_launch(launcher, GTK_WINDOW(about), NULL, NULL, NULL);
	g_object_unref(launcher);
	
	return TRUE; /* Signal handled */
}


/* GTK 4 application activation callback - replaces the main UI setup */
static void on_activate(GtkApplication *app, gpointer user_data)
{
	GtkWidget *window;
	GError *error = NULL;
	
	/* Load UI with GtkBuilder (replaces libglade) */
	Builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(Builder, UIFILE, &error))
	{
		g_printerr("Error loading %s: %s\nThe application might not be correctly installed.\n", 
		           UIFILE, error ? error->message : "Unknown error");
		if (error)
			g_error_free(error);
		return;
	}
	
	/* Initialize network subsystem */
	nactv_net_init();
	
	/* Create and show main window */
	window = main_window_create();
	gtk_application_add_window(app, GTK_WINDOW(window));
	gtk_window_present(GTK_WINDOW(window));
}

/* GTK 4 application shutdown callback - handles cleanup */
static void on_shutdown(GtkApplication *app, gpointer user_data)
{
	/* Cleanup resources */
	main_window_data_cleanup();
	nactv_net_free();
	
	if (Builder)
	{
		g_object_unref(Builder);
		Builder = NULL;
	}
}

int
main (int argc, char *argv[])
{
	GtkApplication *app;
	int status;
	
	/* Internationalization setup - preserved from GTK 2 version */
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif
	
	/* Create GtkApplication (replaces gnome_program_init and gtk_init/gtk_main) */
	app = gtk_application_new("org.netactview.Netactview", 
	                          G_APPLICATION_DEFAULT_FLAGS);
	
	/* Connect application lifecycle signals */
	g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), NULL);
	
	/* Run application - handles event loop internally */
	status = g_application_run(G_APPLICATION(app), argc, argv);
	
	/* Cleanup application object */
	g_object_unref(app);
	
	return status;
}
