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
use super::Destination;

pub mod c {
    use super::*;
    use crate::util::c::Wrapped;
    /// The mutable instance of state
    pub type Actor = Wrapped<State>;
    /// It's the same because the state is a simple mutex-protected type.
    /// There are no channels involved.
    pub type Destination = Wrapped<State>;
}

pub enum Event {
    Overlay(Option<String>),
    ScreensaverActive(bool),
}

impl Destination for c::Destination {
    type Event = Event;
    fn send(&self, event: Self::Event) {
        let actor = self.clone_ref();
        let mut actor = actor.borrow_mut();
        *actor = actor.clone().handle_event(event);
    }
}

#[derive(Clone)]
pub struct State {
    pub overlay: Option<String>,
    /// Settings button active
    pub settings_active: bool,
}

impl State {
    pub fn new(settings_active: bool) -> Self {
        Self {
            overlay: None,
            settings_active,
        }
    }
    
    fn handle_event(mut self, event: Event) -> Self {
        match event {
            Event::Overlay(overlay) => { self.overlay = overlay; },
            Event::ScreensaverActive(lock_active) => { self.settings_active = !lock_active; },
        };
        self
    }
}
