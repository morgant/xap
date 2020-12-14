#include <gtk/gtk.h>


void
on_cb_changeperms_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_skip_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_all_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_cb_changeowner_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_cb_changeperms_clicked              (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_wmain_delete_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
cb_fill_combo (GtkWidget *wmain);
void
cb_fill_wmain (GtkWidget *wmain, void *data);

void
on_wmain_realize                       (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_cb_changegroup_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_diskusage_clicked               (GtkButton       *button,
                                        gpointer         user_data);
