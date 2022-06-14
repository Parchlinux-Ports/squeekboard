/*! Defines the application-wide message bus for updating state.*/

use crate::event_loop::driver::Threaded;

pub mod c {
    use super::*;
    use crate::util::c::Wrapped;
    pub type State = Wrapped<Threaded>;
}

// The state receiver is an endpoint of a channel, so it's safely cloneable.
// There's no need to keep it in a Rc.
// The C version uses Wrapped with an underlying Rc,
// because Wrapped is well-tested already.
pub type State = Threaded;