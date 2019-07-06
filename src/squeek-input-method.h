#pragma once 

#include <glib-object.h>

#include "input-method-unstable-v2-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"
#include "squeek-enumtypes.h"

G_BEGIN_DECLS

#define SQUEEK_TYPE_INPUT_METHOD (squeek_input_method_get_type())
G_DECLARE_DERIVABLE_TYPE(SqueekInputMethod, squeek_input_method, SQUEEK, INPUT_METHOD, GObject)

typedef enum {
    /**
     * no special behavior
     */
    SQUEEK_INPUT_METHOD_HINT_NONE                = ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,
    /**
     * suggest word completions
     */
    SQUEEK_INPUT_METHOD_HINT_COMPLETION          = ZWP_TEXT_INPUT_V3_CONTENT_HINT_COMPLETION,
    /**
     * suggest word corrections
     */
    SQUEEK_INPUT_METHOD_HINT_SPELLCHECK          = ZWP_TEXT_INPUT_V3_CONTENT_HINT_SPELLCHECK,
    /**
     * switch to uppercase letters at the start of a sentence
     */
    SQUEEK_INPUT_METHOD_HINT_AUTO_CAPITALIZATION = ZWP_TEXT_INPUT_V3_CONTENT_HINT_AUTO_CAPITALIZATION,
    /**
     * prefer lowercase letters
     */
    SQUEEK_INPUT_METHOD_HINT_LOWERCASE           = ZWP_TEXT_INPUT_V3_CONTENT_HINT_LOWERCASE,
    /**
     * prefer uppercase letters
     */
    SQUEEK_INPUT_METHOD_HINT_UPPERCASE           = ZWP_TEXT_INPUT_V3_CONTENT_HINT_UPPERCASE,
    /**
     * prefer casing for titles and headings (can be language dependent)
     */
    SQUEEK_INPUT_METHOD_HINT_TITLECASE           = ZWP_TEXT_INPUT_V3_CONTENT_HINT_TITLECASE,
    /**
     * characters should be hidden
     */
    SQUEEK_INPUT_METHOD_HINT_HIDDEN_TEXT         = ZWP_TEXT_INPUT_V3_CONTENT_HINT_HIDDEN_TEXT,
    /**
     * typed text should not be stored
     */
    SQUEEK_INPUT_METHOD_HINT_SENSITIVE_DATA      = ZWP_TEXT_INPUT_V3_CONTENT_HINT_SENSITIVE_DATA,
    /**
     * just Latin characters should be entered
     */
    SQUEEK_INPUT_METHOD_HINT_LATIN               = ZWP_TEXT_INPUT_V3_CONTENT_HINT_LATIN,
    /**
     * the text input is multiline
     */
    SQUEEK_INPUT_METHOD_HINT_MULTILINE           = ZWP_TEXT_INPUT_V3_CONTENT_HINT_MULTILINE,
} SqueekInputMethodHint;

typedef enum {
    /**
     * default input, allowing all characters
     */
    SQUEEK_INPUT_METHOD_PURPOSE_NORMAL   = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL,
    /**
     * allow only alphabetic characters
     */
    SQUEEK_INPUT_METHOD_PURPOSE_ALPHA    = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_ALPHA,
    /**
     * allow only digits
     */
    SQUEEK_INPUT_METHOD_PURPOSE_DIGITS   = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DIGITS,
    /**
     * input a number (including decimal separator and sign)
     */
    SQUEEK_INPUT_METHOD_PURPOSE_NUMBER   = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NUMBER,
    /**
     * input a phone number
     */
    SQUEEK_INPUT_METHOD_PURPOSE_PHONE    = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PHONE,
    /**
     * input an URL
     */
    SQUEEK_INPUT_METHOD_PURPOSE_URL      = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_URL,
    /**
     * input an email address
     */
    SQUEEK_INPUT_METHOD_PURPOSE_EMAIL    = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_EMAIL,
    /**
     * input a name of a person
     */
    SQUEEK_INPUT_METHOD_PURPOSE_NAME     = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NAME,
    /**
     * input a password (combine with sensitive_data hint)
     */
    SQUEEK_INPUT_METHOD_PURPOSE_PASSWORD = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PASSWORD,
    /**
     * input is a numeric password (combine with sensitive_data hint)
     */
    SQUEEK_INPUT_METHOD_PURPOSE_PIN      = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PIN,
    /**
     * input a date
     */
    SQUEEK_INPUT_METHOD_PURPOSE_DATE     = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATE,
    /**
     * input a time
     */
    SQUEEK_INPUT_METHOD_PURPOSE_TIME     = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TIME,
    /**
     * input a date and time
     */
    SQUEEK_INPUT_METHOD_PURPOSE_DATETIME = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATETIME,
    /**
     * input for a terminal
     */
    SQUEEK_INPUT_METHOD_PURPOSE_TERMINAL = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TERMINAL,
} SqueekInputMethodPurpose;


struct _SqueekInputMethodClass {
    GObjectClass        parent_class;

    /* input-method signals */
    void        (*activate)                       (SqueekInputMethod *self);

    void        (*deactivate)                     (SqueekInputMethod *self);

    void        (*surrounding_text)               (SqueekInputMethod *self,
                                                   const gchar       *text,
                                                   guint              cursor,
                                                   guint              anchor);

    void        (*text_change_cause)              (SqueekInputMethod *self,
                                                   guint              cause);

    void        (*content_type)                   (SqueekInputMethod *self,
                                                   guint              hint,
                                                   guint              purpose);

    void        (*done)                           (SqueekInputMethod *self);

    void        (*unavailable)                    (SqueekInputMethod *self);

    /* input-method requests */
    void        (*commit_string)                  (SqueekInputMethod *self,
                                                   const gchar       *string);

    void        (*preedit_string)                 (SqueekInputMethod *self,
                                                   const gchar       *text,
                                                   gint               cursor_begin,
                                                   gint               cursor_end);

    void        (*delete_surrounding_text)        (SqueekInputMethod *self,
                                                   guint              before_length,
                                                   guint              after_length);


    void        (*commit)                         (SqueekInputMethod *self);
};

void                 squeek_input_method_commit_string          (SqueekInputMethod *self,
                                                                 const gchar       *string);

void                 squeek_input_method_preedit_string         (SqueekInputMethod *self,
                                                                 const gchar       *text,
                                                                 gint               cursor_begin,
                                                                 gint               cursor_end);

void                 squeek_input_method_delete_surrounding_text(SqueekInputMethod *self,
                                                                 guint              before_length,
                                                                 guint              after_length);

void                 squeek_input_method_commit                 (SqueekInputMethod *self);

SqueekInputMethod   *squeek_input_method_new                    (struct zwp_input_method_manager_v2 *manager,
                                                                 struct wl_seat                     *seat);

G_END_DECLS


