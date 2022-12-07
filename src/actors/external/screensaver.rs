/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
use crate::actors::Destination;
use crate::actors::popover;
use crate::logging;
use std::thread;
use zbus::{Connection, dbus_proxy};

use super::Void;


#[dbus_proxy(
    interface = "org.freedesktop.ScreenSaver",
    default_service = "org.freedesktop.ScreenSaver",
    default_path = "/org/freedesktop/ScreenSaver"
)]
pub trait Manager {
    #[dbus_proxy(signal)]
    fn active_changed(&self, active: bool) -> fdo::Result<()>;
}

/// Listens to screensaver (screen lock) changes
pub fn init(destination: popover::Destination) {
    thread::spawn(move || {
        if let Err(e) = start(destination) {
            log_print!(
                logging::Level::Surprise,
                "Could not track screensaver status, giving up: {:?}",
                e,
            );
        }
    });
}

fn start(destination: popover::Destination) -> Result<Void, zbus::Error> {
    let conn = Connection::new_session()?;
    let manager = ManagerProxy::new(&conn)?;

    manager.connect_active_changed(move |m| {
        destination.send(popover::Event::ScreensaverActive(m));
        Ok(())
    })?;

    loop {
        match manager.next_signal() {
            Ok(None) => {}
            other => log_print!(
                logging::Level::Bug,
                "Encountered unhandled event: {:?}",
                other,
            ),
        }
    }
}