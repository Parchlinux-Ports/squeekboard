/* Copyright (C) 2022 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 */

/*! Actors are parts of Squeekboard containing state independent from the main application state.

Because main application state is meant to be immutable,
it cannot be referenced directly by pieces of logic
interacting with the environment.

Such impure logic is split away (actor's logic)
and combined with relevant pieces of state (actor state),
thus preserving the purity (and sometimes simplicity) of the main state.

Actors can communicate with the main state by sending it messages,
and by receiving updates from it.
*/

// TODO: move crate::panel into crate::actors::panel.
// Panel contains state and logic to protect the main state from getting flooded
// with low-level wayland and gtk sizing events.

pub mod external;
pub mod popover;

/// The implementing actor is able to receive and handle messages.
/// Typically, it's the sending end of the channel,
/// whose other end is inside an event loop.
// TODO: implement for remaning actors and make the event loop refer to this.
pub trait Destination {
    type Event;
    fn send(&self, event: Self::Event);
}
