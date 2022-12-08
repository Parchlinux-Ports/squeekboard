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
use crate::logging;
use std::borrow::BorrowMut;
use std::sync::{Arc, Mutex};

pub mod c {
    use super::*;
    use crate::util::c::ArcWrapped;
    /// The mutable instance of state.
    /// Thread-safe because this actor does not get its own event loop,
    /// and therefore can't have a channel to receive messages,
    /// so instead messages will be passed directly to the mutexed actor.
    pub type Actor = ArcWrapped<State>;
}

pub type Destination = Arc<Mutex<State>>;

#[derive(Debug)]
pub enum Event {
    Overlay(Option<String>),
    ScreensaverActive(bool),
}

impl super::Destination for Destination {
    type Event = Event;
    fn send(&self, event: Self::Event) {
        let actor = self.lock();
        match actor {
            Ok(mut actor) => {
                let actor = actor.borrow_mut();
                **actor = actor.clone().handle_event(event);
            },
            Err(e) => log_print!(
                logging::Level::Bug,
                "Cannot lock popover state: {:?}",
                e,
            ),
        }
    }
}

#[derive(Clone, Debug)]
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
