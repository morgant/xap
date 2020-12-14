#include <gtk/gtk.h>

extern GtkWidget *wAbout;


void
on_msave_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_mquit_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_mabout_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_btn_find_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_coe_root_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_btn_about_clicked                   (GtkButton       *button,
                                        gpointer         user_data);


void
on_ok_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancel_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_lst_result_key_press_event          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_select_all_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_m_select_all                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_m_unselect_all                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pm_delete                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_lst_result_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_coe_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_coe_pattern_realize                 (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_coe_drag_data_received              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_btn_browse_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_dir_ok_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_btn_dir_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_wdirsel_realize                     (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_btn_find_key_press                  (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_pm_opendir_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_lst_result_drag_data_get            (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_lst_result_drag_data_delete         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

void
on_lst_result_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_pm_attribute_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
