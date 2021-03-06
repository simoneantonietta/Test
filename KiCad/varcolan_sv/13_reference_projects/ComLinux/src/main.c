/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"
#include "saet.h"

GtkWidget *window1;
GtkWidget *dialog_sens;
GtkWidget *dialog_att;
GtkWidget *dialog_zona;
GtkWidget *dialog_opzioni;
GtkWidget *dialog_tlc;
GtkWidget *dialog_versione;
GtkWidget *dialog_evento;
GtkWidget *dialog_segreto;
GtkWidget *dialog_generico;

int
main (int argc, char *argv[])
{

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  g_thread_init (NULL);
  gdk_threads_init ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  window1 = create_window1 ();
  gtk_widget_show (window1);
  dialog_sens = create_dialog_sens ();
  dialog_att = create_dialog_att ();
  dialog_zona = create_dialog_zona ();
  dialog_opzioni = create_dialog_opzioni ();
  dialog_tlc = create_dialog_tlc ();
  dialog_evento = create_dialog_evento ();
  dialog_segreto = create_dialog_segreto ();
  dialog_generico = create_dialog_generico ();
  dialog_versione = create_versione ();
  
  saet_load_param();
  
  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();
  
  return 0;
}

