#pragma once

#include "eek/layersurface.h"
#include "src/layout.h"
#include "src/main.h"
#include "src/submission.h"

// Stores the objects that the panel and its widget will refer to
struct panel_manager {
    EekboardContextService *state; // unowned
    /// Needed for instantiating the widget
    struct squeek_state_manager *state_manager; // shared reference
    struct squeek_popover *popover; // shared reference
    struct submission *submission; // unowned

    // both memoized - doesn't have to be, but bugs happen:
    // https://gitlab.gnome.org/World/Phosh/squeekboard/-/issues/343
    PhoshLayerSurface *window;
    GtkWidget *widget;

    // Those should be held in Rust
    struct wl_output *current_output;
};

struct panel_manager panel_manager_new(EekboardContextService *state, struct submission *submission, struct squeek_state_manager *state_manager, struct squeek_popover *popover);
