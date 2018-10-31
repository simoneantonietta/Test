#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <time.h>
#include <dirent.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "saet.h"

#include <stdlib.h>
#include <pthread.h>

extern GtkWidget *window1;
extern GtkWidget *dialog_sens;
extern GtkWidget *dialog_att;
extern GtkWidget *dialog_zona;
extern GtkWidget *dialog_opzioni;
extern GtkWidget *dialog_tlc;
extern GtkWidget *dialog_versione;
extern GtkWidget *dialog_evento;
extern GtkWidget *dialog_segreto;
extern GtkWidget *dialog_generico;

int conn_state = 0;

void
set_bg_color(gint state)
{
  GtkWidget *w;
  
  conn_state = state;
  gdk_threads_enter ();
  w = lookup_widget(window1, "attivo");
  gtk_widget_queue_draw(w);
  gdk_threads_leave ();
}


void
print_event(gchar *event)
{
  GtkWidget *w;
  GtkTextBuffer *t;
  GtkTextIter i;
  
  gdk_threads_enter ();
  w = lookup_widget(window1, "eventi");
  t = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
  gtk_text_buffer_get_end_iter(t, &i);
  gtk_text_buffer_insert(t, &i, event, strlen(event));
  gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(w), &i, 0.0, FALSE, 0.0, 0.0);
  gdk_threads_leave ();
}


void
on_opt_lan_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *w;
  
  w = lookup_widget(dialog_opzioni, "entry_ip");
  gtk_widget_set_sensitive(w, TRUE);
  w = lookup_widget(dialog_opzioni, "entry_port");
  gtk_widget_set_sensitive(w, TRUE);
  w = lookup_widget(dialog_opzioni, "entry_device");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_baud");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_ip2");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_port2");
  gtk_widget_set_sensitive(w, FALSE);
}


void
on_opt_ser_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *w;
  
  w = lookup_widget(dialog_opzioni, "entry_ip");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_port");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_device");
  gtk_widget_set_sensitive(w, TRUE);
  w = lookup_widget(dialog_opzioni, "entry_baud");
  gtk_widget_set_sensitive(w, TRUE);
  w = lookup_widget(dialog_opzioni, "entry_ip2");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_port2");
  gtk_widget_set_sensitive(w, FALSE);
}


void
on_opt_udp_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *w;
  
  w = lookup_widget(dialog_opzioni, "entry_ip");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_port");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_device");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_baud");
  gtk_widget_set_sensitive(w, FALSE);
  w = lookup_widget(dialog_opzioni, "entry_ip2");
  gtk_widget_set_sensitive(w, TRUE);
  w = lookup_widget(dialog_opzioni, "entry_port2");
  gtk_widget_set_sensitive(w, TRUE);
}


void
on_opzioni_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
  gint ret;
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_opzioni));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_opzioni, "opt_lan");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    {
      saetparam.conn = 1;
      w = lookup_widget(dialog_opzioni, "entry_ip");
      memcpy(saetparam.data.lan.address, gtk_entry_get_text(GTK_ENTRY(w)), 16);
      w = lookup_widget(dialog_opzioni, "entry_port");
      saetparam.data.lan.port = atoi(gtk_entry_get_text(GTK_ENTRY(w)));
    }
    w = lookup_widget(dialog_opzioni, "opt_ser");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    {
      saetparam.conn = 2;
      w = lookup_widget(dialog_opzioni, "entry_device");
      memcpy(saetparam.data.ser.device, gtk_entry_get_text(GTK_ENTRY(w)), 16);
      w = lookup_widget(dialog_opzioni, "entry_baud");
      saetparam.data.ser.baud = atoi(gtk_entry_get_text(GTK_ENTRY(w)));
    }
    w = lookup_widget(dialog_opzioni, "opt_udp");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    {
      saetparam.conn = 3;
      w = lookup_widget(dialog_opzioni, "entry_ip2");
      memcpy(saetparam.data.lan.address, gtk_entry_get_text(GTK_ENTRY(w)), 16);
      w = lookup_widget(dialog_opzioni, "entry_port2");
      saetparam.data.lan.port = atoi(gtk_entry_get_text(GTK_ENTRY(w)));
    }
    w = lookup_widget(dialog_opzioni, "entry_poll");
    saetparam.polling = atoi(gtk_entry_get_text(GTK_ENTRY(w)));
    w = lookup_widget(dialog_opzioni, "debug");
    saetparam.debug = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
    saet_save_param();
  }
  gtk_widget_hide(dialog_opzioni);
}


void
on_connetti_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *w;
  gchar buf[64];
  
  if(gtk_toggle_button_get_active(togglebutton))
  {
    if(saet_connect())
    {
      gtk_button_set_label(GTK_BUTTON(togglebutton), _("Disconnetti"));
      w = lookup_widget(window1, "centrale");
      sprintf(buf, "%s:%d", saetparam.data.lan.address, saetparam.data.lan.port);
      gtk_label_set_text(GTK_LABEL(w), buf);
    }
    else
      gtk_toggle_button_set_active(togglebutton, FALSE);
  }
  else
  {
    saet_disconnect();
    gtk_button_set_label(GTK_BUTTON(togglebutton), _("Connetti"));
    w = lookup_widget(window1, "centrale");
    gtk_label_set_text(GTK_LABEL(w), "");
  }
}


void
on_add_sens_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_sens));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_sens, "entry1");
    sprintf(cmd, "D%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 5);
  }
  gtk_widget_hide(dialog_sens);
}


void
on_del_sens_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_sens));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_sens, "entry1");
    sprintf(cmd, "C%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 5);
  }
  gtk_widget_hide(dialog_sens);
}


void
on_add_zona_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_zona));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_zona, "radiobutton1");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    {
      w = lookup_widget(dialog_zona, "entry2");
      sprintf(cmd, "I%03d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
      saet_queue_cmd(cmd, 4);
    }
    else
    {
      w = lookup_widget(dialog_zona, "radiobutton2");
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
      {
        w = lookup_widget(dialog_zona, "entry2");
        sprintf(cmd, "I%03d", 216 + atoi(gtk_entry_get_text(GTK_ENTRY(w))));
        saet_queue_cmd(cmd, 4);
      }
      else
      {
        saet_queue_cmd("I000", 4);
      }
    }
  }
  gtk_widget_hide(dialog_zona);
}


void
on_del_zona_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_zona));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_zona, "radiobutton1");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    {
      w = lookup_widget(dialog_zona, "entry2");
      sprintf(cmd, "J%03d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
      saet_queue_cmd(cmd, 4);
    }
    else
    {
      w = lookup_widget(dialog_zona, "radiobutton2");
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
      {
        w = lookup_widget(dialog_zona, "entry2");
        sprintf(cmd, "J%03d", 216 + atoi(gtk_entry_get_text(GTK_ENTRY(w))));
        saet_queue_cmd(cmd, 4);
      }
      else
      {
        saet_queue_cmd("J000", 4);
      }
    }
  }
  gtk_widget_hide(dialog_zona);
}


void
on_radiobutton1_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *w;
  
  if(gtk_toggle_button_get_active(togglebutton))
  {
    w = lookup_widget(dialog_zona, "entry2");
    gtk_widget_set_sensitive(w, TRUE);
  }
}


void
on_radiobutton2_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *w;
  
  if(gtk_toggle_button_get_active(togglebutton))
  {
    w = lookup_widget(dialog_zona, "entry2");
    gtk_widget_set_sensitive(w, TRUE);
  }
}


void
on_radiobutton3_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *w;
  
  if(gtk_toggle_button_get_active(togglebutton))
  {
    w = lookup_widget(dialog_zona, "entry2");
    gtk_widget_set_sensitive(w, FALSE);
  }
}


void
on_comandi1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_sensori1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_in_servizio1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_add_sens_clicked(NULL, NULL);
}


void
on_fuori_servizio1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_del_sens_clicked(NULL, NULL);
}


void
on_accetta_allarme1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_sens));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_sens, "entry1");
    sprintf(cmd, "/381");
    *(short*)(cmd+4) = atoi(gtk_entry_get_text(GTK_ENTRY(w)));
    saet_queue_cmd(cmd, 6);
  }
  gtk_widget_hide(dialog_sens);
}


void
on_accetta_manomissione1_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_sens));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_sens, "entry1");
    sprintf(cmd, "/382");
    *(short*)(cmd+4) = atoi(gtk_entry_get_text(GTK_ENTRY(w)));
    saet_queue_cmd(cmd, 6);
  }
  gtk_widget_hide(dialog_sens);
}


void
on_accetta_guasto1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_sens));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_sens, "entry1");
    sprintf(cmd, "/383");
    *(short*)(cmd+4) = atoi(gtk_entry_get_text(GTK_ENTRY(w)));
    saet_queue_cmd(cmd, 6);
  }
  gtk_widget_hide(dialog_sens);
}


void
on_refresh1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("/35", 3);
}


void
on_attuatori1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_refresh2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("/36", 3);
}


void
on_zone1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_liste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_sensori2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("a", 1);
}


void
on_attuatori2_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("b", 1);
}


void
on_zone2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("c", 1);
}


void
on_memoria_utente1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("t", 1);
}


void
on_in_servizio2_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_att));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_att, "entry4");
    sprintf(cmd, "B%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 5);
  }
  gtk_widget_hide(dialog_att);
}


void
on_fuori_servizio2_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_att));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_att, "entry4");
    sprintf(cmd, "A%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 5);
  }
  gtk_widget_hide(dialog_att);
}


void
on_attiva1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    on_add_zona_clicked(NULL, NULL);
}


void
on_disattiva1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    on_del_zona_clicked(NULL, NULL);
}


void
on_clear_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *w;
  GtkTextBuffer *t;
  
  w = lookup_widget(window1, "eventi");
  t = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
  gtk_text_buffer_set_text(t, "", 0);
}


void
on_orologio_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  time_t t;
  struct tm *tm;
  char cmd[16];
  
  t = time(NULL);
  tm = localtime(&t);
  
  sprintf(cmd, "W%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
  saet_queue_cmd(cmd, 7);
  sprintf(cmd, "U%c%02d%02d%02d", tm->tm_wday, tm->tm_mday, tm->tm_mon+1, tm->tm_year-100);
  saet_queue_cmd(cmd, 8);
}


void
on_testbtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
#if 0
  time_t t;
  struct tm *tm;
  char cmd[16];
  
  t = time(NULL);
  tm = localtime(&t);
  
  saet_queue_cmd("H", 1);
#endif
  
#if 0
  FILE *fp;
  unsigned char cmd[136];
  int seq, len;
  
  fp = fopen("/tmp/lara.gz", "r");
  if(!fp) return;
  
  memcpy(cmd, "/ML", 3);
  seq = 0;
  
  while((len = fread(cmd+7, 1, 64, fp)) > 0)
  {
    *(unsigned short*)(cmd+3) = seq++;
    *(unsigned short*)(cmd+5) = len;
    saet_queue_cmd(cmd, 7+len);
  }
  
  *(unsigned short*)(cmd+3) = seq;
  *(unsigned short*)(cmd+5) = 0;
  saet_queue_cmd(cmd, 7);
  
  fclose(fp);
#endif
}


void
on_dialog_opzioni_show                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkWidget *w;
  gchar buf[16];
  
  switch(saetparam.conn)
  {
    case 1:
      w = lookup_widget(dialog_opzioni, "opt_lan");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      
      w = lookup_widget(dialog_opzioni, "entry_ip");
      gtk_entry_set_text(GTK_ENTRY(w), saetparam.data.lan.address);
      w = lookup_widget(dialog_opzioni, "entry_port");
      sprintf(buf, "%d", saetparam.data.lan.port);
      gtk_entry_set_text(GTK_ENTRY(w), buf);
      w = lookup_widget(dialog_opzioni, "entry_poll");
      sprintf(buf, "%d", saetparam.polling);
      gtk_entry_set_text(GTK_ENTRY(w), buf);
      break;
    case 2:
      w = lookup_widget(dialog_opzioni, "opt_ser");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      
      w = lookup_widget(dialog_opzioni, "entry_device");
      gtk_entry_set_text(GTK_ENTRY(w), saetparam.data.ser.device);
      w = lookup_widget(dialog_opzioni, "entry_baud");
      sprintf(buf, "%d", saetparam.data.ser.baud);
      gtk_entry_set_text(GTK_ENTRY(w), buf);
      w = lookup_widget(dialog_opzioni, "entry_poll");
      sprintf(buf, "%d", saetparam.polling);
      gtk_entry_set_text(GTK_ENTRY(w), buf);
      break;
    case 3:
      w = lookup_widget(dialog_opzioni, "opt_udp");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      
      w = lookup_widget(dialog_opzioni, "entry_ip2");
      gtk_entry_set_text(GTK_ENTRY(w), saetparam.data.lan.address);
      w = lookup_widget(dialog_opzioni, "entry_port2");
      sprintf(buf, "%d", saetparam.data.lan.port);
      gtk_entry_set_text(GTK_ENTRY(w), buf);
      w = lookup_widget(dialog_opzioni, "entry_poll");
      sprintf(buf, "%d", saetparam.polling);
      gtk_entry_set_text(GTK_ENTRY(w), buf);
      break;
  }
  w = lookup_widget(dialog_opzioni, "debug");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), saetparam.debug);
}


gboolean
on_attivo_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
  GdkColor c;
  GdkGC *gc;
  
  gc = gdk_gc_new(widget->window);
  
  switch(conn_state)
  {
    case 1:
      gdk_color_parse("red", &c);
      break;
    case 2:
      gdk_color_parse("green", &c);
      break;
    case 3:
      gdk_color_parse("blue", &c);
      break;
    default:
//      gdk_color_parse("light gray", &c);
      gdk_color_parse("gainsboro", &c);
      break;
  }
  gdk_colormap_alloc_color(gdk_colormap_get_system(), &c, FALSE, TRUE);
  gdk_gc_set_rgb_fg_color(gc, &c);
  gdk_draw_rectangle(widget->window, gc, 1, event->area.x, event->area.y, event->area.width, event->area.height);
  gdk_gc_unref(gc);
  
  return FALSE;
}


void
on_cancella1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *w;
  GtkTextBuffer *t;
  
  w = lookup_widget(window1, "eventi");
  t = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
  gtk_text_buffer_set_text(t, "", 0);
}


void
on_sensori3_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("d", 1);
}


void
on_attuatori3_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("e", 1);
}


void
on_giorni_festivi1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("k", 1);
}


void
on_periferiche1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("n", 1);
}


void
on_fasce_orarie1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("u", 1);
}


void
on_associazione_zone_sensori1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("w", 1);
}


void
on_telecomando_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w, *togglebutton;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_tlc));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_tlc, "entry3");
    togglebutton = lookup_widget(dialog_tlc, "radiobutton1");
    if(gtk_toggle_button_get_active(togglebutton))
      sprintf(cmd, "K%03d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    else
      sprintf(cmd, "K%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 5);
  }
  gtk_widget_hide(dialog_tlc);
}


void
on_opzioni1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_opzioni2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_opzioni_clicked(NULL, NULL);
}


void
on_versione1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_dialog_run(GTK_DIALOG(dialog_versione));
  gtk_widget_hide(dialog_versione);
}


void
on_eventi_realize                      (GtkWidget       *widget,
                                        gpointer         user_data)
{
  PangoFontDescription *font;
  
//  font = pango_font_description_from_string("courier");
  font = pango_font_description_from_string("monospace");
  gtk_widget_modify_font(widget, font);
}


void
on_memoria_utente2_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("/380", 4);
}


void
on_telecomando1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_telecomando_clicked(NULL, NULL);
}


void
on_versione_realize                    (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkWidget *w;
  
  w = lookup_widget(dialog_versione, "label_versione");
  gtk_label_set_text(GTK_LABEL(w), "v" VERSION);
}


void
on_trova1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


void
on_test1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  FILE *fp;
  char buf[256];
  int len, seq;
  
  if(conn_state)
  {
    /* programma utente */
    sprintf(buf, "%s/%s/libuser.so", saet_test_path, user_data);
    fp = fopen(buf, "r");
    if(fp)
    {
      seq = 0;
      while(len = fread(buf+7, 1, 100, fp))
      {
        memcpy(buf, "/MU", 3);
        *(unsigned short*)(buf+3) = seq++;
        *(unsigned short*)(buf+5) = len;
        saet_queue_cmd(buf, len+7);
      }
      memcpy(buf, "/MU", 3);
      *(unsigned short*)(buf+3) = seq;
      *(unsigned short*)(buf+5) = 0;
      saet_queue_cmd(buf, 7);
      fclose(fp);
    }
    /* saet.conf */
    sprintf(buf, "%s/%s/saet.conf", saet_test_path, user_data);
    fp = fopen(buf, "r");
    if(fp)
    {
      seq = 0;
      while(len = fread(buf+7, 1, 100, fp))
      {
        memcpy(buf, "/MC", 3);
        *(unsigned short*)(buf+3) = seq++;
        *(unsigned short*)(buf+5) = len;
        saet_queue_cmd(buf, len+7);
      }
      memcpy(buf, "/MC", 3);
      *(unsigned short*)(buf+3) = seq;
      *(unsigned short*)(buf+5) = 0;
      saet_queue_cmd(buf, 7);
      fclose(fp);
    }
    /* saet.nv */
    sprintf(buf, "%s/%s/saet.nv", saet_test_path, user_data);
    fp = fopen(buf, "r");
    if(fp)
    {
      seq = 0;
      while(len = fread(buf+7, 1, 100, fp))
      {
        memcpy(buf, "/MN", 3);
        *(unsigned short*)(buf+3) = seq++;
        *(unsigned short*)(buf+5) = len;
        saet_queue_cmd(buf, len+7);
      }
      memcpy(buf, "/MN", 3);
      *(unsigned short*)(buf+3) = seq;
      *(unsigned short*)(buf+5) = 0;
      saet_queue_cmd(buf, 7);
      fclose(fp);
    }
    /* consumer.conf */
    sprintf(buf, "%s/%s/consumer.conf", saet_test_path, user_data);
    fp = fopen(buf, "r");
    if(fp)
    {
      seq = 0;
      while(len = fread(buf+7, 1, 100, fp))
      {
        memcpy(buf, "/MP", 3);
        *(unsigned short*)(buf+3) = seq++;
        *(unsigned short*)(buf+5) = len;
        saet_queue_cmd(buf, len+7);
      }
      memcpy(buf, "/MP", 3);
      *(unsigned short*)(buf+3) = seq;
      *(unsigned short*)(buf+5) = 0;
      saet_queue_cmd(buf, 7);
      fclose(fp);
    }
    /* strings.conf */
    sprintf(buf, "%s/%s/strings.conf", saet_test_path, user_data);
    fp = fopen(buf, "r");
    if(fp)
    {
      seq = 0;
      while(len = fread(buf+7, 1, 100, fp))
      {
        memcpy(buf, "/MS", 3);
        *(unsigned short*)(buf+3) = seq++;
        *(unsigned short*)(buf+5) = len;
        saet_queue_cmd(buf, len+7);
      }
      memcpy(buf, "/MS", 3);
      *(unsigned short*)(buf+3) = seq;
      *(unsigned short*)(buf+5) = 0;
      saet_queue_cmd(buf, 7);
      fclose(fp);
    }
#if 0
    /* sprint.xml */
    sprintf(buf, "%s/%s/sprint.xml", saet_test_path, user_data);
    fp = fopen(buf, "r");
    if(fp)
    {
      seq = 0;
      while(len = fread(buf+7, 1, 100, fp))
      {
        memcpy(buf, "/MX", 3);
        *(unsigned short*)(buf+3) = seq++;
        *(unsigned short*)(buf+5) = len;
        saet_queue_cmd(buf, len+7);
      }
      memcpy(buf, "/MX", 3);
      *(unsigned short*)(buf+3) = seq;
      *(unsigned short*)(buf+5) = 0;
      saet_queue_cmd(buf, 7);
      fclose(fp);
    }
#endif
    /* lara.gz */
    sprintf(buf, "%s/%s/lara.gz", saet_test_path, user_data);
    fp = fopen(buf, "r");
    if(fp)
    {
      seq = 0;
      while(len = fread(buf+7, 1, 100, fp))
      {
        memcpy(buf, "/ML", 3);
        *(unsigned short*)(buf+3) = seq++;
        *(unsigned short*)(buf+5) = len;
        saet_queue_cmd(buf, len+7);
      }
      memcpy(buf, "/ML", 3);
      *(unsigned short*)(buf+3) = seq;
      *(unsigned short*)(buf+5) = 0;
      saet_queue_cmd(buf, 7);
      fclose(fp);
    }
    saet_queue_cmd("/MR", 3);
  }
}


void
on_restart1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  if(conn_state)
    saet_queue_cmd("/MR", 3);
}


void
on_eventi_populate_popup               (GtkTextView     *textview,
                                        GtkMenu         *menu,
                                        gpointer         user_data)
{
  GtkWidget *m, *i;
  DIR *dir;
  struct dirent *ent;
  static GSList *tpath = NULL;
  
  g_slist_foreach(tpath, g_free, NULL);
  g_slist_free(tpath);
  tpath = NULL;
  
  m = gtk_separator_menu_item_new();
  gtk_widget_show(m);
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), m);
  
  m = gtk_image_menu_item_new_with_label(_("Restart"));
  i = gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show(i);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(m), i);
  gtk_widget_show(m);
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), m);
  g_signal_connect ((gpointer) m, "activate",
                    G_CALLBACK (on_restart1_activate),
                    NULL);
  
  if(saet_test_path)
  {
    dir = opendir(saet_test_path);
    if(dir)
    {
      m = gtk_separator_menu_item_new();
      gtk_widget_show(m);
      gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), m);
      
      while(ent = readdir(dir))
      {
        if((ent->d_type == DT_DIR) && (ent->d_name[0] != '.'))
        {
          tpath = g_slist_prepend(tpath, strdup(ent->d_name));
          m = gtk_image_menu_item_new_with_label(tpath->data);
//          i = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
//          gtk_widget_show(i);
//          gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(m), i);
          gtk_widget_show(m);
          gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), m);
          g_signal_connect ((gpointer) m, "activate",
                    G_CALLBACK (on_test1_activate),
                    tpath->data);
        }
      }
      closedir(dir);
    }
  }

#if 0
  m = gtk_image_menu_item_new_with_label(_("Trova"));
  i = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show(i);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(m), i);
  gtk_widget_show(m);
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), m);
  g_signal_connect ((gpointer) m, "activate",
                    G_CALLBACK (on_trova1_activate),
                    NULL);
#endif
}


void
on_accettazione_totale1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("H", 1);
}


void
on_evento1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w1, *w2;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_evento));
  if(ret == GTK_RESPONSE_OK)
  {
    w1 = lookup_widget(dialog_evento, "entry5");
    w2 = lookup_widget(dialog_evento, "entry6");
    sprintf(cmd, "/53%d", atoi(gtk_entry_get_text(GTK_ENTRY(w1))));
    *(unsigned short*)(cmd+4) = atoi(gtk_entry_get_text(GTK_ENTRY(w2)));;
    saet_queue_cmd(cmd, 6);
  }
  gtk_widget_hide(dialog_evento);
}


void
on_segreto1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w1, *w2;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_segreto));
  if(ret == GTK_RESPONSE_OK)
  {
    w1 = lookup_widget(dialog_segreto, "entry7");
    w2 = lookup_widget(dialog_segreto, "entry8");
    sprintf(cmd, "/52%d", atoi(gtk_entry_get_text(GTK_ENTRY(w1))));
    *(unsigned short*)(cmd+4) = atoi(gtk_entry_get_text(GTK_ENTRY(w2)));;
    saet_queue_cmd(cmd, 6);
  }
  gtk_widget_hide(dialog_segreto);
}


void
on_attiva_con_allarme1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_zona));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_zona, "radiobutton1");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    {
      w = lookup_widget(dialog_zona, "entry2");
      sprintf(cmd, "Y1%03d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
      saet_queue_cmd(cmd, 5);
    }
    else
    {
      w = lookup_widget(dialog_zona, "radiobutton2");
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
      {
        w = lookup_widget(dialog_zona, "entry2");
        sprintf(cmd, "Y1%03d", 216 + atoi(gtk_entry_get_text(GTK_ENTRY(w))));
        saet_queue_cmd(cmd, 5);
      }
      else
      {
        saet_queue_cmd("Y1000", 5);
      }
    }
  }
  gtk_widget_hide(dialog_zona);
}


void
on_attiva_con_fuori_servizio1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[8];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_zona));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_zona, "radiobutton1");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    {
      w = lookup_widget(dialog_zona, "entry2");
      sprintf(cmd, "Y2%03d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
      saet_queue_cmd(cmd, 5);
    }
    else
    {
      w = lookup_widget(dialog_zona, "radiobutton2");
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
      {
        w = lookup_widget(dialog_zona, "entry2");
        sprintf(cmd, "Y2%03d", 216 + atoi(gtk_entry_get_text(GTK_ENTRY(w))));
        saet_queue_cmd(cmd, 5);
      }
      else
      {
        saet_queue_cmd("Y2000", 5);
      }
    }
  }
  gtk_widget_hide(dialog_zona);
}


void
on_generico1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret, i, j;
  GtkWidget *w;
  guchar *cmd, val[4];
  const gchar *entry;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_generico));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_generico, "entry9");
    entry = gtk_entry_get_text(GTK_ENTRY(w));
    ret = strlen(entry);
    cmd = (guchar*)malloc(ret);
    j = 0;
    val[2] = 0;
    for(i=0; i<ret; i++)
    {
      if(entry[i] == '#')
      {
        val[0] = entry[++i];
        val[1] = entry[++i];
        cmd[j] = strtol(val, NULL, 16);
      }
      else
        cmd[j] = entry[i];
      j++;
    }
    if(j) saet_queue_cmd(cmd, j);
    free(cmd);
  }
  gtk_widget_hide(dialog_generico);
}


void
on_on_attuatore1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[12];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_att));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_att, "entry4");
    sprintf(cmd, "/570%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 8);
  }
  gtk_widget_hide(dialog_att);
}


void
on_on_attuatore_lampeggiante1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[12];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_att));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_att, "entry4");
    sprintf(cmd, "/571%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 8);
  }
  gtk_widget_hide(dialog_att);
}


void
on_off_attuatore1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret;
  gchar cmd[12];
  GtkWidget *w;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_att));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_att, "entry4");
    sprintf(cmd, "/572%04d", atoi(gtk_entry_get_text(GTK_ENTRY(w))));
    saet_queue_cmd(cmd, 8);
  }
  gtk_widget_hide(dialog_att);
}


void
on_allineamento1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("x", 1);
}


void
on_lara1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret, i, j;
  GtkWidget *w;
  guchar *cmd, val[4];
  const gchar *entry;
  
  ret = gtk_dialog_run(GTK_DIALOG(dialog_generico));
  if(ret == GTK_RESPONSE_OK)
  {
    w = lookup_widget(dialog_generico, "entry9");
    entry = gtk_entry_get_text(GTK_ENTRY(w));
    ret = strlen(entry);
    cmd = (guchar*)malloc(ret);
    j = 0;
    val[2] = 0;
    for(i=0; i<ret; i++)
    {
      if(entry[i] == '#')
      {
        val[0] = entry[++i];
        val[1] = entry[++i];
        cmd[j] = strtol(val, NULL, 16);
      }
      else
        cmd[j] = entry[i];
      j++;
    }
    if(j) lara_queue_cmd(cmd, j);
    free(cmd);
  }
  gtk_widget_hide(dialog_generico);
}

void
on_sequenza1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint ret, i, j;
  guchar *cmd, val[4];
  gchar entry[256];
  FILE *fp;
  
  fp = fopen("comandi.txt", "r");
  if(!fp) return;
  
  while(fgets(entry, 256, fp))
  {
    ret = strlen(entry);
    cmd = (guchar*)malloc(ret);
    j = 0;
    val[2] = 0;
    for(i=0; i<ret; i++)
    {
      if(entry[i] == '#')
      {
        val[0] = entry[++i];
        val[1] = entry[++i];
        cmd[j] = strtol(val, NULL, 16);
      }
      else
        cmd[j] = entry[i];
      j++;
    }
    if(j) saet_queue_cmd(cmd, j);
    free(cmd);
  }
  fclose(fp);
}

void* comlinux_test(void *null)
{
  char cmd[8];
  int cnt;
  
  cmd[0] = '/';
  cmd[1] = 'L';
  cmd[2] = 'a';
  
  cnt = 0;
  while(1)
  {
    *(short*)(cmd+3) = cnt++;
    saet_queue_cmd(cmd, 5);
    cnt %= 20;
    sleep(1);
  }
}

void
on_avviatest_clicked                   (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
/*
  pthread_t pth;
  
  pthread_create(&pth, NULL, comlinux_test, NULL);
*/
}


void
on_radio1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_inizio_manutenzione1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("K1858", 5);
}


void
on_fine_manutenzione1_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  saet_queue_cmd("K1859", 5);
}

