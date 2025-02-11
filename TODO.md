# Current TODOs

## General

This list is ordered from current highest priority to lowest.
Naturally, these items may change as the progress progresses.

- [ ] Fix the many OpenGL related bugs
- [ ] Sound!
  - [ ] Find something that offers the same metering API that FMOD does... maybe [SoLoud](https://solhsa.com/soloud/)
- [ ] Improve the implementation for the `^(:?v?s?n?|s?n?)printf$` functions and `sscanf`
- [ ] Create a proper memory implementation. The current one is slow...

## Code quality

These can happen in any order, generally just as needed.

- [ ] Add some wrapper for pointers to emulator memory
- [ ] Create some form of compile time configuration
- [ ] Figure out how to unwind the stack
- [ ] Create a larger orchestrator class to manage the emulator context
- [ ] Separate most implementations from its headers
- [ ] Code formatting???
- [ ] Implement some sort of basic testing
- [ ] Isolate Geometry Dash and its implementation from the codebase
- [ ] Keyboard support on 1.9+
- [ ] Async loading
- [ ] Native JNI integration
