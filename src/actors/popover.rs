/* Copyright (C) 2022 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 */

/*! The popover is opened directly by the GTK surface,
without bouncing click events off the main state.
Then it must accurately show which layout has been selected.
It can get the system layout directly from gsettings on open,
but it cannot get the user-selected overlay, because it's stored in state.

To solve this, overlay will be cached in the popover actor,
and updated by main state every time it changes.
*/

pub mod c {
    use super::*;
    use crate::util::c::Wrapped;
    /// The mutable instance of state
    pub type Actor = Wrapped<State>;
}

#[derive(Clone)]
pub struct State {
    pub overlay: Option<String>,
}

impl State {
    pub fn new() -> Self {
        Self { overlay: None }
    }
}

pub fn set_overlay(
    actor: &c::Actor,
    overlay: Option<String>,
) {
    let actor = actor.clone_ref();
    let mut actor = actor.borrow_mut();
    actor.overlay = overlay;
}