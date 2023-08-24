# Current TODOs

## General

This list is ordered from current highest priority to lowest.
Naturally, these items may change as the progress progresses.

- [ ] **Figure out the broken jump in `cocos2d::CCAtlasNode::updateBlendFunc()`**
- [ ] Determine if [ANGLE](https://github.com/google/angle) is right for this project.
  - [ ] If so, find a way to include it without creating additional build steps
- [ ] Look into decent cross-platform windowing libraries
  - (GLFW looks promising)
- [ ] Add some implementation for the `^(:?v?s?n?|s?n?)printf$` functions
- [ ] Create a proper memory implementation.
  - May likely reuse C++'s `unsynchronized_pool_resource`

## Code quality

These can happen in any order, generally just as needed.

- [x] Rewrite the way call args are translated to allow implementing custom types easily
- [ ] Add some wrapper for pointers to emulator memory
- [ ] Create some form of compile time configuration
- [ ] Figure out how to unwind the stack
- [ ] Create a larger orchestrator class to manage the emulator context
- [ ] Separate most implementations from its headers
- [ ] Code formatting???
- [ ] Implement some sort of basic testing
- [ ] Isolate Geometry Dash and its implementation from the codebase
