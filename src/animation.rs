/* Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 */

/*! Animation details */

use std::time::Duration;

use crate::outputs::OutputId;
use crate::panel::PixelSize;
use crate::layout::ArrangementKind;

/// The keyboard should hide after this has elapsed to prevent flickering.
pub const HIDING_TIMEOUT: Duration = Duration::from_millis(200);

/// Panel contents
#[derive(PartialEq, Clone, Debug)]
pub struct Contents {
    pub name: String,
    pub kind: ArrangementKind,
    pub overlay_name: Option<String>,
}

/// The outwardly visible state of visibility
#[derive(PartialEq, Debug, Clone)]
pub enum Outcome {
    Visible {
        output: OutputId,
        height: PixelSize,
        contents: Contents,
    },
    Hidden,
}
