#include <gtk/gtk.h>

void
set_bg_color(gint state);

void
on_opt_lan_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_opt_ser_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_opzioni_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_connetti_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_add_sens_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_del_sens_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_add_zona_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_del_zona_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_radiobutton1_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_radiobutton2_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_radiobutton3_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_esci1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_comandi1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sensori1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_in_servizio1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_fuori_servizio1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_accetta_allarme1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_accetta_manomissione1_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_accetta_guasto1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_refresh1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attuatori1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_refresh2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_zone1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_liste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sensori2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attuatori2_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_zone2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_memoria_utente1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_in_servizio2_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_fuori_servizio2_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attiva1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_disattiva1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clear_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_orologio_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_testbtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_dialog_opzioni_show                 (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_attivo_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
on_cancella1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sensori3_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attuatori3_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_giorni_festivi1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_periferiche1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_fasce_orarie1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_associazione_zone_sensori1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_telecomando_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_opzioni1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_opzioni2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_versione1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_eventi_realize                      (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_memoria_utente2_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_telecomando1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_versione_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_opt_udp_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_eventi_populate_popup               (GtkTextView     *textview,
                                        GtkMenu         *menu,
                                        gpointer         user_data);

void
on_accettazione_totale1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_evento1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_segreto1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attiva_con_allarme1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attiva_con_fuori_servizio1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_generico1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_on_attuatore1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_on_attuatore_lampeggiante1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_off_attuatore1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_allineamento1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_lara1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sequenza1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_avviatest_clicked                   (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_radio1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_inizio_manutenzione1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_fine_manutenzione1_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
