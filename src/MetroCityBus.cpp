#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


const long HISTORY_CAP = 512000;

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
	AutoFader metro_fader;
	ConstantPan metro_pan[16];

	const int fade_speed = 20;   // milliseconds from 0 to gain
	const int smooth_speed = 100;   // milliseconds from full left to full right
	float pan_history[HISTORY_CAP] = {};
	long hist_i = 0;
	long hist_size = 0;
	bool reverse_poly = false;
	bool post_fades[2] = {false, false};
	float spread_pos = 0.f;
	int channel_no = 0;
	float light_brights[9] = {};
	long f_delay = 0;   // follow delay
	float pan_rate = APP->engine->getSampleRate() / 3.f;   // sample rate divided by pan clock divider

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
		metro_fader.setSpeed(fade_speed);
		for (int i = 0; i < 16; i++) {
			metro_pan[i].setSmoothSpeed(smooth_speed);
		}
	}

	void process(const ProcessArgs &args) override {
		// on off button controls auto fader to avoid pops
		if (on_trigger.process(params[ON_PARAM].getValue()) + on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			metro_fader.on = !metro_fader.on;
		}

		metro_fader.process();

		// button to reverse polyphonic pan order
		if (reverse_poly_trigger.process(params[REVERSE_PARAM].getValue())) reverse_poly = !reverse_poly;

		// post fader send buttons
		if (blue_post_trigger.process(params[BLUE_POST_PARAM].getValue())) post_fades[0] = !post_fades[0];
		if (orange_post_trigger.process(params[ORANGE_POST_PARAM].getValue())) post_fades[1] = !post_fades[1];

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

		// get number of channels
		channel_no = inputs[POLY_INPUT].getChannels();

		// pans
		if (pan_divider.process() && metro_fader.on) {   // calculate pan every few samples when input is on

			if (inputs[PAN_CV_INPUT].isConnected()) {   // create follow pan when CV connected

				// get pan knob with CV and attenuator
				float pan_pos = params[PAN_PARAM].getValue() + (((inputs[PAN_CV_INPUT].getNormalVoltage(0) * 2) * params[PAN_ATT_PARAM].getValue()) * 0.1f);
				metro_pan[0].setPan(pan_pos);   // no smoothing on first channel's pan

				// spread is only 0 to 1 for pan follow
				spread_pos = std::abs(params[SPREAD_PARAM].getValue());

				// Store pan history of first channel
				if (hist_i >= HISTORY_CAP) hist_i = 0;   // reset history buffer index
				pan_history[hist_i] = metro_pan[0].position;

				// Calculate delay for pan
				f_delay = std::round(spread_pos * pan_rate);   // f_delay * 16 should not be more than HISTORY_CAP

				// calculate pan position for other channels
				for (int c = 1; c < channel_no; c++) {
					long follow = c * f_delay;
					if (follow > hist_size) {   // there is not enough history to follow
						metro_pan[c].position = 0.f;   // stay centered until history is populated
						metro_pan[c].levels[0] = 1.f;
						metro_pan[c].levels[1] = 1.f;
					} else {
						follow = hist_i - follow;
						if (follow < 0) follow = HISTORY_CAP + follow;   // fix follow when buffer resets to 0

						// smooth pan for dynamic channels and history catch up
						metro_pan[c].setSmoothPan(pan_history[follow]);
					}
				}

				hist_i++;   // Keep history buffer rolling
				if (hist_size < HISTORY_CAP) hist_size++;

			} else {   // create spread pan when no CV connected

				hist_size = 0; hist_i = 0;   // reset pan history when CV not connected

				// Get pan and spread positions
				metro_pan[0].setPan(params[PAN_PARAM].getValue());   // first channel is pan knob position
				spread_pos = params[SPREAD_PARAM].getValue();

				// Calculate spread as portion of field between pan knob and hard left or hard right
				float pan_spread = 0.f;
				if (spread_pos < 0) pan_spread = (metro_pan[0].position + 1) * spread_pos;
				if (spread_pos > 0) pan_spread = -1 * ((metro_pan[0].position - 1) * spread_pos);

				// calculate polyphonic spread and pan levels for other channels
				for (int c = 1; c < channel_no; c++) {
					float channel_pos = metro_pan[0].position + (((float)c / (float)(channel_no - 1)) * pan_spread);
					metro_pan[c].setSmoothPan(channel_pos);
				}
			}
		}   // end pan_divider.process()

		// process inputs
		float stereo_in[2] = {0.f, 0.f};
		if (spread_pos == 0) {   // sum channels if no spread
			float sum_in = inputs[POLY_INPUT].getVoltageSum();
			for (int c = 0; c < 2; c++) {
				stereo_in[c] = sum_in * metro_pan[0].levels[c] * metro_fader.getFade();
			}
		} else {
			for (int c = 0; c < channel_no; c++) {
				float channel_in = inputs[POLY_INPUT].getPolyVoltage(c);
				if (reverse_poly) {   // reverses order of pan levels applied to channels
					stereo_in[0] += channel_in * metro_pan[channel_no - c - 1].levels[0];
					stereo_in[1] += channel_in * metro_pan[channel_no - c - 1].levels[1];
				} else {
					stereo_in[0] += channel_in * metro_pan[c].levels[0];
					stereo_in[1] += channel_in * metro_pan[c].levels[1];
				}
			}

			// Apply fade after summing
			stereo_in[0] *= metro_fader.getFade();
			stereo_in[1] *= metro_fader.getFade();
		}

		// process bus outputs
		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus
			for (int c = 0; c < 2; c++) {
				int bus_channel = (2 * sb) + c;
				outputs[BUS_OUTPUT].setVoltage((stereo_in[c] * in_levels[sb]) + inputs[BUS_INPUT].getPolyVoltage(bus_channel), bus_channel);
			}
		}

		// set bus outputs for 3 stereo buses out
		outputs[BUS_OUTPUT].setChannels(6);

		// set lights
		if (light_divider.process()) {   // set lights infrequently

			// button light states
			lights[ON_LIGHT].value = metro_fader.getFade();
			lights[REVERSE_LIGHT].value = reverse_poly;
			lights[BLUE_POST_LIGHT].value = post_fades[0];
			lights[ORANGE_POST_LIGHT].value = post_fades[1];

			if (metro_fader.on) {   // only process pan lights if input is on
				for (int c = 0; c < channel_no; c++) {
					for (int l = 0; l < 9; l++) {
						float light_pan[16] = { };
						float light_delta = 2.f / 8.f;   // 8 divisions because light 1 and 9 are halved by offset
						float light_pos = (l * light_delta) - 1 - (light_delta / 2.f);

						light_pan[c] = metro_pan[c].position;

						// roll back lights when out of bounds
						if (light_pan[c] > 1) {
							light_pan[c] = 1.f - (light_pan[c] - 1.f);
						} else {
							if (light_pan[c] < -1) {
								light_pan[c] = std::abs(light_pan[c] + 1.f) + -1.f;
							}
						}

						// set initial pan light brightness
						if (light_pan[c] >= light_pos && light_pan[c] < light_pos + light_delta) {
							int flipper = c;   // used to flip lights when reverse channel button is on
							if (reverse_poly) flipper = channel_no - 1 - c;   // channel flipping for reverse poly
							if (inputs[POLY_INPUT].getVoltage(flipper) * 0.1f > light_brights[l]) {
								light_brights[l] = inputs[POLY_INPUT].getVoltage(flipper) * 0.12f; 
							} else {
								if (light_brights[l] < 0.18f) light_brights[l] = 0.18f;   // light visible for quiet channel
							}
						}
					}   // for l lights
				}   // for c channels

				// process changes to pan light brightness
				for (int l = 0; l < 9; l++) {
					if (light_brights[l] > 0) {
						lights[PAN_LIGHTS + l].value = light_brights[l];
						light_brights[l] -= 1000 / args.sampleRate;
					}
				}
			} else {   // turn off pan lights if input is off
				for (int l = 0; l < 9; l++) lights[PAN_LIGHTS + l].value = 0;
			}
		}   // light divider
	}

	// save on, post and reverse buttons, and gain states
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(metro_fader.on));
		json_object_set_new(rootJ, "reverse_poly", json_integer(reverse_poly));
		json_object_set_new(rootJ, "blue_post_fade", json_integer(post_fades[0]));
		json_object_set_new(rootJ, "orange_post_fade", json_integer(post_fades[1]));
		json_object_set_new(rootJ, "gain", json_real(metro_fader.getGain()));
		return rootJ;
	}

	// load on, post and reverse buttons, and gain states
	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) metro_fader.on = json_integer_value(input_onJ);
		json_t *reverse_polyJ = json_object_get(rootJ, "reverse_poly");
		if (reverse_polyJ) reverse_poly = json_integer_value(reverse_polyJ);
		json_t *blue_post_fadeJ = json_object_get(rootJ, "blue_post_fade");
		if (blue_post_fadeJ) post_fades[0] = json_integer_value(blue_post_fadeJ);
		json_t *orange_post_fadeJ = json_object_get(rootJ, "orange_post_fade");
		if (orange_post_fadeJ) post_fades[1] = json_integer_value(orange_post_fadeJ);
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) metro_fader.setGain((float)json_real_value(gainJ));
	}

	// recalculate fader, pan smoothing, and pan_rate (used by pan follow)
	void onSampleRateChange() override {
		metro_fader.setSpeed(fade_speed);
		for (int i = 0; i < 16; i++) {
			metro_pan[i].setSmoothSpeed(smooth_speed);
		}
		pan_rate = (APP->engine->getSampleRate() / 3.f);   // used by pan follow, should match pan clock divider
	}

	// Initialize on state and buttons
	void onReset() override {
		metro_fader.on = true;
		metro_fader.setGain(1.f);
		reverse_poly = false;
		post_fades[0] = false;
		post_fades[1] = false;
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

		addParam(createParamCentered<gtgBlackButton>(mm2px(Vec(20.32, 15.20)), module, MetroCityBus::ON_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(20.32, 15.20)), module, MetroCityBus::ON_LIGHT));
		addParam(createParamCentered<gtgGrayTinyKnob>(mm2px(Vec(11.379, 39.74)), module, MetroCityBus::SPREAD_PARAM));
		addParam(createParamCentered<gtgGrayTinyKnob>(mm2px(Vec(29.06, 39.74)), module, MetroCityBus::PAN_ATT_PARAM));
		addParam(createParamCentered<gtgBlackButton>(mm2px(Vec(6.95, 50.01)), module, MetroCityBus::REVERSE_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.95, 50.01)), module, MetroCityBus::REVERSE_LIGHT));
		addParam(createParamCentered<gtgGrayKnob>(mm2px(Vec(20.32, 50.01)), module, MetroCityBus::PAN_PARAM));
		addParam(createParamCentered<gtgBlackButton>(mm2px(Vec(6.95, 66.46)), module, MetroCityBus::BLUE_POST_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.95, 66.46)), module, MetroCityBus::BLUE_POST_LIGHT));
		addParam(createParamCentered<gtgBlueKnob>(mm2px(Vec(20.32, 66.46)), module, MetroCityBus::LEVEL_PARAMS + 0));
		addParam(createParamCentered<gtgBlackButton>(mm2px(Vec(6.95, 82.87)), module, MetroCityBus::ORANGE_POST_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.95, 82.87)), module, MetroCityBus::ORANGE_POST_LIGHT));
		addParam(createParamCentered<gtgOrangeKnob>(mm2px(Vec(20.32, 82.87)), module, MetroCityBus::LEVEL_PARAMS + 1));
		addParam(createParamCentered<gtgRedKnob>(mm2px(Vec(20.32, 99.32)), module, MetroCityBus::LEVEL_PARAMS + 2));

		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.44, 21.083)), module, MetroCityBus::POLY_INPUT));
		addInput(createInputCentered<gtgKeyPort>(mm2px(Vec(33.231, 21.083)), module, MetroCityBus::ON_CV_INPUT));
		addInput(createInputCentered<gtgKeyPort>(mm2px(Vec(33.73, 50.01)), module, MetroCityBus::PAN_CV_INPUT));
		addInput(createInputCentered<gtgKeyPort>(mm2px(Vec(33.73, 66.46)), module, MetroCityBus::LEVEL_CV_INPUTS + 0));
		addInput(createInputCentered<gtgKeyPort>(mm2px(Vec(33.73, 82.87)), module, MetroCityBus::LEVEL_CV_INPUTS + 1));
		addInput(createInputCentered<gtgKeyPort>(mm2px(Vec(33.73, 99.32)), module, MetroCityBus::LEVEL_CV_INPUTS + 2));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.44, 114.107)), module, MetroCityBus::BUS_INPUT));

		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(33.231, 114.107)), module, MetroCityBus::BUS_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(4.423, 33.341)), module, MetroCityBus::PAN_LIGHTS + 0));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(8.401, 31.86)), module, MetroCityBus::PAN_LIGHTS + 1));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(12.379, 30.83)), module, MetroCityBus::PAN_LIGHTS + 2));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(16.357, 30.33)), module, MetroCityBus::PAN_LIGHTS + 3));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(20.335, 30.08)), module, MetroCityBus::PAN_LIGHTS + 4));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(24.313, 30.33)), module, MetroCityBus::PAN_LIGHTS + 5));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(28.292, 30.83)), module, MetroCityBus::PAN_LIGHTS + 6));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(32.27, 31.86)), module, MetroCityBus::PAN_LIGHTS + 7));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(36.248, 33.341)), module, MetroCityBus::PAN_LIGHTS + 8));
	}

	// add gain options to context menu
	void appendContextMenu(Menu* menu) override {
		MetroCityBus* module = dynamic_cast<MetroCityBus*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Input Gain"));

		struct GainItem : MenuItem {
			MetroCityBus* module;
			float gain;
			void onAction(const event::Action& e) override {
				module->metro_fader.setGain(gain);
			}
		};

		std::string gainTitles[3] = {"100% (default)", "150%", "200%"};
		float gainAmounts[3] = {1.f, 1.5f, 2.f};
		for (int i = 0; i < 3; i++) {
			GainItem* gainItem = createMenuItem<GainItem>(gainTitles[i]);
			gainItem->rightText = CHECKMARK(module->metro_fader.getGain() == gainAmounts[i]);
			gainItem->module = module;
			gainItem->gain = gainAmounts[i];
			menu->addChild(gainItem);
		}
	}
};


Model *modelMetroCityBus = createModel<MetroCityBus, MetroCityBusWidget>("MetroCityBus");
