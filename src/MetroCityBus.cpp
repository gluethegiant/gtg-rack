#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


const long HISTORY_CAP = 512000;
const float pan_division = 3.f;

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
	SimpleSlewer level_smoother[3];
	SimpleSlewer post_btn_filters[2];

	const int fade_speed = 26;   // milliseconds from 0 to gain
	const int smooth_speed = 86;   // milliseconds from full left to full right
	const int level_speed = 26;
	float pan_history[HISTORY_CAP] = {};
	long hist_i = 0;
	long hist_size = 0;
	bool reverse_poly = false;
	bool post_fades[2] = {false, false};
	float spread_pos = 0.f;
	int channel_no = 0;
	float light_pan[16] = {};
	float light_delta = 2.f / 8.f;   // 8 divisions because light 1 and 9 are halved by offset
	float light_brights[9] = {};
	long f_delay = 0;   // follow delay
	float pan_rate = APP->engine->getSampleRate() / pan_division;   // to work with pan clock divider
	bool level_cv_filter = true;
	int color_theme = 0;

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
		pan_divider.setDivision(pan_division);
		light_divider.setDivision(64);
		metro_fader.setSpeed(fade_speed);
		initializePanObjects();
		for (int i = 0; i < 3; i++) {
			level_smoother[i].setSlewSpeed(level_speed);
		}
		for (int i = 0; i < 2; i++) {
			post_btn_filters[i].setSlewSpeed(level_speed);
			post_btn_filters[i].value = 1.f;
		}
		color_theme = loadDefaultTheme();
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

		// smooth input levels
		if (level_cv_filter) {
			for (int sb = 0; sb < 3; sb++) {
				in_levels[sb] = level_smoother[sb].slew(in_levels[sb]);
			}
		}

		// set post fades on levels
		for (int i = 0; i < 2; i++) {
			if (post_fades[i]) {
				in_levels[i] *= post_btn_filters[i].slew(in_levels[2]);
			} else {
				in_levels[i] *= post_btn_filters[i].slew(1.f);
			}
		}

		// get number of channels
		channel_no = inputs[POLY_INPUT].getChannels();

		// pans
		if (pan_divider.process() && metro_fader.on) {   // calculate pan every few samples when input is on

			// create follow pan when CV connected
			if (inputs[PAN_CV_INPUT].isConnected()) {

				// get pan knob with CV and attenuator
				float pan_pos = params[PAN_PARAM].getValue() + (((inputs[PAN_CV_INPUT].getNormalVoltage(0) * 2) * params[PAN_ATT_PARAM].getValue()) * 0.1f);
				metro_pan[0].setSmoothPan(pan_pos);
				light_pan[0] = metro_pan[0].position;   // pan position for lights

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
					if (follow <= hist_size) {   // stay put until there is enough history to follow
						follow = hist_i - follow;
						if (follow < 0) follow = HISTORY_CAP + follow;   // fix follow when buffer resets to 0

						// smooth pan for dynamic channels and history catch up
						if (inputs[POLY_INPUT].getPolyVoltage(c) > 0.f) {
							metro_pan[c].setSmoothPan(pan_history[follow]);   // full pan calculation if there is sound
							light_pan[c] = metro_pan[c].position;
						} else {
							light_pan[c] = pan_history[follow];   // set only lights on silent channels
						}
					}
				}

				hist_i++;   // Keep history buffer rolling
				if (hist_size < HISTORY_CAP) hist_size++;

			} else {   // create spread pan when no CV connected

				hist_size = 0; hist_i = 0;   // reset pan history when CV not connected

				// Get pan and spread positions
				metro_pan[0].setPan(params[PAN_PARAM].getValue());   // first channel is pan knob position
				light_pan[0] = metro_pan[0].position;   // pan position for lights

				spread_pos = params[SPREAD_PARAM].getValue();

				// Calculate spread as portion of field between pan knob and hard left or hard right
				float pan_spread = 0.f;
				if (spread_pos < 0) pan_spread = (metro_pan[0].position + 1) * spread_pos;
				if (spread_pos > 0) pan_spread = -1 * ((metro_pan[0].position - 1) * spread_pos);

				// calculate polyphonic spread and pan levels for other channels
				for (int c = 1; c < channel_no; c++) {
					float channel_pos = metro_pan[0].position + (((float)c / (float)(channel_no - 1)) * pan_spread);
					metro_pan[c].setSmoothPan(channel_pos);
					light_pan[c] = metro_pan[c].position;
				}
			}
		}   // end pan_divider.process()

		// process inputs
		float stereo_in[2] = {0.f, 0.f};
		if (spread_pos == 0 && metro_pan[channel_no - 1].position == params[PAN_PARAM].getValue()) {   // sum channels if no spread
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
						float light_pos = (l * light_delta) - 1 - (light_delta / 2.f);

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
								if (light_brights[l] < 0.12f) light_brights[l] = 0.12f;   // light visible for quiet channel
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
		json_object_set_new(rootJ, "level_cv_filter", json_integer(level_cv_filter));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
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
		json_t *level_cv_filterJ = json_object_get(rootJ, "level_cv_filter");
		if (level_cv_filterJ) {
			level_cv_filter = json_integer_value(level_cv_filterJ);
		} else {
			if (input_onJ) level_cv_filter = false;   // do not change existing patches
		}
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// recalculate fader, pan smoothing, and pan_rate (used by pan follow)
	void onSampleRateChange() override {
		metro_fader.setSpeed(fade_speed);
		for (int i = 0; i < 16; i++) {
			metro_pan[i].setSmoothSpeed(smooth_speed);
		}
		pan_rate = (APP->engine->getSampleRate() / pan_division);   // used by pan follow, accounts for pan clock divider
		for (int i = 0; i < 3; i++) {
			level_smoother[i].setSlewSpeed(level_speed);
		}
		for (int i = 0; i < 2; i++) {
			post_btn_filters[i].setSlewSpeed(level_speed);
		}
	}

	// Initialize on state and buttons
	void onReset() override {
		metro_fader.on = true;
		metro_fader.setGain(1.f);
		reverse_poly = false;
		post_fades[0] = false;
		post_fades[1] = false;
		initializePanObjects();
		level_cv_filter = true;
	}

	// initialize pan objects
	void initializePanObjects () {
		for (int i = 0; i < 16; i++) {
			metro_pan[i].position = 0.f;
			metro_pan[i].levels[0] = 1.f;
			metro_pan[i].levels[1] = 1.f;
			metro_pan[i].setSmoothSpeed(smooth_speed);
		}
	}

};


struct MetroCityBusWidget : ModuleWidget {
	SvgPanel* night_panel;

	MetroCityBusWidget(MetroCityBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MetroCityBus.svg")));

		// load night panel if not preview
		if (module) {
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MetroCityBus_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(20.32, 15.20)), module, MetroCityBus::ON_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(20.32, 15.20)), module, MetroCityBus::ON_LIGHT));
		addParam(createThemedParamCentered<gtgGrayTinyKnob>(mm2px(Vec(11.379, 39.74)), module, MetroCityBus::SPREAD_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgGrayTinyKnob>(mm2px(Vec(29.06, 39.74)), module, MetroCityBus::PAN_ATT_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(6.95, 50.01)), module, MetroCityBus::REVERSE_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(6.95, 50.01)), module, MetroCityBus::REVERSE_LIGHT));
		addParam(createThemedParamCentered<gtgGrayKnob>(mm2px(Vec(20.32, 50.01)), module, MetroCityBus::PAN_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(6.95, 66.46)), module, MetroCityBus::BLUE_POST_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.95, 66.46)), module, MetroCityBus::BLUE_POST_LIGHT));
		addParam(createThemedParamCentered<gtgBlueKnob>(mm2px(Vec(20.32, 66.46)), module, MetroCityBus::LEVEL_PARAMS + 0, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(6.95, 82.87)), module, MetroCityBus::ORANGE_POST_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.95, 82.87)), module, MetroCityBus::ORANGE_POST_LIGHT));
		addParam(createThemedParamCentered<gtgOrangeKnob>(mm2px(Vec(20.32, 82.87)), module, MetroCityBus::LEVEL_PARAMS + 1, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgRedKnob>(mm2px(Vec(20.32, 99.32)), module, MetroCityBus::LEVEL_PARAMS + 2, module ? &module->color_theme : NULL));

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.44, 21.083)), true, module, MetroCityBus::POLY_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(33.231, 21.083)), true, module, MetroCityBus::ON_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(33.73, 50.01)), true, module, MetroCityBus::PAN_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(33.73, 66.46)), true, module, MetroCityBus::LEVEL_CV_INPUTS + 0, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(33.73, 82.87)), true, module, MetroCityBus::LEVEL_CV_INPUTS + 1, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(33.73, 99.32)), true, module, MetroCityBus::LEVEL_CV_INPUTS + 2, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.44, 114.107)), true, module, MetroCityBus::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(33.231, 114.107)), false, module, MetroCityBus::BUS_OUTPUT, module ? &module->color_theme : NULL));

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

	// create menu
	void appendContextMenu(Menu* menu) override {
		MetroCityBus* module = dynamic_cast<MetroCityBus*>(this->module);

		struct ThemeItem : MenuItem {
			MetroCityBus* module;
			int theme;
			void onAction(const event::Action& e) override {
				module->color_theme = theme;
			}
		};

		struct GainItem : MenuItem {
			MetroCityBus* module;
			float gain;
			void onAction(const event::Action& e) override {
				module->metro_fader.setGain(gain);
			}
		};

		struct LevelCvFilterItem : MenuItem {
			MetroCityBus* module;
			void onAction(const event::Action& e) override {
				module->level_cv_filter = !module->level_cv_filter;
			}
		};

		struct DefaultThemeItem : MenuItem {
			MetroCityBus* module;
			void onAction(const event::Action &e) override {
				saveDefaultTheme(rightText.empty());
			}
		};

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Color Theme"));

		std::string themeTitles[2] = {"70's Cream", "Night Ride"};
		for (int i = 0; i < 2; i++) {
			ThemeItem* themeItem = createMenuItem<ThemeItem>(themeTitles[i]);
			themeItem->rightText = CHECKMARK(module->color_theme == i);
			themeItem->module = module;
			themeItem->theme = i;
			menu->addChild(themeItem);
		}

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Preamp on Polyphonic Input"));

		std::string gainTitles[3] = {"No gain (default)", "2x gain", "4x gain"};
		float gainAmounts[3] = {1.f, 2.f, 4.f};
		for (int i = 0; i < 3; i++) {
			GainItem* gainItem = createMenuItem<GainItem>(gainTitles[i]);
			gainItem->rightText = CHECKMARK(module->metro_fader.getGain() == gainAmounts[i]);
			gainItem->module = module;
			gainItem->gain = gainAmounts[i];
			menu->addChild(gainItem);
		}

		// CV filters
		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("CV Input Filters"));

		LevelCvFilterItem* levelCvFilterItem = createMenuItem<LevelCvFilterItem>("Smoothing on level CVs");
		levelCvFilterItem->rightText = CHECKMARK(module->level_cv_filter);
		levelCvFilterItem->module = module;
		menu->addChild(levelCvFilterItem);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("All Modular Bus Mixers"));

		DefaultThemeItem* defaultThemeItem = createMenuItem<DefaultThemeItem>("Default Night Ride theme");
		defaultThemeItem->rightText = CHECKMARK(loadDefaultTheme());
		defaultThemeItem->module = module;
		menu->addChild(defaultThemeItem);
}

	// display panel based on theme
	void step() override {
		if (module) {
			panel->visible = ((((MetroCityBus*)module)->color_theme) == 0);
			night_panel->visible = ((((MetroCityBus*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelMetroCityBus = createModel<MetroCityBus, MetroCityBusWidget>("MetroCityBus");
