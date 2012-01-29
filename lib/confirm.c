#include "util.h"
#include "confirm.h"
#include <gtk/gtk.h>

typedef struct _HybridConfirmWindow HybridConfirmWindow;

struct _HybridConfirmWindow {
    GtkWidget              *window;
    GtkWidget              *image;
    GtkWidget              *entry;
    HybridAccount          *account;
    HybridConfirmOkFunc     ok_func;
    HybridConfirmCancelFunc cancel_func;
    gpointer                user_data;
};

GdkPixbuf*
hybrid_create_default_icon(gint scale_size)
{
    GdkPixbuf *pixbuf;

    if (scale_size) {
        pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPS_DIR"icons/icon.png",
                scale_size, scale_size, NULL);

    } else {
        pixbuf = gdk_pixbuf_new_from_file(PIXMAPS_DIR"icons/icon.png", NULL);
    }

    return pixbuf;
}

GtkWidget*
hybrid_create_window(const gchar *title, GdkPixbuf *icon,
                     GtkWindowPosition pos, gboolean resizable)
{
    GtkWidget *window;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    if (!icon) {
        icon = hybrid_create_default_icon(0);
    } else {
        g_object_ref(icon);
    }

    gtk_window_set_icon(GTK_WINDOW(window), icon);

    gtk_window_set_resizable(GTK_WINDOW(window), resizable);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_position(GTK_WINDOW(window), pos);
    g_object_unref(icon);

    return window;
}

GdkPixbuf*
hybrid_create_pixbuf(const guchar *pixbuf_data, gint pixbuf_len)
{
    GdkPixbufLoader *loader;
    GdkPixbuf       *pixbuf;
    guchar          *default_pixbuf_data;
    gsize            default_pixbuf_size;

    loader = gdk_pixbuf_loader_new();

    if (!pixbuf_data || pixbuf_len == 0) { /**< Load the default. */
        g_file_get_contents(PIXMAPS_DIR"icons/icon.png",
                (gchar**)&default_pixbuf_data, &default_pixbuf_size, NULL);
        gdk_pixbuf_loader_write(loader, default_pixbuf_data,
                default_pixbuf_size, NULL);
        g_free(default_pixbuf_data);

    } else {
        gdk_pixbuf_loader_write(loader, pixbuf_data, pixbuf_len, NULL);
    }
    gdk_pixbuf_loader_close(loader, NULL);

    pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    if (pixbuf) {
        g_object_ref(loader);
    }
    g_object_unref(loader);

    return pixbuf;
}


/**
 * Destroy the HybridConfirmWindow object when the window
 * was destroyed.
 */
static void
destroy_cb(GtkWidget *widget, HybridConfirmWindow *con)
{
    g_free(con);
}

static void
ok_cb(GtkWidget *widget, HybridConfirmWindow *con)
{
    const gchar *code;

    if (con->ok_func) {
        code = gtk_entry_get_text(GTK_ENTRY(con->entry));
        con->ok_func(con->account, code, con->user_data);
    }

    gtk_widget_destroy(con->window);
}

static void
cancel_cb(GtkWidget *widget, HybridConfirmWindow *con)
{
    if (con->cancel_func) {
        con->cancel_func(con->account, con->user_data);
    }
    gtk_widget_destroy(con->window);
}

void
hybrid_confirm_window_create(HybridAccount           *account,
                             guchar                  *pic_data,
                             gint                     pic_len,
                             HybridConfirmOkFunc      ok_func,
                             HybridConfirmCancelFunc  cancel_func,
                             gpointer                 user_data)
{
    HybridConfirmWindow *con;
    GtkWidget           *vbox;
    GtkWidget           *table;
    GtkWidget           *action_area;
    GtkWidget           *button;
    GtkWidget           *label;
    GdkPixbuf           *pixbuf;

    con              = g_new0(HybridConfirmWindow, 1);
    con->account     = account;
    con->ok_func     = ok_func;
    con->cancel_func = cancel_func;
    con->user_data   = user_data;

    con->window = hybrid_create_window(_("Confirm Code"), NULL,
                                       GTK_WIN_POS_CENTER, FALSE);
    gtk_widget_set_size_request(con->window, 300, 200);
    gtk_container_set_border_width(GTK_CONTAINER(con->window), 8);
    g_signal_connect(con->window, "destroy", G_CALLBACK(destroy_cb), con);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(con->window), vbox);

    table = gtk_table_new(3, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 5);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         _("<i>Please input the following code in the picture:</i>"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, 0, 1);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<i>Code:</i>"));
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    /* show code picture. */
    pixbuf     = hybrid_create_pixbuf(pic_data, pic_len);
    con->image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    gtk_table_attach_defaults(GTK_TABLE(table), con->image, 1, 2, 1, 2);

    con->entry = gtk_entry_new();
    g_signal_connect(con->entry, "activate", G_CALLBACK(ok_cb), con);
    gtk_table_attach_defaults(GTK_TABLE(table), con->entry, 0, 2, 2, 3);

    action_area = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), action_area, FALSE, FALSE, 5);

    button = gtk_button_new_with_label(_("OK"));
    gtk_widget_set_size_request(button, 100, 30);
    gtk_box_pack_end(GTK_BOX(action_area), button, FALSE, FALSE, 5);
    g_signal_connect(button, "clicked", G_CALLBACK(ok_cb), con);

    button = gtk_button_new_with_label(_("Cancel"));
    gtk_widget_set_size_request(button, 100, 30);
    gtk_box_pack_end(GTK_BOX(action_area), button, FALSE, FALSE, 5);
    g_signal_connect(button, "clicked", G_CALLBACK(cancel_cb), con);

    gtk_widget_show_all(con->window);
}
