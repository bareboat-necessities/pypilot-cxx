#include "pypilot/pypilot.hpp"

#include <iostream>

int main()
{
    pypilot::EventLoop loop;
    pypilot::PypilotState state;

    pypilot::ValueRegistry values;
    pypilot::PypilotServer server(loop, state, values);
    pypilot::Autopilot autopilot(loop, state);

    for (int i = 0; i < 3; ++i) {
        autopilot.iteration();
        std::string protocol_output = server.collect_output();
        (void)protocol_output;
        std::cout << "heading=" << state.ap.heading
                  << " command=" << state.ap.heading_command
                  << " error=" << state.ap.heading_error
                  << " servo=" << state.servo.command << '\n';
    }

    // TODO: replace this bounded demo with the long-running Linux server.
    return 0;
}
