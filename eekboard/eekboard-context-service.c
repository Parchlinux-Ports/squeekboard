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

#include "config.h"

#include <stdio.h>

#include <gio/gio.h>

#include "wayland.h"

#include "eek/eek-keyboard.h"
#include "src/server-context-service.h"

#include "eekboard/eekboard-context-service.h"

enum {
    PROP_0, // Magic: without this, keyboard is not useable in g_object_notify
    PROP_KEYBOARD,
    PROP_LAST
};

#define LAYOUT_HOLDER_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), EEKBOARD_TYPE_LAYOUT_HOLDER, LayoutHolderPrivate))

struct _LayoutHolderPrivate {
    LevelKeyboard *keyboard; // currently used keyboard

    /// Needed for keymap changes after keyboard updates
    struct submission *submission; // unowned
};

G_DEFINE_TYPE_WITH_PRIVATE (LayoutHolder, layout_holder, G_TYPE_OBJECT);

static void
eekboard_context_service_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
    (void)value;
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
eekboard_context_service_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
    LayoutHolder *context = LAYOUT_HOLDER(object);

    switch (prop_id) {
    case PROP_KEYBOARD:
        g_value_set_object (value, context->priv->keyboard);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
eekboard_context_service_dispose (GObject *object)
{
    G_OBJECT_CLASS (layout_holder_parent_class)->
        dispose (object);
}

static void
settings_get_layout(GSettings *settings, char **type, char **layout)
{
    if (!settings) {
        return;
    }
    GVariant *inputs = g_settings_get_value(settings, "sources");
    if (g_variant_n_children(inputs) == 0) {
        g_warning("No system layout present");
        *type = NULL;
        *layout = NULL;
    } else {
        // current layout is always first
        g_variant_get_child(inputs, 0, "(ss)", type, layout);
    }
    g_variant_unref(inputs);
}

void
eekboard_context_service_use_layout(LayoutHolder *context, struct squeek_layout_state *state) {
    gchar *layout_name = state->overlay_name;

    if (layout_name == NULL) {
        layout_name = state->layout_name;

        switch (state->purpose) {
        case ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NUMBER:
        case ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PHONE:
            layout_name = "number";
            break;
        case ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TERMINAL:
            layout_name = "terminal";
            break;
        default:
            ;
        }

        if (layout_name == NULL) {
            layout_name = "us";
        }
    }

    // generic part follows
    struct squeek_layout *layout = squeek_load_layout(layout_name, state->arrangement);
    LevelKeyboard *keyboard = level_keyboard_new(layout);
    // set as current
    LevelKeyboard *previous_keyboard = context->priv->keyboard;
    context->priv->keyboard = keyboard;
    // Update the keymap if necessary.
    // TODO: Update submission on change event
    if (context->priv->submission) {
        submission_set_keyboard(context->priv->submission, keyboard);
    }

    // Update UI
    g_object_notify (G_OBJECT(context), "keyboard");

    // replacing the keyboard above will cause the previous keyboard to get destroyed from the UI side (eek_gtk_keyboard_dispose)
    if (previous_keyboard) {
        level_keyboard_free(previous_keyboard);
    }
}

static void
layout_holder_init (LayoutHolder *self) {
    self->priv = LAYOUT_HOLDER_GET_PRIVATE(self);
}

static void
eekboard_context_service_constructed (GObject *object)
{
    (void)object;
}

static void
layout_holder_class_init (LayoutHolderClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GParamSpec *pspec;

    gobject_class->constructed = eekboard_context_service_constructed;
    gobject_class->set_property = eekboard_context_service_set_property;
    gobject_class->get_property = eekboard_context_service_get_property;
    gobject_class->dispose = eekboard_context_service_dispose;

    /**
     * EekboardContextService:keyboard:
     *
     * An #EekKeyboard currently active in this context.
     */
    pspec = g_param_spec_pointer("keyboard",
                                 "Keyboard",
                                 "Keyboard",
                                 G_PARAM_READABLE);
    g_object_class_install_property (gobject_class,
                                     PROP_KEYBOARD,
                                     pspec);
}

void
eekboard_context_service_destroy (LayoutHolder *context)
{
    (void)context;
}

/**
 * eekboard_context_service_get_keyboard:
 * @context: an #EekboardContextService
 *
 * Get keyboard currently active in @context.
 * Returns: (transfer none): an #EekKeyboard
 */
LevelKeyboard *
eekboard_context_service_get_keyboard (LayoutHolder *context)
{
    return context->priv->keyboard;
}

void eekboard_context_service_set_hint_purpose(LayoutHolder *context,
                                               uint32_t hint, uint32_t purpose)
{
    if (context->layout->hint != hint || context->layout->purpose != purpose) {
        context->layout->hint = hint;
        context->layout->purpose = purpose;
        eekboard_context_service_use_layout(context, context->layout);
    }
}

void
eekboard_context_service_set_overlay(LayoutHolder *context, const char* name) {
    if (g_strcmp0(context->layout->overlay_name, name)) {
        g_free(context->layout->overlay_name);
        context->layout->overlay_name = g_strdup(name);
        eekboard_context_service_use_layout(context, context->layout);
    }
}

const char*
eekboard_context_service_get_overlay(LayoutHolder *context) {
    return context->layout->overlay_name;
}

LayoutHolder *eekboard_context_service_new(struct squeek_layout_state *state)
{
    LayoutHolder *context = g_object_new (EEKBOARD_TYPE_LAYOUT_HOLDER, NULL);
    context->layout = state;
    eekboard_context_service_use_layout(context, context->layout);
    return context;
}

void eekboard_context_service_set_submission(LayoutHolder *context, struct submission *submission) {
    context->priv->submission = submission;
    if (context->priv->submission) {
        submission_set_keyboard(context->priv->submission, context->priv->keyboard);
    }
}

static void settings_update_layout(struct gsettings_tracker *self) {
    // The layout in the param must be the same layout as held by context.
    g_autofree gchar *keyboard_layout = NULL;
    g_autofree gchar *keyboard_type = NULL;
    settings_get_layout(self->gsettings,
                        &keyboard_type, &keyboard_layout);

    if (g_strcmp0(self->layout->layout_name, keyboard_layout) != 0 || self->layout->overlay_name) {
        g_free(self->layout->overlay_name);
        self->layout->overlay_name = NULL;
        if (keyboard_layout) {
            g_free(self->layout->layout_name);
            self->layout->layout_name = g_strdup(keyboard_layout);
        }
        // This must actually update the UI.
        eekboard_context_service_use_layout(self->context, self->layout);
    }
}

static gboolean
handle_layout_changed(GSettings *s,
                               gpointer keys, gint n_keys,
                               gpointer user_data) {
    (void)s;
    (void)keys;
    (void)n_keys;
    struct gsettings_tracker *self = user_data;
    settings_update_layout(self);
    return TRUE;
}

void eek_gsettings_tracker_init(struct gsettings_tracker *tracker, LayoutHolder *context, struct squeek_layout_state *layout)
{
    tracker->layout = layout;
    tracker->context = context;

    const char *schema_name = "org.gnome.desktop.input-sources";
    GSettingsSchemaSource *ssrc = g_settings_schema_source_get_default();
    if (ssrc) {
        GSettingsSchema *schema = g_settings_schema_source_lookup(ssrc,
                                                                  schema_name,
                                                                  TRUE);
        if (schema) {
            // Not referencing the found schema directly,
            // because it's not clear how...
            tracker->gsettings = g_settings_new (schema_name);
            gulong conn_id = g_signal_connect(tracker->gsettings, "change-event",
                                              G_CALLBACK(handle_layout_changed),
                                              tracker);
            if (conn_id == 0) {
                g_warning ("Could not connect to gsettings updates, "
                           "automatic layout changing unavailable");
            }
        } else {
            g_warning("Gsettings schema %s is not installed on the system. "
                      "Layout switching unavailable", schema_name);
        }
    } else {
        g_warning("No gsettings schemas installed. Layout switching unavailable.");
    }

    settings_update_layout(tracker);
}
