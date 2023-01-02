/* Copyright (C) 2020,2022 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 */

/*! Glue for the main loop. */
use crate::actors;
use crate::actors::external::debug;
use crate::animation;
use crate::data::loading;
use crate::event_loop;
use crate::panel;
use crate::state;
use glib::{Continue, MainContext, PRIORITY_DEFAULT, Receiver};


mod c {
    use super::*;
    use std::ffi::CString;
    use std::os::raw::{c_char, c_void};
    use std::ptr;
    use std::rc::Rc;
    use std::time::Instant;

    use crate::actors::Destination;
    use crate::actors::popover;
    use crate::event_loop::driver;
    use crate::imservice::IMService;
    use crate::imservice::c::InputMethod;
    use crate::layout;
    use crate::outputs::Outputs;
    use crate::state;
    use crate::submission::Submission;
    use crate::util::c::{ArcWrapped, Wrapped};
    use crate::vkeyboard::c::ZwpVirtualKeyboardV1;
    
    /// DbusHandler*
    #[repr(transparent)]
    pub struct DBusHandler(*const c_void);

    /// EekboardContextService* in the role of a hint receiver
    // The clone/copy is a concession to C style of programming.
    // It would be hard to get rid of it.
    #[repr(transparent)]
    #[derive(Clone, Copy)]
    pub struct HintManager(*const c_void);
    
    /// Holds the Rust structures that are interesting from C.
    #[repr(C)]
    pub struct RsObjects {
        /// The handle to which Commands should be sent
        /// for processing in the main loop.
        receiver: Wrapped<Receiver<Commands>>,
        state_manager: Wrapped<EventLoop>,
        submission: Wrapped<Submission>,
        /// Not wrapped, because C needs to access this.
        wayland: *mut Wayland,
        popover: actors::popover::c::Actor,
    }

    /// Corresponds to wayland.h::squeek_wayland.
    /// Fields unused by Rust are marked as generic data types.
    #[repr(C)]
    pub struct Wayland {
        layer_shell: *const c_void,
        virtual_keyboard_manager: *const c_void,
        input_method_manager: *const c_void,
        outputs: Wrapped<Outputs>,
        seat: *const c_void,
        input_method: InputMethod,
        virtual_keyboard: ZwpVirtualKeyboardV1,
    }

    impl Wayland {
        fn new(outputs_manager: Outputs) -> Self {
            Wayland {
                layer_shell: ptr::null(),
                virtual_keyboard_manager: ptr::null(),
                input_method_manager: ptr::null(),
                outputs: Wrapped::new(outputs_manager),
                seat: ptr::null(),
                input_method: InputMethod::null(),
                virtual_keyboard: ZwpVirtualKeyboardV1::null(),
            }
        }
    }
    
    extern "C" {
        #[allow(improper_ctypes)]
        fn init_wayland(wayland: *mut Wayland);
        #[allow(improper_ctypes)]
        fn eekboard_context_service_set_layout(service: HintManager, name: *const c_char, layout: *const layout::Layout, timestamp: u32);
        // This should probably only get called from the gtk main loop,
        // given that dbus handler is using glib.
        fn dbus_handler_set_visible(dbus: *const DBusHandler, visible: u8);
    }
    
    // INITIALIZATION

    /// Creates what's possible in Rust to eliminate as many FFI calls as possible,
    /// because types aren't getting checked across their boundaries,
    /// and that leads to suffering.
    #[no_mangle]
    pub extern "C"
    fn squeek_init() -> RsObjects {
        // Set up channels
        let (sender, receiver) = MainContext::channel(PRIORITY_DEFAULT);
        let now = Instant::now();
        let state_manager = driver::Threaded::new(sender, state::Application::new(now));

        debug::init(state_manager.clone());

        let outputs = Outputs::new(state_manager.clone());
        let mut wayland = Box::new(Wayland::new(outputs));
        let wayland_raw = &mut *wayland as *mut _;
        unsafe { init_wayland(wayland_raw); }

        let vk = wayland.virtual_keyboard;

        let imservice = if wayland.input_method.is_null() {
            None
        } else {
            Some(IMService::new(wayland.input_method, state_manager.clone()))
        };
        let submission = Submission::new(vk, imservice);
        
        let popover = ArcWrapped::new(actors::popover::State::new(true));

        #[cfg(feature = "zbus_v1_5")]
        crate::actors::external::screensaver::init(popover.clone_ref());
        
        RsObjects {
            submission: Wrapped::new(submission),
            state_manager: Wrapped::new(state_manager),
            receiver: Wrapped::new(receiver),
            wayland: Box::into_raw(wayland),
             popover,
        }
    }

    /// Places the UI loop callback in the glib main loop.
    #[no_mangle]
    pub extern "C"
    fn register_ui_loop_handler(
        receiver: Wrapped<Receiver<Commands>>,
        panel_manager: panel::c::PanelManager,
        popover: actors::popover::c::Actor,
        hint_manager: HintManager,
        dbus_handler: *const DBusHandler,
    ) {
        let receiver = unsafe { receiver.unwrap() };
        let receiver = Rc::try_unwrap(receiver).expect("References still present");
        let receiver = receiver.into_inner();
        let panel_manager = Wrapped::new(panel::Manager::new(panel_manager));
        let ctx = MainContext::default();
        let _acqu = ctx.acquire();
        receiver.attach(
            Some(&ctx),
            move |msg| {
                main_loop_handle_message(
                    msg,
                    panel_manager.clone(),
                    &popover.clone_ref(),
                    hint_manager,
                    dbus_handler,
                );
                Continue(true)
            },
        );
        #[cfg(not(feature = "glib_v0_14"))]
        ctx.release();
    }

    /// A single iteration of the UI loop.
    /// Applies state outcomes to external portions of the program.
    /// This is the outest layer of the imperative shell,
    /// and doesn't lend itself to testing other than integration.
    fn main_loop_handle_message(
        msg: Commands,
        panel_manager: Wrapped<panel::Manager>,
        popover: &actors::popover::Destination,
        hint_manager: HintManager,
        dbus_handler: *const DBusHandler,
    ) {
        if let Some(visibility) = msg.panel_visibility {
            panel::Manager::update(panel_manager, visibility);
        }

        if let Some(visible) = msg.dbus_visible_set {
            if dbus_handler != std::ptr::null() {
                unsafe { dbus_handler_set_visible(dbus_handler, visible as u8) };
            }
        }
        
        if let Some(commands::SetLayout { description }) = msg.layout_selection {
            let animation::Contents {
                name,
                kind,
                overlay_name,
                purpose,
            } = description;
            popover.send(popover::Event::Overlay(overlay_name.clone()));
            let layout = loading::load_layout(&name, kind, purpose, &overlay_name);
            let layout = Box::into_raw(Box::new(layout));
            // CSS can't express "+" in the class
            let name = overlay_name.unwrap_or(name).replace('+', "_");
            let name = CString::new(name).unwrap_or(
                CString::new("").unwrap()
            );
            unsafe {
                // Take out the pointer to a temp variable so that it outlives the set_layout call.
                let name = name.as_ptr();
                eekboard_context_service_set_layout(hint_manager, name, layout, 0);
            }
        }
    }
    
    // EVENT PASSING    

    use crate::logging;
    use crate::state::{Event, Presence};
    use crate::state::LayoutChoice;
    use crate::state::visibility;
    use crate::util;
    
    use crate::logging::Warn;
    
    #[no_mangle]
    pub extern "C"
    fn squeek_state_send_force_visible(mgr: Wrapped<EventLoop>) {
        let sender = mgr.clone_ref();
        let sender = sender.borrow();
        sender.send(Event::Visibility(visibility::Event::ForceVisible))
            .or_warn(&mut logging::Print, logging::Problem::Warning, "Can't send to state manager");
    }
    
    #[no_mangle]
    pub extern "C"
    fn squeek_state_send_force_hidden(sender: Wrapped<EventLoop>) {
        let sender = sender.clone_ref();
        let sender = sender.borrow();
        sender.send(Event::Visibility(visibility::Event::ForceHidden))
            .or_warn(&mut logging::Print, logging::Problem::Warning, "Can't send to state manager");
    }

    #[no_mangle]
    pub extern "C"
    fn squeek_state_send_keyboard_present(sender: Wrapped<EventLoop>, present: u32) {
        let sender = sender.clone_ref();
        let sender = sender.borrow();
        let state =
            if present == 0 { Presence::Missing }
            else { Presence::Present };
        sender.send(Event::PhysicalKeyboard(state))
            .or_warn(&mut logging::Print, logging::Problem::Warning, "Can't send to state manager");
    }
    
    #[no_mangle]
    pub extern "C"
    fn squeek_state_send_layout_set(
        sender: Wrapped<EventLoop>,
        name: *const c_char,
        source: *const c_char,
        // TODO: use when synthetic events are needed
        _timestamp: u32,
    ) {
        let sender = sender.clone_ref();
        let sender = sender.borrow();
        let string_or_empty = |v| String::from(
            util::c::as_str(v)
            .unwrap_or(Some(""))
            .unwrap_or("")
        );
        sender
            .send(Event::LayoutChoice(LayoutChoice {
                name: string_or_empty(&name),
                source: string_or_empty(&source).into(),
            }))
            .or_warn(&mut logging::Print, logging::Problem::Warning, "Can't send to state manager");
    }
}


pub type EventLoop = event_loop::driver::Threaded<state::Application>;


pub mod commands {
    use crate::animation;
    #[derive(Clone, Debug)]
    pub struct SetLayout {
        pub description: animation::Contents,
    }
}

/// The commands consumed by the main loop,
/// to be sent out to external components.
#[derive(Clone)]
pub struct Commands {
    pub panel_visibility: Option<panel::Command>,
    pub dbus_visible_set: Option<bool>,
    pub layout_selection: Option<commands::SetLayout>,
}
