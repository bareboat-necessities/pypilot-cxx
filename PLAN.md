# pypilot-cxx plan

This file tracks the top-down C++ translation of Python PyPilot into a compact Linux/Arduino-capable project.

## Source organization rules

- `src/pypilot/*.cpp` mirrors original `pypilot/*.py` modules.
- `src/pypilot/pilots/*.cpp` mirrors original `pypilot/pilots/*.py` modules.
- `src/pypilot/syslib/*.hpp` contains consolidated header-only support libraries.
- Prefer a small number of headers; do not create one header per Python module unless necessary.
- Mark incomplete translated behavior with `// TODO` in the code.

## Phase 0 — repository skeleton

Status: started.

- [x] CMake project.
- [x] GitHub Actions build workflow.
- [x] PyPilot-shaped source tree.
- [x] Consolidated `syslib` headers.
- [x] Initial tests.
- [ ] Add upstream PyPilot attribution notes for translated files.

## Phase 0.5 — runtime/syslib foundation

Status: scaffolded.

- [x] `syslib/event_loop.hpp` with cooperative timers.
- [x] `syslib/event_loop_linux.hpp` with Linux clock/sleep shell.
- [x] `syslib/event_loop_arduino.hpp` with Arduino clock/sleep shell.
- [x] `syslib/data_model.hpp` with initial typed PyPilot state.
- [x] `syslib/nmea0183.hpp` with checksum/parser shell.
- [ ] Fold in more tested logic from the uploaded event-loop idea source.
- [ ] Add bounded wait/poll API equivalent to Python `select.poll(timeout)` usage.
- [ ] Add real Linux TCP/UDP/serial backends.
- [ ] Add Arduino Serial/WiFi stream backends.

## Phase 1 — values/server/client

Status: in progress.

- [x] `values.cpp` typed descriptor registry over `PypilotState` instead of string-value storage.
- [x] External string names are now protocol metadata only; storage lives in typed structures.
- [x] `server.cpp` direct line-protocol parser for list/get/set/watch.
- [x] Immediate and periodic watch queues over typed fields.
- [x] `client.cpp` outbound watch/set shell and inbound typed-field apply.
- [ ] Translate full `values.py` property metadata and exact JSON output compatibility.
- [ ] Add persistence/profile behavior from `server.py`.
- [ ] Translate TCP server behavior from `server.py`.
- [ ] Translate reconnect/watch behavior from `client.py`.

## Phase 2 — top-level autopilot loop

Status: scaffolded.

- [x] `autopilot.cpp` owns top-level iteration order.
- [x] Main loop example.
- [ ] Translate original `autopilot.py` iteration behavior line-by-line.
- [ ] Preserve explicit IMU-read, pilot compute, servo command, maintenance order.
- [ ] Translate mode adjustment and heading command rules.
- [ ] Translate compass calibration-change handling.

## Phase 3 — pilots

Status: scaffolded.

- [x] `pilots/pilot.cpp` base shell.
- [x] `basic.cpp`, `simple.cpp`, `gps.cpp`, `wind.cpp` shells.
- [x] Remaining pilot filenames present.
- [ ] Translate actual pilot gains and command computation.
- [ ] Add tests for heading-error sign and servo command direction.
- [ ] Translate learning/autotune behavior only after core pilots work.

## Phase 4 — sensors/NMEA/servo/boatimu

Status: scaffolded.

- [x] `nmea.cpp` wraps NMEA parser shell.
- [x] `sensors.cpp` source-priority shell.
- [x] `servo.cpp`, `boatimu.cpp`, `rudder.cpp`, `tacking.cpp` shells.
- [ ] Translate `sensors.py` source priority/timeouts.
- [ ] Translate APB route behavior and 2 Hz gate.
- [ ] Translate NMEA bridge/output formatting.
- [ ] Translate servo state/fault/current/voltage protocol.
- [ ] Translate BoatIMU runtime interface before low-level IMU math.

## Phase 5 — math/IMU/calibration

Status: not started.

- [ ] Add Eigen dependency.
- [ ] Translate only math needed by top-level behavior first.
- [ ] Add quaternion/vector/calibration code incrementally.
- [ ] Add deterministic tests from known PyPilot behavior.

## Phase 6 — Linux and Arduino targets

Status: not started.

- [ ] Linux socket/serial runtime.
- [ ] Arduino compile target.
- [ ] Arduino Serial/WiFi transport.
- [ ] Hardware IMU/servo backends.
