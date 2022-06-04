#pragma once
/// This all wraps https://gtk-rs.org/gtk-rs-core/stable/latest/docs/glib/struct.MainContext.html#method.channel

#include <inttypes.h>

#include "input-method-unstable-v2-client-protocol.h"
#include "virtual-keyboard-unstable-v1-client-protocol.h"

#include "eek/eek-types.h"
#include "dbus.h"
#include "panel.h"
#include "src/popover.h"


struct receiver;

/// Wrapped<event_loop::driver::Threaded>
struct squeek_state_manager;

struct submission;

struct rsobjects {
    struct receiver *receiver;
    struct squeek_state_manager *state_manager;
    struct submission *submission;
    struct squeek_wayland *wayland;
    struct squeek_popover *popover;
};

void register_ui_loop_handler(struct receiver *receiver, struct panel_manager *panel, struct squeek_popover *popover, EekboardContextService *hint_manager, DBusHandler *dbus_handler);

struct rsobjects squeek_init(void);

void squeek_state_send_force_visible(struct squeek_state_manager *state);
void squeek_state_send_force_hidden(struct squeek_state_manager *state);

void squeek_state_send_keyboard_present(struct squeek_state_manager *state, uint32_t keyboard_present);
void squeek_state_send_layout_set(struct squeek_state_manager *state, char *name, char *layout, uint32_t timestamp);
