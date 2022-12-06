/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*! Contains actors with custom event loops, not based off of the event_loop module. */
 
pub mod debug;
#[cfg(feature = "zbus_v1_5")]
pub mod screensaver;

/// The uninhabited type. Cannot be created or returned; means "will never return" as return type. Useful for infinite loops.
enum Void {}