// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

// Injects touch input into a KWin session through the fake input protocol.
// KWin must run with KWIN_WAYLAND_NO_PERMISSION_CHECKS=1.
//
//   touch tap X Y
//   touch swipe X1 Y1 X2 Y2 [X3 Y3 ...]

#include "fake-input-client-protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-client.h>

static struct org_kde_kwin_fake_input *fake_input;

static void registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    (void)data;
    if (strcmp(interface, org_kde_kwin_fake_input_interface.name) == 0) {
        fake_input = wl_registry_bind(registry, name, &org_kde_kwin_fake_input_interface, version < 4 ? version : 4);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {registry_global, registry_global_remove};

static void sleep_ms(int ms)
{
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s tap X Y | swipe X1 Y1 X2 Y2 [...]\n", argv[0]);
        return 1;
    }

    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "cannot connect to wayland\n");
        return 1;
    }
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    if (!fake_input) {
        fprintf(stderr, "no org_kde_kwin_fake_input\n");
        return 1;
    }
    org_kde_kwin_fake_input_authenticate(fake_input, "touch", "testing plasma-keyboard");

    const int points = (argc - 2) / 2;
    double x = atof(argv[2]), y = atof(argv[3]);
    org_kde_kwin_fake_input_touch_down(fake_input, 0, wl_fixed_from_double(x), wl_fixed_from_double(y));
    org_kde_kwin_fake_input_touch_frame(fake_input);
    wl_display_flush(display);
    sleep_ms(40);

    // Swipes move in small steps between the given points
    for (int point = 1; point < points; ++point) {
        const double toX = atof(argv[2 + point * 2]), toY = atof(argv[3 + point * 2]);
        for (int step = 1; step <= 10; ++step) {
            const double t = step / 10.0;
            org_kde_kwin_fake_input_touch_motion(fake_input, 0, wl_fixed_from_double(x + (toX - x) * t), wl_fixed_from_double(y + (toY - y) * t));
            org_kde_kwin_fake_input_touch_frame(fake_input);
            wl_display_flush(display);
            sleep_ms(12);
        }
        x = toX;
        y = toY;
    }

    org_kde_kwin_fake_input_touch_up(fake_input, 0);
    org_kde_kwin_fake_input_touch_frame(fake_input);
    wl_display_roundtrip(display);
    wl_display_disconnect(display);
    return 0;
}
