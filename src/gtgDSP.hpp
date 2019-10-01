#include "plugin.hpp"


// simple fader for smoothing on off states and setting a common gain
struct AutoFader {

	bool on = true;
	float fade = 0.f;

	void setSpeed(int speed) {   // uses sampleRate and gain to keep time consistent
		last_speed = speed;
		float sampleRate = APP->engine->getSampleRate();
		delta = gain/(sampleRate * 0.001f * speed);   // milliseconds from 0 to full gain
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

	void process() {   // increments or decreases fade value
		if (on) {
			if (fade < gain) {
				fade += delta;
			} else {
				fade = gain;
			}
		} else {
			if (fade > 0) {
				fade -= delta;
			} else {
				fade = 0.f;
			}
		}
	}

private:

	int last_speed = 20;
	float delta = 0.001f;
	float gain = 1.f;
};


// constant power pan with optional smoothing
struct ConstantPan {

	float position = 0.f;
	float levels[2] = {1.f, 1.f};

	void setPan(float new_position) {
		if (new_position != position) {   // recalculates pan only after a change
			position = new_position;
			setLevels(position);
		}
	}

	void setSmoothSpeed(int speed) {   // uses sampleRate to keep smoothing consistent
		float sampleRate = APP->engine->getSampleRate();
		delta = 2.0f/(sampleRate * 0.001f * speed);   // milliseconds from pan left to pan right
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

	void setLevels(float final_position) {
		float pan_angle = (final_position + 1.f) * 0.5f;
		levels[0] = sin((1.f - pan_angle) * M_PI_2) * M_SQRT2;
		levels[1] = sin(pan_angle * M_PI_2) * M_SQRT2;
	}
};
