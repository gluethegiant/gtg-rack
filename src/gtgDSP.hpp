#include "plugin.hpp"


struct AutoFader {

	bool on = true;

	void setSpeed(int speed) {
		last_speed = speed;
		float sampleRate = APP->engine->getSampleRate();
		delta = gain/(sampleRate * 0.001f * speed);
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

	void process() {
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
	float fade = 0.f;
	float delta = 0.001f;
	float gain = 1.f;

};

struct ConstantPanner {

	float position = 0.f;
	float levels[2] = {1.f, 1.f};

	void setPan(float new_position) {
		if (new_position != position) {
			position = new_position;
			float pan_angle = (position + 1.f) * 0.5f;
			levels[0] = sin((1.f - pan_angle) * M_PI_2) * M_SQRT2;
			levels[1] = sin(pan_angle * M_PI_2) * M_SQRT2;
		}
	}

	float getLevel(int index) {
		return levels[index];
	}
};
