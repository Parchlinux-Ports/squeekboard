/* 
 * Copyright (C) 2010-2011 Daiki Ueno <ueno@unixuser.org>
 * Copyright (C) 2010-2011 Red Hat, Inc.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#if !defined(__EEKBOARD_SERVICE_H_INSIDE__) && !defined(EEKBOARD_COMPILATION)
#error "Only <eekboard/eekboard-service.h> can be included directly."
#endif

#ifndef EEKBOARD_CONTEXT_SERVICE_H
#define EEKBOARD_CONTEXT_SERVICE_H 1

#include "src/submission.h"
#include "src/layout.h"

#include "virtual-keyboard-unstable-v1-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"

G_BEGIN_DECLS

#define EEKBOARD_TYPE_LAYOUT_HOLDER (layout_holder_get_type())
#define LAYOUT_HOLDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EEKBOARD_TYPE_LAYOUT_HOLDER, LayoutHolder))
#define EEKBOARD_CONTEXT_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), EEKBOARD_TYPE_CONTEXT_SERVICE, EekboardContextServiceClass))
#define EEKBOARD_IS_CONTEXT_SERVICE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EEKBOARD_TYPE_CONTEXT_SERVICE))
#define EEKBOARD_IS_CONTEXT_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EEKBOARD_TYPE_CONTEXT_SERVICE))
#define EEKBOARD_CONTEXT_SERVICE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), EEKBOARD_TYPE_CONTEXT_SERVICE, EekboardContextServiceClass))


typedef struct _LayoutHolderClass LayoutHolderClass;
typedef struct _LayoutHolderPrivate LayoutHolderPrivate;

/**
 * Handles layout state, and virtual-keyboard.
 *
 * TODO: Restrict to managing keyboard layouts, and maybe button repeats,
 * and the virtual keyboard protocol.
 */
struct _LayoutHolder {
    GObject parent;
    LayoutHolderPrivate *priv;
    struct squeek_layout_state *layout; // Unowned
};

struct _LayoutHolderClass {
    /*< private >*/
    GObjectClass parent_class;

    /*< private >*/
    /* padding */
    gpointer pdummy[24];
};

GType         layout_holder_get_type(void) G_GNUC_CONST;

/// Handles gsettings os-level keyboard layout switches.
struct gsettings_tracker {
    GSettings *gsettings; // Owned reference
    LayoutHolder *context; // Unowned
    struct squeek_layout_state *layout; // Unowned
};

void eek_gsettings_tracker_init(struct gsettings_tracker* tracker, LayoutHolder *context, struct squeek_layout_state *layout);

LayoutHolder *eek_layout_holder_new(struct squeek_layout_state *state);
void eek_layout_holder_set_submission(LayoutHolder *context, struct submission *submission);
LevelKeyboard *eek_layout_holder_get_keyboard(LayoutHolder *context);

void eekboard_context_service_set_keymap(LayoutHolder *context,
                                         const LevelKeyboard *keyboard);

void eekboard_context_service_set_hint_purpose(LayoutHolder *context,
                                               uint32_t hint,
                                               uint32_t purpose);
void
eek_layout_holder_use_layout(LayoutHolder *context, struct squeek_layout_state *layout);
G_END_DECLS
#endif  /* EEKBOARD_CONTEXT_SERVICE_H */
