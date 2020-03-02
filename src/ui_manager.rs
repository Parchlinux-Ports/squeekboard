/* Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 */

/*! Centrally manages the shape of the UI widgets, and the choice of layout.
 * 
 * Coordinates this based on information collated from all possible sources.
 */

use std::cell::RefCell;
use std::cmp::min;
use std::rc::Rc;

use ::logging;
use ::outputs::{ OutputId, Outputs, OutputState};
use ::outputs::c::OutputHandle;

// Traits
use ::logging::Warn;

mod c {
    use super::*;
    use std::os::raw::c_void;
    use ::outputs::c::COutputs;
    use ::util::c::Wrapped;

    #[derive(Clone, Copy)]
    #[repr(C)]
    pub struct PhoshLayerSurface(*const c_void);

    extern "C" {
        // Rustc wrongly assumes
        // that COutputs allows C direct access to the underlying RefCell.
        #[allow(improper_ctypes)]
        pub fn squeek_manager_set_surface_height(
            surface: PhoshLayerSurface,
            height: u32,
        );
    }

    #[no_mangle]
    pub extern "C"
    fn squeek_uiman_new(outputs: COutputs) -> Wrapped<Manager> {
        let uiman_raw = Wrapped::new(Manager::new());
        if !outputs.is_null() {
            let uiman = uiman_raw.clone_ref();
            let outputs = outputs.clone_ref();
            let mut outputs = outputs.borrow_mut();
            register_output_man(uiman, &mut outputs);
        }
        uiman_raw
    }

    /// Used to size the layer surface containing all the OSK widgets.
    #[no_mangle]
    pub extern "C"
    fn squeek_uiman_get_perceptual_height(
        uiman: Wrapped<Manager>,
    ) -> u32 {
        let uiman = uiman.clone_ref();
        let uiman = uiman.borrow();
        // TODO: what to do when there's no output?
        uiman.state.get_perceptual_height().unwrap_or(0)
    }

    #[no_mangle]
    pub extern "C"
    fn squeek_uiman_set_output(
        uiman: Wrapped<Manager>,
        output: OutputHandle,
    ) {
        let uiman = uiman.clone_ref();
        let mut uiman = uiman.borrow_mut();
        uiman.set_output(output)
    }

    #[no_mangle]
    pub extern "C"
    fn squeek_uiman_set_surface(
        uiman: Wrapped<Manager>,
        surface: PhoshLayerSurface,
    ) {
        let uiman = uiman.clone_ref();
        let mut uiman = uiman.borrow_mut();
        // Surface is not state, so doesn't need to propagate updates.
        uiman.surface = Some(surface);
    }
}

/// Stores current state of all things influencing what the UI should look like.
#[derive(Clone, PartialEq)]
pub struct ManagerState {
    current_output: Option<(OutputId, OutputState)>,
    //// Pixel size of the surface. Needs explicit updating.
    //surface_size: Option<Size>,
}

impl ManagerState {
    /// The largest ideal heigth for the keyboard as a whole
    /// judged by the ease of hitting targets within.
    /// Ideally related to finger size, the crammedness of the layout,
    /// distance from display, and motor skills of the user.
    // FIXME: Start by making this aware of display's dpi,
    // then layout number of rows.
    fn get_max_target_height(output: &OutputState) -> u32 {
        let layout_rows = 4; // FIXME: use number from layout.
        let px_size = output.get_pixel_size();
        let phys_size = output.get_phys_size();

        let finger_height_px = match (px_size, phys_size) {
            (Some(px_size), Some(phys_size)) => {
                // Fudged to result in 420px from the original design.
                // That gives about 9.5mm per finger height.
                // Maybe floats are not the best choice here,
                // but it gets rounded ASAP. Consider rationals.
                let keyboard_fraction_of_display: f64 = 420. / 1440.;
                let keyboard_mm = keyboard_fraction_of_display * 130.;
                let finger_height_mm = keyboard_mm / 4.;
                // TODO: Take into account target shape/area, not just height.
                finger_height_mm * px_size.height as f64 / phys_size.height as f64
            },
            (_, None) => output.scale as f64 * 52.5, // match total 420px at scale 2 from original design
            (None, Some(_)) => {
                log_print!(
                    logging::Level::Surprise,
                    "Output has physical size data but no pixel info",
                );
                output.scale as f64 * 52.5
            },
        };
        (layout_rows as f64 * finger_height_px) as u32
    }

    fn get_perceptual_height(&self) -> Option<u32> {
        let output_info = (&self.current_output).as_ref()
            .map(|(_id, os)| (
                os.scale as u32,
                os.get_pixel_size(),
                ManagerState::get_max_target_height(&os),
            ));
        match output_info {
            Some((scale, Some(px_size), target_height)) => Some({
                let height = if (px_size.width < 720) & (px_size.width > 0) {
                    px_size.width * 7 / 12 // to match 360×210
                } else if px_size.width < 1080 {
                    360 + (1080 - px_size.width) * 60 / 360 // smooth transition
                } else {
                    360
                };

                // Don't exceed half the display size.
                let height = min(height, px_size.height / 2);
                // Don't waste screen space by exceeding best target height.
                let height = min(height, target_height);
                height / scale
            }),
            Some((scale, None, _)) => Some(360 / scale),
            None => None,
        }
    }
}

pub struct Manager {
    state: ManagerState,
    surface: Option<c::PhoshLayerSurface>,
}

impl Manager {
    fn new() -> Manager {
        Manager {
            state: ManagerState { current_output: None, },
            surface: None,
        }
    }
    fn set_output(&mut self, output: OutputHandle) {
        let output_state = output.get_state()
            .or_warn(
                &mut logging::Print,
                logging::Problem::Bug,
                // This is really bad. It only happens when the layer surface
                // is placed, and it happens once.
                // The layer surface is on an output that can't be tracked.
                "Tried to set output that's not known to exist. Ignoring.",
            );
        self.state.current_output = output_state.map(
            |state| (output.get_id(), state)
        );
        // TODO: At the time of writing, this function is only used once,
        // before the layer surface is initialized.
        // Therefore it doesn't update anything. Maybe it should in the future,
        // if it sees more use.
    }

    fn handle_output_change(&mut self, output: OutputHandle) {
        let (id, output_state) = match &self.state.current_output {
            Some((id, state)) => {
                if *id != output.get_id() { return } // Not the current output.
                else { (id, state) }
            },
            None => return, // Keyboard isn't on any output.
        };
        if let Some(new_output_state) = output.get_state() {
            if new_output_state != *output_state {
                let new_state = ManagerState {
                    current_output: Some((id.clone(), new_output_state)),
                    ..self.state.clone()
                };
                if let Some(surface) = &self.surface {
                    let new_height = new_state.get_perceptual_height();
                    if new_height != self.state.get_perceptual_height() {
                        // TODO: here hard-size the keyboard and suggestion box too.
                        match new_height {
                            Some(new_height) => unsafe {
                                c::squeek_manager_set_surface_height(
                                    *surface,
                                    new_height,
                                )
                            }
                            None => log_print!(
                                logging::Level::Bug,
                                "Can't calculate new size",
                            ),
                        }
                    }
                }
                self.state = new_state;
            }
        };
    }
}

fn register_output_man(
    ui_man: Rc<RefCell<Manager>>,
    output_man: &mut Outputs,
) {
    let ui_man = ui_man.clone();
    output_man.set_update_cb(Box::new(move |output: OutputHandle| {
        let mut ui_man = ui_man.borrow_mut();
        ui_man.handle_output_change(output)
    }))
}
