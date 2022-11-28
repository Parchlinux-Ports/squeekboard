/* Copyright (C) 2021 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 */
 
/*! This drives the loop from the `loop` module.
 * 
 * The tracker loop needs to be driven somehow,
 * and connected to the external world,
 * both on the side of receiving and sending events.
 * 
 * That's going to be implementation-dependent,
 * connecting to some external mechanisms
 * for time, messages, and threading/callbacks.
 * 
 * This is the "imperative shell" part of the software,
 * and no longer unit-testable.
 */

use crate::event_loop;
use crate::logging;
use glib;
use std::sync::mpsc;
use std::thread;
use std::time::Instant;
use super::{ActorState, Outcome};

// Traits
use crate::logging::Warn;
use super::Event;


type UISender<S> = glib::Sender<
    <
        <S as ActorState>::Outcome as Outcome
    >::Commands
>;

/// This loop driver spawns a new thread which updates the state in a loop,
/// in response to incoming events.
/// It sends outcomes to the glib main loop using a channel.
/// The outcomes are applied by the UI end of the channel in the `main` module.
// This could still be reasonably tested,
/// by creating a glib::Sender and checking what messages it receives.
// This can/should be abstracted over Event and Commands,
// so that the C call-ins can be thrown away from here and defined near events.
#[derive(Clone)]
pub struct Threaded<S>
where
    S: ActorState + Send,
    S::Event: Send,
    <S::Outcome as Outcome>::Commands: Send,
{
    /// Waits for external events
    thread: mpsc::Sender<S::Event>,
}

impl<S> Threaded<S>
where
    // Not sure why this needs 'static. It's already owned.
    S: ActorState + Send + 'static,
    S::Event: Send,
    <S::Outcome as Outcome>::Commands: Send,
{
    pub fn new(
        ui: UISender<S>,
        initial_state: S,
    ) -> Self {
        let (sender, receiver) = mpsc::channel();
        let saved_sender = sender.clone();
        thread::spawn(move || {
            let mut state = event_loop::State::new(initial_state, Instant::now());
            loop {
                match receiver.recv() {
                    Ok(event) => {
                        state = Self::handle_loop_event(&sender, state, event, &ui);
                    },
                    Err(e) => {
                        logging::print(logging::Level::Bug, &format!("Senders hung up, aborting: {}", e));
                        return;
                    },
                };
            }
        });

        Self {
            thread: saved_sender,
        }
    }
    
    pub fn send(&self, event: S::Event) -> Result<(), mpsc::SendError<S::Event>> {
        self.thread.send(event)
    }
    
    fn handle_loop_event(
        loop_sender: &mpsc::Sender<S::Event>,
        state: event_loop::State<S>,
        event: S::Event, 
        ui: &UISender<S>,
    ) -> event_loop::State<S> {
        let now = Instant::now();

        let (new_state, commands) = event_loop::handle_event(state.clone(), event, now);

        ui.send(commands)
            .or_warn(&mut logging::Print, logging::Problem::Bug, "Can't send to UI");

        if new_state.scheduled_wakeup != state.scheduled_wakeup {
            if let Some(when) = new_state.scheduled_wakeup {
                Self::schedule_timeout_wake(loop_sender, when);
            }
        }
        
        new_state
    }

    fn schedule_timeout_wake(
        loop_sender: &mpsc::Sender<S::Event>,
        when: Instant,
    ) {
        let sender = loop_sender.clone();
        thread::spawn(move || {
            let now = Instant::now();
            thread::sleep(when - now);
            sender.send(S::Event::new_timeout_reached(when))
                .or_warn(&mut logging::Print, logging::Problem::Warning, "Can't wake manager");
        });
    }
}
