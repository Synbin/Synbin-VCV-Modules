#pragma once

#include "rack.hpp"

using namespace rack;

namespace widgets {

	struct Knob32 : app::SvgKnob
	{
		Knob32();
	};

	struct ToggleSwitch2State : app::SvgSwitch
	{
		ToggleSwitch2State();
	};

	struct JackSocket : app::SvgPort
	{
		JackSocket();
	};

	struct PushButton : app::SvgSwitch
	{
		PushButton();
	};

} // namespace widgets
