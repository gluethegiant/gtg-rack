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
