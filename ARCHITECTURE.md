# Architecture notes

`PypilotState` is the only runtime state store.

`ValueRegistry` is a protocol-facing adapter for PyPilot clients. It maps external string names to typed fields, formats values, and tracks client watches. It must not own control state.

`Autopilot` is a control object. It reads and writes `PypilotState` directly and must not depend on `ValueRegistry`, `PypilotServer`, or `PypilotClient`.
