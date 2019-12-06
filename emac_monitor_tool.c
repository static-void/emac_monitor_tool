/*
Copyright (c) 2019, Ed Robbins
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the organization nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct i2c_handles {
    char name[30];
    uint8_t addr;
    uint8_t reg;
    uint8_t min;
    uint8_t max;
    uint8_t val;
    uint8_t init_val;
} controls[] = {
    { "Contrast",                   0x46, 0x00, 0x00, 0xff, 0x00, 0xff },
    { "Brightness",                 0x46, 0x11, 0x00, 0x3f, 0x00, 0x1e },
    { "Gamma?",                     0x46, 0x10, 0x00, 0xff, 0x00, 0x88 },
    { "Horizontal size",            0x46, 0x0d, 0x80, 0xff, 0x00, 0x96 },
    { "Horizontal size 2?",         0x46, 0x12, 0x00, 0x7f, 0x00, 0x46 },
    { "Horizontal position",        0x46, 0x07, 0x80, 0xff, 0x00, 0xcd },
    { "Vertical size",              0x46, 0x08, 0x80, 0xff, 0x00, 0xb6 },
    { "Vertical size 2?",           0x46, 0x14, 0x00, 0x3f, 0x00, 0x52 },
    { "Vertical position",          0x46, 0x09, 0x80, 0xff, 0x00, 0xba },
    { "Blue",                       0x46, 0x01, 0x00, 0xff, 0x00, 0x8c },
    { "Green",                      0x46, 0x02, 0x00, 0xff, 0x00, 0xaa },
    { "Red",                        0x46, 0x03, 0x00, 0xff, 0x00, 0xaa },
    { "Top pinch",                  0x46, 0x04, 0x80, 0xff, 0x00, 0xc4 },
    { "Top lean",                   0x46, 0x05, 0x80, 0xff, 0x00, 0xc4 },
    { "Bottom lean",                0x46, 0x06, 0x00, 0xff, 0x00, 0xc4 },
    { "Shape/sphere",               0x46, 0x0a, 0x80, 0xff, 0x00, 0x80 },
    { "Keystone",                   0x46, 0x0b, 0x80, 0xff, 0x00, 0xc0 },
    { "Pincushion",                 0x46, 0x0c, 0x80, 0xff, 0x00, 0xdb },
    { "Top/bottom pull left/right", 0x46, 0x0e, 0x80, 0xff, 0x00, 0xb6 },
    { "Paralellogram",              0x46, 0x0f, 0x80, 0xff, 0x00, 0xc0 },
    { "Bottom pinch",               0x46, 0x15, 0x00, 0x7f, 0x00, 0x3f },
    { "Colour temp blue?",          0x4c, 0x00, 0x00, 0xff, 0x00, 0x96 },
    { "Colour temp yellow?",        0x4c, 0x01, 0x00, 0xff, 0x00, 0x96 },
    { "Colour temp magenta?",       0x4c, 0x02, 0x00, 0xff, 0x00, 0x96 },
    { "Rotation",                   0x4c, 0x03, 0x00, 0xff, 0x00, 0xad },
};
int dev;
GtkWidget *sliders[ARRAY_SIZE(controls)];

static uint8_t
set_slave_addr(uint8_t addr) {
    if (ioctl(dev, I2C_SLAVE, addr) < 0) {
        fprintf(stderr, "Error: could not set slave address to 0x%x\n", addr);
        return 0;
    }
    return 1;
}

static void
set_value(GtkRange *slider, uint32_t control_index) {
    uint32_t i = control_index;
    controls[i].val = (uint8_t)gtk_range_get_value(slider);
    if (!set_slave_addr(controls[i].addr)) goto end;
    if (i2c_smbus_write_byte_data(dev, controls[i].reg, controls[i].val) < 0) {
        fprintf(stderr, "Error: could not write to register 0x%x\n", controls[i].reg);
    }
end:
    return;
}

static void
get_value(uint32_t control_index) {
    uint32_t i = control_index;
    int32_t result;
    if (!set_slave_addr(controls[i].addr)) goto end;
    if ((result = i2c_smbus_read_byte_data(dev, controls[i].reg)) < 0) {
        fprintf(stderr, "Error: could not read register 0x%x\n", controls[i].reg);
    }
    else controls[i].val = (uint8_t)result;
end:
    return;
}

static void
revert(GtkWidget *widget, GtkWidget *sliders[]) {
    for (int i = 0; i < ARRAY_SIZE(controls); ++i) {
        gtk_range_set_value(GTK_RANGE(sliders[i]), (gdouble)controls[i].init_val);
        set_value(GTK_RANGE(sliders[i]), i);
    }
}

static void
activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *labels[ARRAY_SIZE(controls)];
    GtkWidget *grid;
    GtkWidget *revert_button;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "eMac Monitor Tool");
    gtk_container_set_border_width(GTK_CONTAINER(window), 5);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    for (int i = 0; i < ARRAY_SIZE(controls); ++i) {
        /* It turns out we can't read the values. So I have
           tried to get fairly normal initial values. They're
           still a bit off but at least the display comes back
           when you revert */
        /*get_value(i);*/
        controls[i].val = controls[i].init_val;

        labels[i] = gtk_label_new(controls[i].name);

        sliders[i] = gtk_hscale_new_with_range((gdouble)controls[i].min, (gdouble)controls[i].max, (gdouble)1);
        gtk_scale_set_digits(GTK_SCALE(sliders[i]), 0);
        gtk_range_set_value(GTK_RANGE(sliders[i]), (gdouble)controls[i].val);
        g_signal_connect(sliders[i], "value-changed", G_CALLBACK(set_value), (void*)i);

        int row, col;
        if (i <= ARRAY_SIZE(controls) / 2) {
            row = i;
            col = 0;
        }
        else {
            row = i - ARRAY_SIZE(controls) / 2 - 1;
            col = 2;
        }
        gtk_grid_attach(GTK_GRID(grid), labels[i], col, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), sliders[i], col + 1, row, 1, 1);
    }

    revert_button = gtk_button_new_with_label("Revert");
    g_signal_connect(revert_button, "clicked", G_CALLBACK(revert), sliders);

    gtk_grid_attach(GTK_GRID(grid), revert_button, 2, ARRAY_SIZE(controls) / 2 + 1, 2, 1);

    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_widget_show_all(window);
}

int
main(int argc, char **argv) {
    GtkApplication *app;
    int status = 0;
    const char *dev_filename = "/dev/i2c-0";

    dev = open(dev_filename, O_RDWR);
    if (dev < 0) {
        fprintf(stderr, "Error: could not open %s\n", dev_filename);
        goto end;
    }

    app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    if (close(dev) < 0) fprintf(stderr, "Error: could not close %s\n", dev_filename);
end:
    return status;
}
