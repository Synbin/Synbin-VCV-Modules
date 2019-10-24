#include "plugin.hpp"
#include "widgets.hpp"

using namespace widgets;

Knob32::Knob32()
{
	setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knob_15mm.svg")));
	minAngle = (-240.0 * M_PI) / 180.0;
	maxAngle = (60.0 * M_PI) / 180.0;
}

ToggleSwitch2State::ToggleSwitch2State()
{
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/toggle_switch_0.svg")));
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/toggle_switch_1.svg")));
}

JackSocket::JackSocket()
{
	setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/JackSocket.svg")));
}

PushButton::PushButton()
{
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/red_button_0.svg")));
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/red_button_1.svg")));
	momentary = true;
}
