# pypilot-cxx

pypilot-cxx is a top-down C++ translation of the core Python PyPilot autopilot server.

The project intentionally keeps the source organization close to original Python PyPilot:

- src/pypilot/*.cpp corresponds to original pypilot/*.py modules.
- src/pypilot/pilots/*.cpp corresponds to original pypilot/pilots/*.py modules.
- src/pypilot/syslib/*.hpp contains consolidated header-only support code for runtime/event-loop, typed model, and NMEA 0183 parsing.

This first version is a buildable skeleton. Missing translated behavior is marked with TODO comments in code and tracked in PLAN.md.

Build:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

Current status:

The current code compiles and exercises the initial value registry, heading wrapping, NMEA checksum/parsing shell, pilot shell, and top-level iteration shell.

Design constraints:

- Keep .cpp files named after original Python files.
- Translate from top-level behavior downward.
- Do not start with low-level math/calibration internals.
- Keep Linux and Arduino portability in syslib/platform.hpp.
- Use explicit TODO markers for missing translated behavior.
