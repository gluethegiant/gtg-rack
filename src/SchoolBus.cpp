#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


struct SchoolBus : Module {
	enum ParamIds {
		ON_PARAM,
		PAN_ATT_PARAM,
		PAN_PARAM,
		BLUE_POST_PARAM,
		ORANGE_POST_PARAM,
		ENUMS(LEVEL_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		LMP_INPUT,
		R_INPUT,
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
		ENUMS(ON_LIGHT, 2),   // single red and green light
		BLUE_POST_LIGHT,
		ORANGE_POST_LIGHT,
		NUM_LIGHTS
	};

	LongPressButton on_button;
	dsp::SchmittTrigger on_cv_trigger;
	dsp::SchmittTrigger blue_post_trigger;
	dsp::SchmittTrigger orange_post_trigger;
	dsp::ClockDivider pan_divider;
	dsp::ClockDivider light_divider;
	AutoFader school_fader;
	ConstantPan school_pan;
	SimpleSlewer level_smoother[3];
	SimpleSlewer post_btn_filters[2];

	const int bypass_speed = 26;
	const int pan_speed = 52;   // milliseconds from left to right
	const int level_speed = 26;   // for level cv filter
	float fade_in = 26.f;
	float fade_out = 26.f;
	bool auto_override = false;
	bool auditioned = false;
	bool post_fades[2] = {false, false};
	bool pan_cv_filter = true;
	bool level_cv_filter = true;
	int color_theme = 0;

	SchoolBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(PAN_ATT_PARAM, 0.f, 1.f, 0.5f, "Pan attenuator");
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Level to blue stereo bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Level to orange stereo bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Level to red stereo bus");
		configParam(BLUE_POST_PARAM, 0.f, 1.f, 0.f, "Post red fader send");
		configParam(ORANGE_POST_PARAM, 0.f, 1.f, 0.f, "Post red fader send");
		pan_divider.setDivision(3);
		light_divider.setDivision(512);
		school_fader.setSpeed(fade_in);
		school_pan.setSmoothSpeed(pan_speed);
		for (int i = 0; i < 3; i++) {
			level_smoother[i].setSlewSpeed(level_speed);
		}
		for (int i = 0; i < 2; i++) {
			post_btn_filters[i].setSlewSpeed(level_speed);
			post_btn_filters[i].value = 1.f;
		}
		post_fades[0] = loadGtgPluginDefault("default_post_fader", 0);
		post_fades[1] = post_fades[0];
		color_theme = loadGtgPluginDefault("default_theme", 0);
	}

	void process(const ProcessArgs &args) override {

		// on off button
		switch (on_button.step(params[ON_PARAM])) {
		default:
		case LongPressButton::NO_PRESS:
			break;
		case LongPressButton::SHORT_PRESS:
			if (audition_mixer) {
				audition_mixer = false;   // single click turns off auditions
			} else {
				if ((APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL) {   // bypass fades with ctrl click
					auto_override = true;
					school_fader.setSpeed(bypass_speed);

					// bypass the fade even if the fade is already underway
					if (school_fader.on) {
						school_fader.on = (school_fader.getFade() != school_fader.getGain());
					} else {
						school_fader.on = (school_fader.getFade() == 0.f);
					}

				} else {   // normal single click
					auto_override = false;   // do not override automation
					school_fader.on = !school_fader.on;
					if (school_fader.on) {
						school_fader.setSpeed(int(fade_in));
					} else {
						school_fader.setSpeed(int(fade_out));
					}
				}
			}
			break;
		case LongPressButton::LONG_PRESS:   // long press to audition

			audition_mixer = true;   // all mixers to audition mode

			if (auditioned) {
				auditioned = false;
				if (school_fader.temped) {
					school_fader.temped = false;
					school_fader.on = false;
				}
			} else {

				auditioned = true;

				if (!school_fader.on) {
					school_fader.temped = !school_fader.temped;   // remember if auditioned mixer is off
				}
			}
			break;
		}

		// process cv trigger
		if (on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			if (!audition_mixer) {
				auto_override = false;   // do not override automation
				school_fader.on = !school_fader.on;
			}
		}

		school_fader.process();

		// post fader send buttons
		if (blue_post_trigger.process(params[BLUE_POST_PARAM].getValue())) post_fades[0] = !post_fades[0];
		if (orange_post_trigger.process(params[ORANGE_POST_PARAM].getValue())) post_fades[1] = !post_fades[1];

		// process fade states and light
		if (light_divider.process()) {

			if (audition_mixer) {   // all mixers are in audition state

				// bypass all fade automation
				auto_override = true;
				school_fader.setSpeed(bypass_speed);

				if (auditioned) {   // this mixer is being auditioned
					school_fader.on =  true;
				} else {   // mute the mixers
					if (school_fader.on) {
						school_fader.temped = true;   // remember this mixer was on
					}
					school_fader.on = false;
				}
			} else {   // stop auditions

				// return to states before auditions
				if (school_fader.temped) {
					school_fader.temped = false;
					auto_override = true;
					school_fader.setSpeed(bypass_speed);
					if (auditioned) {
						school_fader.on = false;
					} else {
						school_fader.on = true;
					}
				}

				// turn off auditions
				auditioned = false;
			}

			// process fade speed changes if dragging slider
			if (!auto_override) {
				if (school_fader.on) {
					if (int(fade_in) != school_fader.getFade()) {
						school_fader.setSpeed(int(fade_in));
					}
				} else {
					if (int(fade_out) != school_fader.getFade()) {
						school_fader.setSpeed(int(fade_out));
					}
				}
			}

			// set lights
			lights[BLUE_POST_LIGHT].value = post_fades[0];
			lights[ORANGE_POST_LIGHT].value = post_fades[1];

			if (school_fader.getFade() == school_fader.getGain()) {
				if (audition_mixer) {
					lights[ON_LIGHT + 0].value = 1.f;   // yellow when auditioned
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = 1.f;   // green light when on
					lights[ON_LIGHT + 1].value = 0.f;
				}
			} else {
				if (school_fader.temped) {
					lights[ON_LIGHT + 0].value = 0.f;   // red when temporarily muted
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = school_fader.getFade();  // yellow dimmer when fading
					lights[ON_LIGHT + 1].value = school_fader.getFade() * 0.5f;
				}
			}
		}

		// get input levels
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

		// get stereo pan levels
		if (pan_divider.process()) {   // calculate pan infrequently, useful for auto panning
			if (inputs[PAN_CV_INPUT].isConnected()) {
				float pan_pos = params[PAN_PARAM].getValue() + (((inputs[PAN_CV_INPUT].getNormalVoltage(0) * 2) * params[PAN_ATT_PARAM].getValue()) * 0.1);
				if (pan_cv_filter) {
					school_pan.setSmoothPan(pan_pos);
				} else {
					school_pan.setPan(pan_pos);
				}
			} else {
				school_pan.setPan(params[PAN_PARAM].getValue());
			}
		}

		// get exponential fade
		float exp_fade = 0.f;
		if (school_fader.fading) {
			exp_fade = school_fader.getExpFade(2.5);
		} else {
			exp_fade = school_fader.getFade();
		}

		// process inputs
		float stereo_in[2] = {0.f, 0.f};
		if (inputs[R_INPUT].isConnected()) {   // get a channel from each cable input
			stereo_in[0] = inputs[LMP_INPUT].getVoltage() * school_pan.getLevel(0) * exp_fade;
			stereo_in[1] = inputs[R_INPUT].getVoltage() * school_pan.getLevel(1) * exp_fade;
		} else {   // split mono or sum of polyphonic cable on LMP
			float lmp_in = inputs[LMP_INPUT].getVoltageSum();
			for (int c = 0; c < 2; c++) {
				stereo_in[c] = lmp_in * school_pan.getLevel(c) * exp_fade;
			}
		}

		// process outputs
		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus
			for (int c = 0; c < 2; c++) {
				int bus_channel = (2 * sb) + c;
				outputs[BUS_OUTPUT].setVoltage((stereo_in[c] * in_levels[sb]) + inputs[BUS_INPUT].getPolyVoltage(bus_channel), bus_channel);
			}
		}

		// set bus outputs for 3 stereo buses out
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// load on, post fades, and gain states
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(school_fader.on));
		json_object_set_new(rootJ, "blue_post_fade", json_integer(post_fades[0]));
		json_object_set_new(rootJ, "orange_post_fade", json_integer(post_fades[1]));
		json_object_set_new(rootJ, "gain", json_real(school_fader.getGain()));
		json_object_set_new(rootJ, "pan_cv_filter", json_integer(pan_cv_filter));
		json_object_set_new(rootJ, "level_cv_filter", json_integer(level_cv_filter));
		json_object_set_new(rootJ, "fade_in", json_real(fade_in));
		json_object_set_new(rootJ, "fade_out", json_real(fade_out));
		json_object_set_new(rootJ, "audition_mixer", json_integer(audition_mixer));
		json_object_set_new(rootJ, "auditioned", json_integer(auditioned));
		json_object_set_new(rootJ, "temped", json_integer(school_fader.temped));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load on, post fades, and gain states
	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) school_fader.on = json_integer_value(input_onJ);
		json_t *blue_post_fadeJ = json_object_get(rootJ, "blue_post_fade");
		if (blue_post_fadeJ) post_fades[0] = json_integer_value(blue_post_fadeJ);
		json_t *orange_post_fadeJ = json_object_get(rootJ, "orange_post_fade");
		if (orange_post_fadeJ) post_fades[1] = json_integer_value(orange_post_fadeJ);
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) school_fader.setGain((float)json_real_value(gainJ));
		json_t *pan_cv_filterJ = json_object_get(rootJ, "pan_cv_filter");
		if (pan_cv_filterJ) {
			pan_cv_filter = json_integer_value(pan_cv_filterJ);
		} else {
			if (input_onJ) pan_cv_filter = false;   // do not change existing patches
		}
		json_t *level_cv_filterJ = json_object_get(rootJ, "level_cv_filter");
		if (level_cv_filterJ) {
			level_cv_filter = json_integer_value(level_cv_filterJ);
		} else {
			if (input_onJ) level_cv_filter = false;   // do not change existing patches
		}
		json_t *fade_inJ = json_object_get(rootJ, "fade_in");
		if (fade_inJ) fade_in = json_real_value(fade_inJ);
		json_t *fade_outJ = json_object_get(rootJ, "fade_out");
		if (fade_outJ) fade_out = json_real_value(fade_outJ);
		json_t *audition_mixerJ = json_object_get(rootJ, "audition_mixer");
		if (audition_mixerJ) {
			audition_mixer = json_integer_value(audition_mixerJ);
		} else {
			audition_mixer = false;   // no auditioning when loading old patch right after auditioned patch
		}
		json_t *auditionedJ = json_object_get(rootJ, "auditioned");
		if (auditionedJ) auditioned = json_integer_value(auditionedJ);
		json_t *tempedJ = json_object_get(rootJ, "temped");
		if (tempedJ) school_fader.temped = json_integer_value(tempedJ);
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed on sample rate change
	void onSampleRateChange() override {
		if (school_fader.on) {
			school_fader.setSpeed(fade_in);
		} else {
			school_fader.setSpeed(fade_out);
		}
		school_pan.setSmoothSpeed(pan_speed);
		for (int i = 0; i < 3; i++) {
			level_smoother[i].setSlewSpeed(level_speed);
		}
		for (int i = 0; i < 2; i++) {
			post_btn_filters[i].setSlewSpeed(level_speed);
		}
	}

	// Initialize on state and post fades
	void onReset() override {
		school_fader.on = true;
		school_fader.setGain(1.f);
		fade_in = 26.f;
		fade_out = 26.f;
		post_fades[0] = loadGtgPluginDefault("default_post_fader", 0);
		post_fades[1] = post_fades[0];
		pan_cv_filter = true;
		level_cv_filter = true;
		audition_mixer = false;
	}
};


struct SchoolBusWidget : ModuleWidget {
	SvgPanel* night_panel;

	SchoolBusWidget(SchoolBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SchoolBus.svg")));

		// load night panel if not preview
		if (module) {
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SchoolBus_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(15.24, 15.20)), module, SchoolBus::ON_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(15.24, 15.20)), module, SchoolBus::ON_LIGHT));
		addParam(createThemedParamCentered<gtgGrayTinyKnob>(mm2px(Vec(15.24, 25.9)), module, SchoolBus::PAN_ATT_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgGrayKnob>(mm2px(Vec(15.24, 43.0)), module, SchoolBus::PAN_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlueKnob>(mm2px(Vec(15.24, 61.0)), module, SchoolBus::LEVEL_PARAMS + 0, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgOrangeKnob>(mm2px(Vec(15.24, 79.13)), module, SchoolBus::LEVEL_PARAMS + 1, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgRedKnob>(mm2px(Vec(15.24, 97.29)), module, SchoolBus::LEVEL_PARAMS + 2, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(4.58, 61.0)), module, SchoolBus::BLUE_POST_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.58, 61.0)), module, SchoolBus::BLUE_POST_LIGHT));
		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(4.58, 79.13)), module, SchoolBus::ORANGE_POST_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.58, 79.13)), module, SchoolBus::ORANGE_POST_LIGHT));

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.95, 21.1)), true, module, SchoolBus::LMP_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.95, 31.23)), true, module, SchoolBus::R_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(23.6, 21.1)), true, module, SchoolBus::ON_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(23.6, 31.23)), true, module, SchoolBus::PAN_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(25.07, 52.63)), true, module, SchoolBus::LEVEL_CV_INPUTS + 0, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(25.07, 70.79)), true, module, SchoolBus::LEVEL_CV_INPUTS + 1, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(25.07, 89.0)), true, module, SchoolBus::LEVEL_CV_INPUTS + 2, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.45, 114.1)), true, module, SchoolBus::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.1, 114.1)), false, module, SchoolBus::BUS_OUTPUT, module ? &module->color_theme : NULL));
	}

	// create menu
	void appendContextMenu(Menu* menu) override {
		SchoolBus* module = dynamic_cast<SchoolBus*>(this->module);

		struct GainLevelItem : MenuItem {
			SchoolBus* module;
			float gain;
			void onAction(const event::Action& e) override {
				module->school_fader.setGain(gain);
			}
		};

		struct GainsItem : MenuItem {
			SchoolBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string gain_titles[4] = {"No gain (default)", "2x gain", "4x gain", "8x gain"};
				float gain_amounts[4] = {1.f, 2.f, 4.f, 8.f};
				for (int i = 0; i < 4; i++) {
					GainLevelItem *gain_item = new GainLevelItem;
					gain_item->text = gain_titles[i];
					gain_item->rightText = CHECKMARK(module->school_fader.getGain() == gain_amounts[i]);
					gain_item->module = module;
					gain_item->gain = gain_amounts[i];
					menu->addChild(gain_item);
				}
				return menu;
			}
		};

		struct PanCvItem : MenuItem {
			SchoolBus *module;
			int cv_filter;
			void onAction(const event::Action &e) override {
				module->pan_cv_filter = cv_filter;
			}
		};

		struct PanCvFiltersItem : MenuItem {
			SchoolBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string filter_titles[2] = {"No filter", "Smoothing (default)"};
				int cv_filter_mode[2] = {0, 1};
				for (int i = 0; i < 2; i++) {
					PanCvItem *cv_filter_item = new PanCvItem;
					cv_filter_item->text = filter_titles[i];
					cv_filter_item->rightText = CHECKMARK(module->pan_cv_filter == cv_filter_mode[i]);
					cv_filter_item->module = module;
					cv_filter_item->cv_filter = cv_filter_mode[i];
					menu->addChild(cv_filter_item);
				}
				return menu;
			}
		};

		struct LevelCvItem : MenuItem {
			SchoolBus *module;
			int cv_filter;
			void onAction(const event::Action &e) override {
				module->level_cv_filter = cv_filter;
			}
		};

		struct LevelCvFiltersItem : MenuItem {
			SchoolBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string filter_titles[2] = {"No filter", "Smoothing (default)"};
				int cv_filter_mode[2] = {0, 1};
				for (int i = 0; i < 2; i++) {
					LevelCvItem *cv_filter_item = new LevelCvItem;
					cv_filter_item->text = filter_titles[i];
					cv_filter_item->rightText = CHECKMARK(module->level_cv_filter == cv_filter_mode[i]);
					cv_filter_item->module = module;
					cv_filter_item->cv_filter = cv_filter_mode[i];
					menu->addChild(cv_filter_item);
				}
				return menu;
			}
		};

		struct ThemeItem : MenuItem {
			SchoolBus* module;
			int theme;
			void onAction(const event::Action& e) override {
				module->color_theme = theme;
			}
		};

		struct DefaultThemeItem : MenuItem {
			SchoolBus* module;
			void onAction(const event::Action &e) override {
				saveGtgPluginDefault("default_theme", rightText.empty());
			}
		};

		struct DefaultSendItem : MenuItem {
			SchoolBus* module;
			void onAction(const event::Action &e) override {
				saveGtgPluginDefault("default_post_fader", rightText.empty());
			}
		};

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Fade Automation"));

		FadeSliderItem *fadeInSliderItem = new FadeSliderItem(&(module->fade_in), "In");
		fadeInSliderItem->box.size.x = 190.f;
		menu->addChild(fadeInSliderItem);

		FadeSliderItem *fadeOutSliderItem = new FadeSliderItem(&(module->fade_out), "Out");
		fadeOutSliderItem->box.size.x = 190.f;
		menu->addChild(fadeOutSliderItem);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Mixer Settings"));

		GainsItem *gainsItem = createMenuItem<GainsItem>("Preamps on L/M/P/R Inputs");
		gainsItem->rightText = RIGHT_ARROW;
		gainsItem->module = module;
		menu->addChild(gainsItem);

		PanCvFiltersItem *panCvFiltersItem = createMenuItem<PanCvFiltersItem>("Pan CV Filter");
		panCvFiltersItem->rightText = RIGHT_ARROW;
		panCvFiltersItem->module = module;
		menu->addChild(panCvFiltersItem);

		LevelCvFiltersItem *levelCvFiltersItem = createMenuItem<LevelCvFiltersItem>("Level CV Filters");
		levelCvFiltersItem->rightText = RIGHT_ARROW;
		levelCvFiltersItem->module = module;
		menu->addChild(levelCvFiltersItem);

		// panel themes
		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Panel Themes"));

		std::string themeTitles[2] = {"70's Cream", "Night Ride"};
		for (int i = 0; i < 2; i++) {
			ThemeItem* themeItem = createMenuItem<ThemeItem>(themeTitles[i]);
			themeItem->rightText = CHECKMARK(module->color_theme == i);
			themeItem->module = module;
			themeItem->theme = i;
			menu->addChild(themeItem);
		}

		// plugin defaults
		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("All Modular Bus Mixers"));

		DefaultThemeItem* defaultThemeItem = createMenuItem<DefaultThemeItem>("Default to Night Ride theme");
		defaultThemeItem->rightText = CHECKMARK(loadGtgPluginDefault("default_theme", 0));
		defaultThemeItem->module = module;
		menu->addChild(defaultThemeItem);

		DefaultSendItem* defaultSendItem = createMenuItem<DefaultSendItem>("Default to post fader sends");
		defaultSendItem->rightText = CHECKMARK(loadGtgPluginDefault("default_post_fader", 0));
		defaultSendItem->module = module;
		menu->addChild(defaultSendItem);
	}

	// display panel based on theme
	void step() override {
		if (module) {
			panel->visible = ((((SchoolBus*)module)->color_theme) == 0);
			night_panel->visible = ((((SchoolBus*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelSchoolBus = createModel<SchoolBus, SchoolBusWidget>("SchoolBus");
