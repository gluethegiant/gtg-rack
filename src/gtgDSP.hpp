#pragma once
#include "plugin.hpp"


// simple fader for smoothing on off states and setting a common gain
struct AutoFader {

	bool on = true;
	bool temped = false;
	float fade = 0.f;
	int last_speed = 26;   // can be checked to see if a fade speed has changed

	void setSpeed(int speed) {   // uses sampleRate and gain to keep time consistent
		last_speed = speed;
		float sampleRate = APP->engine->getSampleRate();
		delta = gain/(sampleRate * 0.001f * (float)speed);   // milliseconds from 0 to full gain
	}

	void setGain(float amount) {
		gain = amount;
		setSpeed(last_speed);
	}

	float getGain() {
		return gain;
	}

	float getFade() {
		return fade;
	}

	float getExpFade(double power) {   // exponential curve on fade
		return std::pow(fade / gain, power) * gain;
	}

	void process() {   // increments or decreases fade value
		if (on) {
			if (fade < gain) {
				fade += delta;
				if (fade > gain) fade = gain;
			}
		} else {
			if (fade > 0.f) {
				fade -= delta;
				if (fade < 0.f) fade = 0.f;
			}
		}
	}

private:

	float delta = 0.001f;
	float gain = 1.f;
};


// constant power pan with optional smoothing
// set pan position with setPan() and then get levels for each channel with getLevel()

struct ConstantPan {

	float position = 0.f;   // pan position from -1.f to 1.f
	float levels[2] = {1.f, 1.f};   // left and right levels

	void setPan(float new_position) {
		if (new_position != position) {   // recalculates pan only after a change
			position = new_position;
			setLevels(position);
		}
	}

	void setSmoothSpeed(int speed) {   // uses sampleRate to keep smoothing speed consistent
		float sampleRate = APP->engine->getSampleRate();
		delta = 2.0f/(sampleRate * 0.001f * (float)speed);   // milliseconds from pan left to pan right
	}

	void setSmoothPan(float new_position) {
		if (new_position != position) {
			if (new_position > position)  {
				position = std::fmin(position + delta, new_position);
			} else {
				position = std::fmax(position - delta, new_position);
			}
			setLevels(position);
		}
	}

	float getLevel(int index) {
		return levels[index];
	}

private:

	float delta = 0.0005f;

	// efficient constant power pan law that adjusts center to 1.f and sounds nice
	void setLevels(float final_position) {
		float pan_angle = (final_position + 1.f) * 0.5f;
		levels[0] = sin((1.f - pan_angle) * M_PI_2) * M_SQRT2;   // left level
		levels[1] = sin(pan_angle * M_PI_2) * M_SQRT2;   // right level
	}
};


// a simple slew limiter that uses milliseconds

struct SimpleSlewer {

	float value = 0.f;

	float slew(float new_value) {
		if (new_value != value) {
			if (new_value > value) {
				value = std::fmin(value + delta, new_value);
			} else {
				value = std::fmax(value - delta, new_value);
			}
		}
		return value;
	}

	void setSlewSpeed(int speed) {
		float sampleRate = APP->engine->getSampleRate();
		delta = 1.f/(sampleRate * 0.001f * (float)speed);   // milliseconds from 0 to 1
	}

private:

	float delta = 0.0005f;
};
