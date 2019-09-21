#include "plugin.hpp"
#include "components.hpp"


#define HISTORY_CAP 512000

struct MetroCityBus : Module {
	enum ParamIds {
		ON_PARAM,
		SPREAD_PARAM,
		PAN_ATT_PARAM,
		REVERSE_PARAM,
		PAN_PARAM,
		ENUMS(LEVEL_PARAMS, 3),
		BLUE_POST_PARAM,
		ORANGE_POST_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		POLY_INPUT,
		ON_CV_INPUT,
		PAN_CV_INPUT,
		ENUMS(LEVEL_CV_INPUTS, 3),
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ON_LIGHT,
		ENUMS(PAN_LIGHTS, 9),
		REVERSE_LIGHT,
		BLUE_POST_LIGHT,
		ORANGE_POST_LIGHT,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger on_trigger;
	dsp::SchmittTrigger on_cv_trigger;
	dsp::SchmittTrigger reverse_poly_trigger;
	dsp::SchmittTrigger blue_post_trigger;
	dsp::SchmittTrigger orange_post_trigger;
	dsp::ClockDivider pan_divider;
	dsp::ClockDivider light_divider;

	float pan_history[HISTORY_CAP] = {};
	long hist_i = 0;
	long hist_size = 0;
	bool input_on = true;
	float onramp = 0.f;
	bool reverse_poly = false;
	bool post_fades[2] = {false, false};
	float pan_pos = 0.f;
	float pan_levels[32] = {};
	float spread_pos = 0.f;
	int channel_no = 0;
	float channel_pan[16] = {};
	float light_brights[9] = {};
	long f_delay = 0;

	MetroCityBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(SPREAD_PARAM, -1.f, 1.f, 0.f, "Polyphonic stereo spread");
		configParam(PAN_ATT_PARAM, 0.f, 1.f, 0.5f, "Pan attenuator");
		configParam(REVERSE_PARAM, 0.f, 1.f, 0.f, "Reverse pan order of polyphonic channels");
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Level to blue stereo bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Level to orange stereo bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Level to red stereo bus");
		configParam(BLUE_POST_PARAM, 0.f, 1.f, 0.f, "Post red fader send");
		configParam(ORANGE_POST_PARAM, 0.f, 1.f, 0.f, "Post red fader send");
		pan_divider.setDivision(3);
		light_divider.setDivision(64);
	}

	void process(const ProcessArgs &args) override {
		// on off button with pop filter
		if (on_trigger.process(params[ON_PARAM].getValue()) + on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			if (input_on) {
				input_on = false;
				onramp = 1;
				for (int l = 0; l < 9; l++) lights[PAN_LIGHTS + l].value = 0;   // turn off pan lights
			} else {
				input_on = true;
				onramp = 0;
			}
		}

		if (input_on) {   // calculate pop filter speed with current sampleRate
			if (onramp < 1) onramp += 50.f / args.sampleRate;
		} else {
			if (onramp > 0) onramp -= 50.f / args.sampleRate;
		}

		lights[ON_LIGHT].value = onramp;

		// get button click to reverse polyphonic pan order
		if (reverse_poly_trigger.process(params[REVERSE_PARAM].getValue())) reverse_poly = !reverse_poly;

		lights[REVERSE_LIGHT].value = reverse_poly;

		// post fader send buttons
		if (blue_post_trigger.process(params[BLUE_POST_PARAM].getValue())) post_fades[0] = !post_fades[0];
		if (orange_post_trigger.process(params[ORANGE_POST_PARAM].getValue())) post_fades[1] = !post_fades[1];

		lights[BLUE_POST_LIGHT].value = post_fades[0];
		lights[ORANGE_POST_LIGHT].value = post_fades[1];

		// get level knobs
		float in_levels[3] = {0.f, 0.f, 0.f};
		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus
			in_levels[sb] = clamp(inputs[LEVEL_CV_INPUTS + sb].getNormalVoltage(10) * 0.1f, 0.f, 1.f) * params[LEVEL_PARAMS + sb].getValue();
		}

		// set post fades on levels
		for (int i = 0; i < 2; i++) {
			if (post_fades[i]) {
				in_levels[i] *= in_levels[2];
			}
		}

		// pans
		if (pan_divider.process() && input_on) {   // calculate pan every few samples

			if (inputs[PAN_CV_INPUT].isConnected()) {   // create follow pan when CV connected
				float pan_delta = 25.f / args.sampleRate;   // for pan smoothing
				// get pan knob with CV and attenuator
				channel_no = inputs[POLY_INPUT].getChannels();
				pan_pos = params[PAN_PARAM].getValue() + (((inputs[PAN_CV_INPUT].getNormalVoltage(0) * 2) * params[PAN_ATT_PARAM].getValue()) * 0.1f);
				spread_pos = std::abs(params[SPREAD_PARAM].getValue());

				// Store pan history
				if (hist_i > HISTORY_CAP) hist_i = 0;   // reset history buffer index
				pan_history[hist_i] = pan_pos;

				// Calculate delay for follow
				f_delay = std::round(spread_pos * (args.sampleRate / pan_divider.getDivision()));   // f_delay * 16 should not be more than HISTORY_CAP

				channel_pan[0] = pan_pos;   // first channel position is current pan position
				float pan_angle = (channel_pan[0] + 1) * 0.5f;
				pan_levels[0] = get_left(pan_angle);
				pan_levels[1] = get_right(pan_angle);

				// calculate pan position for other channels
				for (int c = 1; c < channel_no; c++) {
					int sc = c * 2;
					long follow = c * f_delay;
					if (follow > hist_size) {   // there is not enough history to follow
						channel_pan[c] = channel_pan[0];   // follow first channel until history is populated
						pan_levels[sc] = pan_levels[0];
						pan_levels[sc + 1] = pan_levels[1];
					} else {
						follow = hist_i - follow;
						if (follow < 0) follow = HISTORY_CAP + follow;   // fix follow when buffer resets to 0
						float last_pan = channel_pan[c];
						float new_pan = pan_history[follow];
						// smooth pan for dynamic channels and history catch up
						channel_pan[c] = smooth_pan(last_pan, pan_delta, new_pan);

						pan_angle = (channel_pan[c] + 1) * 0.5f;
						pan_levels[sc] = get_left(pan_angle);
						pan_levels[sc + 1] = get_right(pan_angle);
					}
				}

				hist_i++;   // Keep history buffer rolling
				if (hist_size < HISTORY_CAP) hist_size++;

			} else {   // create spread pan when no CV connected
				hist_size = 0; hist_i = 0;   // reset pan history when CV not connected
				float pan_delta = 18.f / args.sampleRate;   // remember pan_divider clock moves pan infrequently
				int new_channel_no = inputs[POLY_INPUT].getChannels();
				float new_pan_pos = params[PAN_PARAM].getValue();
				float new_spread_pos = params[SPREAD_PARAM].getValue();
				if (new_pan_pos != pan_pos || new_spread_pos != spread_pos || new_channel_no != channel_no) {   // only calculate pan if something has changed
					pan_pos = new_pan_pos; spread_pos = new_spread_pos; channel_no = new_channel_no;

					// Spread evenly over available stereo field
					float pan_spread = 0.f;
					if (spread_pos < 0) pan_spread = (pan_pos + 1) * spread_pos;
					if (spread_pos > 0) pan_spread = -1 * ((pan_pos - 1) * spread_pos);

					channel_pan[0] = pan_pos;   // first channel position is current pan
					float pan_angle = (channel_pan[0] + 1) * 0.5f;
					pan_levels[0] = get_left(pan_angle);
					pan_levels[1] = get_right(pan_angle);

					// calculate polyphonic spread and pan levels for other channels
					for (int c = 1; c < channel_no; c++) {
						float last_pan = channel_pan[c];
						channel_pan[c] = pan_pos + (((float)c / (float)(channel_no - 1)) * pan_spread);

						// smooth pan for dynamic channels
						channel_pan[c] = smooth_pan(last_pan, pan_delta, channel_pan[c]);

						pan_angle = (channel_pan[c] + 1) * 0.5f;
						pan_levels[c * 2] = get_left(pan_angle);
						pan_levels[(c * 2) + 1] = get_right(pan_angle);
					}
				}
			}
		}   // end pan_divider.process()

		// process inputs
		float stereo_in[2] = {0.f, 0.f};
		if (spread_pos == 0) {   // sum channels if no spread
			float sum_in = inputs[POLY_INPUT].getVoltageSum();
			for (int c = 0; c < 2; c++) {
				stereo_in[c] = sum_in * pan_levels[c] * onramp;
			}
		} else {
			for (int c = 0; c < channel_no; c++) {
				int sc = c * 2;
				float channel_in = inputs[POLY_INPUT].getPolyVoltage(c);
				if (reverse_poly) {   // reverses order of levels applied to channels
					stereo_in[0] += channel_in * pan_levels[(channel_no * 2) - 2 - sc];
					stereo_in[1] += channel_in * pan_levels[(channel_no * 2) - 1 - sc];
				} else {
					stereo_in[0] += channel_in * pan_levels[sc];
					stereo_in[1] += channel_in * pan_levels[sc + 1];
				}
			}
			stereo_in[0] *= onramp;
			stereo_in[1] *= onramp;
		}

		// set bus outputs for 3 stereo buses out
		outputs[BUS_OUTPUT].setChannels(6);

		// process bus outputs
		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus
			for (int c = 0; c < 2; c++) {
				int bus_channel = (2 * sb) + c;
				outputs[BUS_OUTPUT].setVoltage((stereo_in[c] * in_levels[sb]) + inputs[BUS_INPUT].getPolyVoltage(bus_channel), bus_channel);
			}
		}

		// set pan lights
		if (light_divider.process() && input_on) {   // set lights infrequently
			for (int c = 0; c < channel_no; c++) {
				for (int l = 0; l < 9; l++) {
					float light_delta = 2.f / 8.f;   // 8 divisions because light 1 and 9 are halved by offset
					float light_pos = (l * light_delta) - 1 - (light_delta / 2.f);

					// roll back lights when out of bounds
					if (channel_pan[c] > 1) {
						channel_pan[c] = 1 - (channel_pan[c] - 1);
					} else {
						if (channel_pan[c] < -1) {
							channel_pan[c] = std::abs(channel_pan[c] + 1) + -1;
						}
					}

					// set light brightness
					if (channel_pan[c] >= light_pos && channel_pan[c] < light_pos + light_delta) {
						int flipper = c;
						if (reverse_poly) flipper = channel_no - 1 - c;
						if (inputs[POLY_INPUT].getVoltage(flipper) * 0.1f > light_brights[l]) {
							light_brights[l] = inputs[POLY_INPUT].getVoltage(flipper) * 0.11f;
						} else {
							if (light_brights[l] < 0.25f) light_brights[l] = 0.25f;   // light visible for quiet channel
						}
					}
				}
			}

			// assign light brightness to lights
			for (int l = 0; l < 9; l++) {
				if (light_brights[l] > 0) {
					lights[PAN_LIGHTS + l].value = light_brights[l];
					light_brights[l] -= 1000 / args.sampleRate;
				}
			}
		}
	}
	float smooth_pan(float last, float delta, float pan) {
		if (pan > last) {
				return std::min(last + delta, pan);
			} else {
			return std::max(last - delta, pan);
		}
	}

	float get_left(float angle) {
		return sin((1 - angle) * M_PI_2) * M_SQRT2;
	}

	float get_right(float angle) {
		return sin(angle * M_PI_2) * M_SQRT2;
	}

	// save on and post_fade send button states
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(input_on));
		json_object_set_new(rootJ, "reverse_poly", json_integer(reverse_poly));
		json_object_set_new(rootJ, "blue_post_fade", json_integer(post_fades[0]));
		json_object_set_new(rootJ, "orange_post_fade", json_integer(post_fades[1]));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) input_on = json_integer_value(input_onJ);
		json_t *reverse_polyJ = json_object_get(rootJ, "reverse_poly");
		if (reverse_polyJ) reverse_poly = json_integer_value(reverse_polyJ);
		json_t *blue_post_fadeJ = json_object_get(rootJ, "blue_post_fade");
		if (blue_post_fadeJ) post_fades[0] = json_integer_value(blue_post_fadeJ);
		json_t *orange_post_fadeJ = json_object_get(rootJ, "orange_post_fade");
		if (orange_post_fadeJ) post_fades[1] = json_integer_value(orange_post_fadeJ);
	}
};


struct MetroCityBusWidget : ModuleWidget {
	MetroCityBusWidget(MetroCityBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MetroCityBus.svg")));

		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BlackButton>(mm2px(Vec(20.32, 15.20)), module, MetroCityBus::ON_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(20.32, 15.20)), module, MetroCityBus::ON_LIGHT));
		addParam(createParamCentered<GrayTinyKnob>(mm2px(Vec(12.379, 39.983)), module, MetroCityBus::SPREAD_PARAM));
		addParam(createParamCentered<GrayTinyKnob>(mm2px(Vec(28.072, 39.983)), module, MetroCityBus::PAN_ATT_PARAM));
		addParam(createParamCentered<BlackButton>(mm2px(Vec(7.453, 50.025)), module, MetroCityBus::REVERSE_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(7.453, 50.025)), module, MetroCityBus::REVERSE_LIGHT));
		addParam(createParamCentered<GrayKnob>(mm2px(Vec(20.32, 50.025)), module, MetroCityBus::PAN_PARAM));
		addParam(createParamCentered<BlackButton>(mm2px(Vec(7.453, 66.208)), module, MetroCityBus::BLUE_POST_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(7.453, 66.208)), module, MetroCityBus::BLUE_POST_LIGHT));
		addParam(createParamCentered<BlueKnob>(mm2px(Vec(20.32, 66.208)), module, MetroCityBus::LEVEL_PARAMS + 0));
		addParam(createParamCentered<BlackButton>(mm2px(Vec(7.453, 82.387)), module, MetroCityBus::ORANGE_POST_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(7.453, 82.387)), module, MetroCityBus::ORANGE_POST_LIGHT));
		addParam(createParamCentered<OrangeKnob>(mm2px(Vec(20.32, 82.387)), module, MetroCityBus::LEVEL_PARAMS + 1));
		addParam(createParamCentered<RedKnob>(mm2px(Vec(20.32, 98.565)), module, MetroCityBus::LEVEL_PARAMS + 2));

		addInput(createInputCentered<NutPort>(mm2px(Vec(7.453, 21.083)), module, MetroCityBus::POLY_INPUT));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(33.231, 21.083)), module, MetroCityBus::ON_CV_INPUT));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(33.231, 50.025)), module, MetroCityBus::PAN_CV_INPUT));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(33.231, 66.208)), module, MetroCityBus::LEVEL_CV_INPUTS + 0));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(33.231, 82.387)), module, MetroCityBus::LEVEL_CV_INPUTS + 1));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(33.231, 98.565)), module, MetroCityBus::LEVEL_CV_INPUTS + 2));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.457, 114.107)), module, MetroCityBus::BUS_INPUT));

		addOutput(createOutputCentered<NutPort>(mm2px(Vec(33.231, 114.107)), module, MetroCityBus::BUS_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(4.423, 33.841)), module, MetroCityBus::PAN_LIGHTS + 0));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(8.401, 32.341)), module, MetroCityBus::PAN_LIGHTS + 1));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(12.379, 31.341)), module, MetroCityBus::PAN_LIGHTS + 2));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(16.357, 30.841)), module, MetroCityBus::PAN_LIGHTS + 3));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(20.335, 30.591)), module, MetroCityBus::PAN_LIGHTS + 4));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(24.313, 30.841)), module, MetroCityBus::PAN_LIGHTS + 5));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(28.292, 31.341)), module, MetroCityBus::PAN_LIGHTS + 6));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(32.27, 32.341)), module, MetroCityBus::PAN_LIGHTS + 7));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(36.248, 33.841)), module, MetroCityBus::PAN_LIGHTS + 8));
	}
};


Model *modelMetroCityBus = createModel<MetroCityBus, MetroCityBusWidget>("MetroCityBus");
