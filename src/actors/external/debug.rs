/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
use crate::main;
use crate::state;

use std::thread;
use zbus::{Connection, ObjectServer, dbus_interface, fdo};

use super::Void;

use std::convert::TryInto;


/// Accepts commands controlling the debug mode
struct Manager {
    sender: main::EventLoop,
    enabled: bool,
}

#[dbus_interface(name = "sm.puri.SqueekDebug")]
impl Manager {
    #[dbus_interface(property, name = "Enabled")]
    fn get_enabled(&self) -> bool {
        self.enabled
    }
    #[dbus_interface(property, name = "Enabled")]
    fn set_enabled(&mut self, enabled: bool) {
        self.enabled = enabled;
        self.sender
            .send(state::Event::Debug(
                if enabled { Event::Enable }
                else { Event::Disable }
            ))
            .unwrap();
    }
}

fn start(mgr: Manager) -> Result<Void, Box<dyn std::error::Error>> {
    let connection = Connection::new_session()?;
    fdo::DBusProxy::new(&connection)?.request_name(
        "sm.puri.SqueekDebug",
        fdo::RequestNameFlags::ReplaceExisting.into(),
    )?;

    let mut object_server = ObjectServer::new(&connection);
    object_server.at(&"/sm/puri/SqueekDebug".try_into()?, mgr)?;

    loop {
        if let Err(err) = object_server.try_handle_next() {
            eprintln!("{}", err);
        }
    }
}

pub fn init(sender: main::EventLoop) {
    let mgr = Manager {
        sender,
        enabled: false,
    };
    thread::spawn(move || {
        start(mgr).unwrap();
    });
}

#[derive(Debug, Clone, Copy)]
pub enum Event {
    Enable,
    Disable,
}
