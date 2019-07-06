/* squeek-input-method.c */
#include "squeek-input-method.h"

#include <stdio.h>

typedef struct _SqueekInputMethodPrivate {
    struct zwp_input_method_v2         *input_method;

    gchar                              *surrounding_text;
    gchar                              *preedit_string;

    guint                               serial;

    guint                               cursor;
    guint                               anchor;

    guint                               hint;
    guint                               purpose;

    gboolean                            active    : 1;
    gboolean                            available : 1;
} SqueekInputMethodPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(SqueekInputMethod, squeek_input_method, G_TYPE_OBJECT)

/*
 * TODO:
 *   - implement signals for 'done' and ... (?)
 *   - take it for a spin
 *   - fix things
 *   - look at 'zwp_input_popup_surface_v2' and think about it for a while
 *     ....
 *   - add some gir documentation
 *   - double check: everyting
 *   
 *   - MOVE ON
 */

enum {
    PROP_0,
    PROP_AVAILABLE,
    PROP_ACTIVE,
    PROP_CURSOR_POSITION,
    PROP_ANCHOR_POSITION,
    PROP_CONTENT_HINT,
    PROP_CONTENT_PURPOSE,
    PROP_SURROUNDING_TEXT,
    PROP_PREEDIT_STRING,
    LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

/* input-method signals */
static void
squeek_input_method_activate(SqueekInputMethod *self)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    if (!priv->active) {
        priv->active = TRUE;
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ACTIVE]);
    }
}

static void
squeek_input_method_deactivate(SqueekInputMethod *self)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    if (priv->active) {
        priv->active = FALSE;
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ACTIVE]);
    }
}

static void
squeek_input_method_surrounding_text(SqueekInputMethod *self,
                                     const gchar       *text,
                                     guint              cursor,
                                     guint              anchor)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    g_clear_pointer(&priv->surrounding_text, g_free);

    priv->surrounding_text = g_strdup(text);
    priv->cursor           = cursor;
    priv->anchor           = anchor;

    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_SURROUNDING_TEXT]);
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_CURSOR_POSITION]);
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ANCHOR_POSITION]);
}

static void
squeek_input_method_text_change_cause(SqueekInputMethod *self,
                                      guint              cause)
{
    fprintf(stderr, "%s: not implemented\n", __func__);
}

static void
squeek_input_method_content_type(SqueekInputMethod *self,
                                 guint              hint,
                                 guint              purpose)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    priv->hint    = hint;
    priv->purpose = purpose;

    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_CONTENT_HINT]);
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_CONTENT_PURPOSE]);
}

static void
squeek_input_method_done(SqueekInputMethod *self)
{
    /* 
        Atomically applies state changes recently sent to the client.

        The done event establishes and updates the state of the client, and
        must be issued after any changes to apply them.

        Text input state (content purpose, content hint, surrounding text, and
        change cause) is conceptually double-buffered within an input method
        context.

        Events modify the pending state, as opposed to the current state in use
        by the input method. A done event atomically applies all pending state,
        replacing the current state. After done, the new pending state is as
        documented for each related request.

        Events must be applied in the order of arrival.

        Neither current nor pending state are modified unless noted otherwise.
     */

    fprintf(stderr, "%s: not implemented\n", __func__);
}

static void
squeek_input_method_unavailable(SqueekInputMethod *self)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    priv->available = FALSE;

    g_clear_pointer(&priv->input_method, zwp_input_method_v2_destroy); 

    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AVAILABLE]);
}

static void
imservice_handle_input_method_activate(void                       *data,
                                       struct zwp_input_method_v2 *input_method)
{
    g_return_if_fail(data && SQUEEK_IS_INPUT_METHOD(data));

    SQUEEK_INPUT_METHOD_GET_CLASS(data)->activate(data);
}

static void
imservice_handle_input_method_deactivate(void                       *data,
                                         struct zwp_input_method_v2 *input_method)
{
    g_return_if_fail(data && SQUEEK_IS_INPUT_METHOD(data));

    SQUEEK_INPUT_METHOD_GET_CLASS(data)->deactivate(data);
}

static void
imservice_handle_surrounding_text(void                       *data,
                                  struct zwp_input_method_v2 *input_method,
                                  const char                 *text,
                                  uint32_t                    cursor,
                                  uint32_t                    anchor)
{
    g_return_if_fail(data && SQUEEK_IS_INPUT_METHOD(data));

    SQUEEK_INPUT_METHOD_GET_CLASS(data)->surrounding_text(data, text, cursor, anchor);
}

static void
imservice_handle_text_change_cause(void                       *data,
                                   struct zwp_input_method_v2 *input_method,
                                   uint32_t                    cause)
{
    g_return_if_fail(data && SQUEEK_IS_INPUT_METHOD(data));

    SQUEEK_INPUT_METHOD_GET_CLASS(data)->text_change_cause(data, cause);
}

static void
imservice_handle_content_type(void                       *data,
                              struct zwp_input_method_v2 *input_method,
                              uint32_t                    hint,
                              uint32_t                    purpose)
{
    g_return_if_fail(data && SQUEEK_IS_INPUT_METHOD(data));

    SQUEEK_INPUT_METHOD_GET_CLASS(data)->content_type(data, hint, purpose);
}

static void
imservice_handle_done(void                       *data,
                      struct zwp_input_method_v2 *input_method)
{
    g_return_if_fail(data && SQUEEK_IS_INPUT_METHOD(data));

    SQUEEK_INPUT_METHOD_GET_CLASS(data)->done(data);
}

static void
imservice_handle_unavailable(void                       *data,
                             struct zwp_input_method_v2 *input_method)
{
    g_return_if_fail(data && SQUEEK_IS_INPUT_METHOD(data));

    SQUEEK_INPUT_METHOD_GET_CLASS(data)->unavailable(data);
}

static const struct zwp_input_method_v2_listener input_method_listener = {
    .activate          = imservice_handle_input_method_activate,
    .deactivate        = imservice_handle_input_method_deactivate,
    .surrounding_text  = imservice_handle_surrounding_text,
    .text_change_cause = imservice_handle_text_change_cause,
    .content_type      = imservice_handle_content_type,
    .done              = imservice_handle_done,
    .unavailable       = imservice_handle_unavailable,
};

/* input-method requests */
static void
squeek_input_method_real_commit_string(SqueekInputMethod *self,
                                       const gchar       *string)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    zwp_input_method_v2_commit_string(priv->input_method,
                                      string);
}

void
squeek_input_method_commit_string(SqueekInputMethod *self,
                                  const gchar       *string)
{
    g_return_if_fail(self && SQUEEK_IS_INPUT_METHOD(self));

    SQUEEK_INPUT_METHOD_GET_CLASS(self)->commit_string(self,
                                                       string);
}

static void
squeek_input_method_real_preedit_string(SqueekInputMethod *self,
                                        const gchar       *text,
                                        gint               cursor_begin,
                                        gint               cursor_end)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    g_clear_pointer(&priv->preedit_string, g_free);
    priv->preedit_string = g_strdup(text);

    /* TODO: think about cursor_[begin|end] */

    zwp_input_method_v2_preedit_string(priv->input_method,
                                       priv->preedit_string,
                                       cursor_begin,
                                       cursor_end);

    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PREEDIT_STRING]);
}

void
squeek_input_method_preedit_string(SqueekInputMethod *self,
                                   const gchar       *text,
                                   gint               cursor_begin,
                                   gint               cursor_end)
{
    g_return_if_fail(self && SQUEEK_IS_INPUT_METHOD(self));

    SQUEEK_INPUT_METHOD_GET_CLASS(self)->preedit_string(self,
                                                        text,
                                                        cursor_begin,
                                                        cursor_end);
}

static void
squeek_input_method_real_delete_surrounding_text(SqueekInputMethod *self,
                                                 guint              before_length,
                                                 guint              after_length)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    zwp_input_method_v2_delete_surrounding_text(priv->input_method,
                                                before_length,
                                                after_length);
}

void
squeek_input_method_delete_surrounding_text(SqueekInputMethod *self,
                                            guint              before_length,
                                            guint              after_length)
{
    g_return_if_fail(self && SQUEEK_IS_INPUT_METHOD(self));

    SQUEEK_INPUT_METHOD_GET_CLASS(self)->delete_surrounding_text(self,
                                                                 before_length,
                                                                 after_length);
}

static void
squeek_input_method_real_commit(SqueekInputMethod *self)
{
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    g_return_if_fail(priv->available);

    zwp_input_method_v2_commit(priv->input_method,
                               priv->serial++);
}

void
squeek_input_method_commit(SqueekInputMethod *self)
{
    g_return_if_fail(self && SQUEEK_IS_INPUT_METHOD(self));

    SQUEEK_INPUT_METHOD_GET_CLASS(self)->commit(self);
}

static void
squeek_input_method_dispose(GObject *object)
{
    SqueekInputMethod        *self = SQUEEK_INPUT_METHOD(object);
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    priv->active    = FALSE;
    priv->available = FALSE;

    g_clear_pointer(&priv->input_method, zwp_input_method_v2_destroy);

    G_OBJECT_CLASS(squeek_input_method_parent_class)->dispose(object);
}

static void
squeek_input_method_init(SqueekInputMethod *self)
{
}

static void
squeek_input_method_get_property(GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    SqueekInputMethod        *self = SQUEEK_INPUT_METHOD(object);
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    switch (prop_id) {
        case PROP_AVAILABLE:
            g_value_set_boolean(value, priv->available);
            break;
        case PROP_ACTIVE:
            g_value_set_boolean(value, priv->active);
            break;
        case PROP_CURSOR_POSITION:
            g_value_set_uint(value, priv->cursor);
            break;
        case PROP_ANCHOR_POSITION:
            g_value_set_uint(value, priv->anchor);
            break;
        case PROP_CONTENT_HINT:
            g_value_set_flags(value, priv->hint);
            break;
        case PROP_CONTENT_PURPOSE:
            g_value_set_enum(value, priv->purpose);
            break;
        case PROP_SURROUNDING_TEXT:
            g_value_set_string(value, priv->surrounding_text);
            break;
        case PROP_PREEDIT_STRING:
            g_value_set_string(value, priv->preedit_string);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
squeek_input_method_set_property(GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    SqueekInputMethod        *self = SQUEEK_INPUT_METHOD(object);
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    switch (prop_id) {
        case PROP_ACTIVE:
            priv->active = g_value_get_boolean(value);

            break;

        /* FIXME: should preedit string be writable without setting cursor? */
        case PROP_PREEDIT_STRING:
            g_clear_pointer(&priv->preedit_string, g_free);
            priv->preedit_string = g_strdup(g_value_get_string(value));

            break;

        case PROP_AVAILABLE:
            /* fall-through: not writable */
        case PROP_CURSOR_POSITION:
            /* fall-through: not writable */
        case PROP_ANCHOR_POSITION:
            /* fall-through: not writable */
        case PROP_CONTENT_HINT:
            /* fall-through: not writable */
        case PROP_CONTENT_PURPOSE:
            /* fall-through: not writable */
        case PROP_SURROUNDING_TEXT:
            /* fall-through: not writable */
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
squeek_input_method_class_init(SqueekInputMethodClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->dispose          = squeek_input_method_dispose;
    object_class->get_property     = squeek_input_method_get_property;
    object_class->set_property     = squeek_input_method_set_property;

    /* events */
    klass->activate                = squeek_input_method_activate;
    klass->deactivate              = squeek_input_method_deactivate;
    klass->surrounding_text        = squeek_input_method_surrounding_text;
    klass->text_change_cause       = squeek_input_method_text_change_cause;
    klass->content_type            = squeek_input_method_content_type;
    klass->done                    = squeek_input_method_done;
    klass->unavailable             = squeek_input_method_unavailable;

    /* requests */
    klass->commit_string           = squeek_input_method_real_commit_string;
    klass->preedit_string          = squeek_input_method_real_preedit_string;
    klass->delete_surrounding_text = squeek_input_method_real_delete_surrounding_text;
    klass->commit                  = squeek_input_method_real_commit;

    properties[PROP_AVAILABLE] =
        g_param_spec_boolean("available",
                             "available",
                             "Availability of this input method (destroy if FALSE)",
                             FALSE,
                             G_PARAM_READABLE);

    properties[PROP_ACTIVE] =
        g_param_spec_boolean("active",
                             "active",
                             "This input method is active",
                             FALSE,
                             G_PARAM_READWRITE);

    properties[PROP_CURSOR_POSITION] =
        g_param_spec_uint("cursor-position",
                          "cursor-position",
                          "The position of the cursor in characters",
                          0, G_MAXUINT, 0,
                          G_PARAM_READABLE);

    properties[PROP_ANCHOR_POSITION] =
        g_param_spec_uint("anchor-position",
                          "anchor-position",
                          "Offset where the current selection starts, or the same as cursor-position",
                          0, G_MAXUINT, 0,
                          G_PARAM_READABLE);

    properties[PROP_CONTENT_HINT] =
        g_param_spec_flags("content-hint",
                           "content-hint",
                           "Hint to guide the behaviour of the input method",
                           SQUEEK_TYPE_INPUT_METHOD_HINT,
                           SQUEEK_INPUT_METHOD_HINT_NONE,
                           G_PARAM_READABLE);

    properties[PROP_CONTENT_PURPOSE] =
        g_param_spec_enum("content-purpose",
                          "content-purpose",
                          "The purpose of a text input",
                          SQUEEK_TYPE_INPUT_METHOD_PURPOSE,
                          SQUEEK_INPUT_METHOD_PURPOSE_NORMAL,
                          G_PARAM_READABLE);

    properties[PROP_SURROUNDING_TEXT] =
        g_param_spec_string("surrounding-text",
                            "surrounding-text",
                            "The text surrounding the cursor",
                            "",
                            G_PARAM_READABLE);

    properties[PROP_PREEDIT_STRING] =
        g_param_spec_string("preedit-string",
                            "preedit-string",
                            "Pre-edit string",
                            "",
                            G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, LAST_PROP, properties);
}

SqueekInputMethod *
squeek_input_method_new(struct zwp_input_method_manager_v2 *manager,
                        struct wl_seat                     *seat)
{
    SqueekInputMethod *self = g_object_new(SQUEEK_TYPE_INPUT_METHOD, NULL);
    SqueekInputMethodPrivate *priv = squeek_input_method_get_instance_private(self);

    priv->input_method = zwp_input_method_manager_v2_get_input_method(manager, seat);

    zwp_input_method_v2_add_listener(priv->input_method, &input_method_listener, self);

    return self;
}
