Layouts
=====

Squeekboard is composed of multiple layouts, several for each language, multiplied by each hint.

Layouts live in the "keyboards" directory.

Hints
-------

The currently supported hints are: default, "email", "emoji", "number', "pin", "terminal", and "url".

Each directory in "keyboards" is named after a hint, with the "keyboards" directory itself taking the role of default.

Languages/scripts
-----------------------

Each hint directory contains multiple layout files. A single language will be composed of multiple files, with names starting with the same text. The language names are taken from iso639-3. An example is "gr".

After the language name optionally comes a "+" and an indication of the variant. For example, "it+fur".

Squeekboard will look for those based on the currently selected layout in Gnome Control Center.

Then, there's an optional part "_wide", which Squeekboard will try to use if the current display is rather wide. Example: "us+colemak_wide" or "us_wide".

Finally, the file name ends with ".yaml", e.g. "jp+kana_wide.yaml".

Together with hint information, this gives a complete path to the layout like this: "keyboards/terminal/fr_wide.yaml" or "keyboards/cz+qwerty.yaml".

Layout syntax
------------------

The layout file follows the YAML syntax, with specific meanings given to sections.

### Outlines

The "outlines" dictionary controls the widths and heights of buttons. 

```
outlines:
    default: { width: 32, height: 52 }
```
The width and height numbers are not in pixels, but rather they are proportionally scaled to fit the panel size.

There may be any number of outlines, but there are some special names:
- "default" applies to every button unless explicitly changed. It should be used for buttons that emit text
- "altline", "wide" have own color scheme, should be used for buttons which cause view changes
- "special" has own color scheme, to be used for confirmations like enter.

### Views

The "views" dictionary contains the actual views and positions of buttons.

```
views:
    base:
        - "q w e r t y u i o p å"
```

Squeekboard's layouts consist of multiple views, of which only one is visible at a time. Different views may contain different or the same buttons, more or fewer buttons, but each layout is independent. They are *not* shift levels – there is no concept of "shift" in Squeekboard. View selection is also not dependent on modifiers.

There is only one special view "base". Views and view switching are described in detail in the [views](views.md) document.

Views in Squeekboard are based on rows. The first row comes near the top of the panel, the next one below, and so on.

```
- "Q W E R T Y U I O P Å"
- "upper   z x c v b n m  BackSpace"
```

Each row is a single string, and button names are separated by spaces. In left-to-right languages, the panel will be laid out just like the view code. CAUTION: buttons are placed on the panel left-to-right, starting from the earliest position in the string. That may not display great in your text editor when you use right-to-left characters as button names.

#### Button names in rows

Unicode characters are supported in the row string, so it's easy to use the correct name for most of them. However, the layout code is still YAML, which excludes certain characters: the space " ", the backslash "\", the double quote `"`. Those must use a replacement name.

Similarly, buttons that do not emit characters must have some names.

### Buttons

The buttons section describes what the button looks like and what it does.

```
    BackSpace:
        outline: altline
        icon: "edit-clear-symbolic"
        action: erase
```

Each entry in the "buttons" dictionary describes some button already present in one of the "views" rows. In the above example, it's "BackSpace".

The button description can have a number of components, each optional. For details, see 

- "outline" selects which entry from the "outlines" section to use to draw this button,
- "label" is what should be displayed on the button, if its name is unsuitable,
- "icon" is the name of the svg icon to use instead of a label (icons are builtin, see the "data/icons" directory),
- "text" is the text to submit when the button is clicked – if the name of the button is not suitable,
- "keysym" is the emulated keyboard keysym to send instead of sending text. Its use is discouraged: Squeekboard will automatically send keysyms if it detects that the receiving application does not accept text.
- "modifier" makes the button set an emulated keyboard modifier. The use of this is discouraged, and never needed for entering text.
- "action" sets aside the button for special actions like view switching

#### Action

```
        action:
            set_view: "numbers"
```

The "action" property has multiple forms.

- "erase" will erase the position behind the cursor,
- "show_preferences" will open the language selection popup,
- "set_view" simply switches to a view,
- "lock_view" switches to a view for a moment.

The two switching modes are better described in the [views](views.md) document.

Sources
----------

The sources, where all this is documented and up to date are in "src/data/parsing.rs". The reference documentation for the `rs::data::parsing::Layout` structure is the main place to look at.