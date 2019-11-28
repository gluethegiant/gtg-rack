#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"

struct BusDepot : Module {
	enum ParamIds {
		ON_PARAM,
		AUX_PARAM,
		LEVEL_PARAM,
		FADE_PARAM,
		FADE_IN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ON_CV_INPUT,
		LEVEL_CV_INPUT,
		LMP_INPUT,
		R_INPUT,
		BUS_INPUT,
		FADE_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(ON_LIGHT, 2),
		ENUMS(LEFT_LIGHTS, 11),
		ENUMS(RIGHT_LIGHTS, 11),
		NUM_LIGHTS
	};

	LongPressButton on_button;
	dsp::VuMeter2 vu_meters[2];
	dsp::ClockDivider vu_divider;
	dsp::ClockDivider light_divider;
	dsp::ClockDivider audition_divider;
	dsp::SchmittTrigger on_cv_trigger;
	AutoFader depot_fader;
	SimpleSlewer level_smoother;

	const int bypass_speed = 26;
	const int level_speed = 26;   // for level cv filter
	float peak_left = 0.f;
	float peak_right = 0.f;
	bool level_cv_filter = true;
	int fade_cv_mode = 0;
	bool auto_override = false;
	bool auditioned = false;
	int audition_mode = 0;
	int color_theme = 0;

	BusDepot() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Output on");   // depot_fader defaults to on and creates a quick fade up
		configParam(AUX_PARAM, 0.f, 1.f, 1.f, "Aux level in");
		configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Master level");
		configParam(FADE_PARAM, 26, 34000, 26, "Fade out automation in milliseconds");
		configParam(FADE_IN_PARAM, 26, 34000, 26, "Fade in automation in milliseconds");
		vu_meters[0].lambda = 25.f;
		vu_meters[1].lambda = 25.f;
		vu_divider.setDivision(500);
		light_divider.setDivision(240);
		audition_divider.setDivision(512);
		depot_fader.setSpeed(26);
		level_smoother.setSlewSpeed(level_speed);   // for level cv filter
		color_theme = loadGtgPluginDefault("default_theme", 0);
	}

	void process(const ProcessArgs &args) override {

		// on off button
		switch (on_button.step(params[ON_PARAM])) {
		default:
		case LongPressButton::NO_PRESS:
			break;
		case LongPressButton::SHORT_PRESS:
			if (audition_depot) {
				audition_depot = false;   // single click turns off auditions
			} else {
				if ((APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL) {   // bypass fades with ctrl click
					auto_override = true;
					depot_fader.setSpeed(bypass_speed);

					// bypass the fade even if the fade is already underway
					if (depot_fader.on) {
						depot_fader.on = (depot_fader.getFade() != depot_fader.getGain());
					} else {
						depot_fader.on = (depot_fader.getFade() == 0.f);
					}

				} else {   // normal single click
					auto_override = false;   // do not override automation
					depot_fader.on = !depot_fader.on;
					if (depot_fader.on) {
						depot_fader.setSpeed(int(params[FADE_IN_PARAM].getValue()));
					} else {
						depot_fader.setSpeed(int(params[FADE_PARAM].getValue()));
					}
				}
			}
			break;
		case LongPressButton::LONG_PRESS:   // long press to audition

			audition_depot = true;   // all depots to audition mode
			auditioned = true;

			if (!depot_fader.on) {
				depot_fader.temped = !depot_fader.temped;   // remember if auditioned depot is off
			}
			break;
		}

		// process cv trigger
		if (on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			auto_override = false;   // do not override automation
			depot_fader.on = !depot_fader.on;
		}

		depot_fader.process();

		// process fade states and on light
		if (audition_divider.process()) {

			if (audition_depot) {   // all depots are in audition state

				// bypass all fade automation
				auto_override = true;
				depot_fader.setSpeed(bypass_speed);

				// set to auditioned if audition mode = 1
				if (audition_mode == 1) {
					if (!depot_fader.on) {
						depot_fader.temped = !depot_fader.temped;
					}
					auditioned = true;
				}

			if (auditioned) {   // this depot is being auditioned
					depot_fader.on =  true;
				} else {   // mute the depot
					if (depot_fader.on) {
						depot_fader.temped = true;   // remember this depot was on
					}
					depot_fader.on = false;
				}
			} else {   // stop auditions

				// return to states before auditions
				if (depot_fader.temped) {
					depot_fader.temped = false;
					auto_override = true;
					depot_fader.setSpeed(bypass_speed);
					if (auditioned) {
						depot_fader.on = false;
					} else {
						depot_fader.on = true;
					}
				}

				// turn off auditions
				auditioned = false;
			}

			// process fade speed changes if turning knobs
			if (!auto_override) {
				if (inputs[FADE_CV_INPUT].isConnected()) {
					if (depot_fader.on) {   // fade in with CV
						if (fade_cv_mode == 0 || fade_cv_mode == 1) {
							depot_fader.setSpeed(std::round((clamp(inputs[FADE_CV_INPUT].getNormalVoltage(0.0f) * 0.1f, 0.0f, 1.0f) * 33974.f) + 26.f));   // 26 to 34000 milliseconds
						} else {
							if (params[FADE_IN_PARAM].getValue() != depot_fader.last_speed) {
								depot_fader.setSpeed(params[FADE_IN_PARAM].getValue());
							}
						}
					} else {   // fade out with CV
						if (fade_cv_mode == 0 || fade_cv_mode == 2) {
							depot_fader.setSpeed(std::round((clamp(inputs[FADE_CV_INPUT].getNormalVoltage(0.0f) * 0.1f, 0.0f, 1.0f) * 33974.f) + 26.f));   // 26 to 34000 milliseconds
						} else {
							if (params[FADE_PARAM].getValue() != depot_fader.last_speed) {
								depot_fader.setSpeed(params[FADE_PARAM].getValue());
							}
						}
					}
				} else {
					if (depot_fader.on) {
						if (params[FADE_IN_PARAM].getValue() != depot_fader.getFade()) {
							depot_fader.setSpeed(params[FADE_IN_PARAM].getValue());
						}
					} else {
						if (params[FADE_PARAM].getValue() != depot_fader.getFade()) {
							depot_fader.setSpeed(params[FADE_PARAM].getValue());
						}
					}
				}
			}

			// set lights
			if (depot_fader.getFade() == depot_fader.getGain()) {
				if (audition_depot) {
					lights[ON_LIGHT + 0].value = 1.f;   // yellow when auditioned
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = 1.f;   // green light when on
					lights[ON_LIGHT + 1].value = 0.f;
				}
			} else {
				if (depot_fader.temped) {
					lights[ON_LIGHT + 0].value = 0.f;   // red when temporarily muted
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = depot_fader.getFade();  // yellow dimmer when fading
					lights[ON_LIGHT + 1].value = depot_fader.getFade() * 0.5f;
				}
			}
		}

		// process sound
		float summed_out[2] = {0.f, 0.f};
		if (depot_fader.getFade() > 0) {   // don't need to process sound when silent

			// get param levels
			float aux_level = params[AUX_PARAM].getValue();
			float master_level = clamp(inputs[LEVEL_CV_INPUT].getNormalVoltage(10.0f) * 0.1f, 0.0f, 1.0f) * params[LEVEL_PARAM].getValue();
			if (level_cv_filter) master_level = level_smoother.slew(master_level);
			float fade = depot_fader.getExpFade(2.5);   // exponential fade for fade automation

			// get aux inputs
			float stereo_in[2] = {0.f, 0.f};
			if (inputs[R_INPUT].isConnected()) {   // get a channel from each cable
				stereo_in[0] = inputs[LMP_INPUT].getVoltage() * aux_level;
				stereo_in[1] = inputs[R_INPUT].getVoltage() * aux_level;
			} else {   // get mono polyphonic cable sum from LMP
				float lmp_in = inputs[LMP_INPUT].getVoltageSum() * aux_level;
				for (int c = 0; c < 2; c++) {
					stereo_in[c] = lmp_in;
				}
			}

			// bus inputs with levels
			float bus_in[6] = {};

			// get blue and orange buses with levels
			for (int c = 0; c < 4; c++) {
				bus_in[c] = inputs[BUS_INPUT].getPolyVoltage(c) * master_level * fade;
			}

			// get red levels and add aux inputs
			for (int c = 4; c < 6; c++) {
				bus_in[c] = (stereo_in[c - 4] + inputs[BUS_INPUT].getPolyVoltage(c)) * master_level * fade;
			}

			// set bus outputs
			for (int c = 0; c < 6; c++) {
				outputs[BUS_OUTPUT].setVoltage(bus_in[c], c);
			}

			// sum stereo mix for stereo outputs and light levels
			for (int c = 0; c < 2; c++) {
				summed_out[c] = bus_in[c] + bus_in[c + 2] + bus_in[c + 4];
			}

			// set stereo mix out
			outputs[LEFT_OUTPUT].setVoltage(summed_out[0]);
			outputs[RIGHT_OUTPUT].setVoltage(summed_out[1]);
		}

		// set three stereo bus outputs on bus out
		outputs[BUS_OUTPUT].setChannels(6);

		// hit peak lights accurately by polling every sample
		if (summed_out[0] > 10.f) peak_left = 1.f;
		if (summed_out[1] > 10.f) peak_right = 1.f;

		// get levels for lights
		if (vu_divider.process()) {   // check levels infrequently
			for (int i = 0; i < 2; i++) {
				vu_meters[i].process(args.sampleTime * vu_divider.getDivision(), summed_out[i] / 10.f);
			}
		}

		if (light_divider.process()) {   // set lights and fade speed infrequently

			// make peak lights stay on when hit
			if (peak_left > 0) peak_left -= 60.f / args.sampleRate; else peak_left = 0.f;
			if (peak_right > 0) peak_right -= 60.f / args.sampleRate; else peak_right = 0.f;
			lights[LEFT_LIGHTS + 0].setBrightness(peak_left);
			lights[RIGHT_LIGHTS + 0].setBrightness(peak_right);

			// green and yellow lights
			for (int i = 1; i < 11; i++) {
				lights[LEFT_LIGHTS + i].setBrightness(vu_meters[0].getBrightness((-3 * i), -3 * (i - 1)));
				lights[RIGHT_LIGHTS + i].setBrightness(vu_meters[1].getBrightness((-3 * i), -3 * (i - 1)));
			}
		}
	}

	// save on button state
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(depot_fader.on));
		json_object_set_new(rootJ, "level_cv_filter", json_integer(level_cv_filter));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		json_object_set_new(rootJ, "fade_cv_mode", json_integer(fade_cv_mode));
		json_object_set_new(rootJ, "auditioned", json_integer(auditioned));
		json_object_set_new(rootJ, "temped", json_integer(depot_fader.temped));
		json_object_set_new(rootJ, "audition_mode", json_integer(audition_mode));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) depot_fader.on = json_integer_value(input_onJ);
		json_t *level_cv_filterJ = json_object_get(rootJ, "level_cv_filter");
		if (level_cv_filterJ) {
			level_cv_filter = json_integer_value(level_cv_filterJ);
		} else {
			if (input_onJ) level_cv_filter = false;   // do not change existing patches
		}
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
		json_t *fade_cv_modeJ = json_object_get(rootJ, "fade_cv_mode");
		if (fade_cv_modeJ) {
			fade_cv_mode = json_integer_value(fade_cv_modeJ);
		} else {
			if (input_onJ) {
				params[FADE_IN_PARAM].setValue(params[FADE_PARAM].getValue());   // same behavior on patches saved before fade in knob existed
			}
		}
		json_t *auditionedJ = json_object_get(rootJ, "auditioned");
		if (auditionedJ) {
			auditioned = json_integer_value(auditionedJ);
			if (!audition_depot) audition_depot = auditioned;   // if one bus depot is auditioned then audition_depot
		}
		json_t *tempedJ = json_object_get(rootJ, "temped");
		if (tempedJ) depot_fader.temped = json_integer_value(tempedJ);
		json_t *audition_modeJ = json_object_get(rootJ, "audition_mode");
		if (audition_modeJ) audition_mode = json_integer_value(audition_modeJ);
	}

	void onSampleRateChange() override {
		if (depot_fader.on) {
			depot_fader.setSpeed(params[FADE_IN_PARAM].getValue());
		} else {
			depot_fader.setSpeed(params[FADE_PARAM].getValue());
		}
		level_smoother.setSlewSpeed(level_speed);
	}

	void onReset() override {
		depot_fader.on = true;
		depot_fader.setGain(1.f);
		level_cv_filter = true;
		fade_cv_mode = 0;
		audition_mode = 0;
		audition_depot = false;
	}
};


struct BusDepotWidget : ModuleWidget {
	SvgPanel* night_panel;

	BusDepotWidget(BusDepot *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusDepot.svg")));

		// load night panel if not preview
		if (module) {
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusDepot_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(15.24, 15.20)), module, BusDepot::ON_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(15.24, 15.20)), module, BusDepot::ON_LIGHT));
		addParam(createThemedParamCentered<gtgBlackTinyKnob>(mm2px(Vec(15.24, 59.48)), module, BusDepot::AUX_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlackKnob>(mm2px(Vec(15.24, 83.88)), module, BusDepot::LEVEL_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgGrayTinySnapKnob>(mm2px(Vec(15.24, 42.54)), module, BusDepot::FADE_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgGrayTinySnapKnob>(mm2px(Vec(15.24, 26.15)), module, BusDepot::FADE_IN_PARAM, module ? &module->color_theme : NULL));

		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(23.6, 21.1)), true, module, BusDepot::ON_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(15.24, 71.13)), true, module, BusDepot::LEVEL_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.95, 21.1)), true, module, BusDepot::LMP_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.95, 31.2)), true, module, BusDepot::R_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.45, 114.1)), true, module, BusDepot::BUS_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(23.6, 31.2)), true, module, BusDepot::FADE_CV_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.1, 103.85)), false, module, BusDepot::LEFT_OUTPUT, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.1, 114.1)), false, module, BusDepot::RIGHT_OUTPUT, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.45, 103.85)), false, module, BusDepot::BUS_OUTPUT, module ? &module->color_theme : NULL));

		// create vu lights
		for (int i = 0; i < 11; i++) {
			float spacing = i * 4.25;
			float top = 49.5;
			if (i < 1 ) {
				addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.95, top + spacing)), module, BusDepot::LEFT_LIGHTS + i));
				addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.6, top + spacing)), module, BusDepot::RIGHT_LIGHTS + i));
			} else {
				if (i < 2) {
					addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(4.95, top + spacing)), module, BusDepot::LEFT_LIGHTS + i));
					addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(25.6, top + spacing)), module, BusDepot::RIGHT_LIGHTS + i));
				} else {
					addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(4.95, top + spacing)), module, BusDepot::LEFT_LIGHTS + i));
					addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(25.6, top + spacing)), module, BusDepot::RIGHT_LIGHTS + i));
				}
			}
		}
	}

	// build the menu
	void appendContextMenu(Menu* menu) override {
		BusDepot* module = dynamic_cast<BusDepot*>(this->module);

		struct LevelCvItem : MenuItem {
			BusDepot *module;
			int cv_filter;
			void onAction(const event::Action &e) override {
				module->level_cv_filter = cv_filter;
			}
		};

		struct LevelCvFiltersItem : MenuItem {
			BusDepot *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string filter_titles[2] = {"No filter", "Smoothing (default)"};
				int cv_filter_mode[2] = {0, 1};
				for (int i = 0; i < 2; i++) {
					LevelCvItem *fade_cv_item = new LevelCvItem;
					fade_cv_item->text = filter_titles[i];
					fade_cv_item->rightText = CHECKMARK(module->level_cv_filter == cv_filter_mode[i]);
					fade_cv_item->module = module;
					fade_cv_item->cv_filter = cv_filter_mode[i];
					menu->addChild(fade_cv_item);
				}
				return menu;
			}
		};

		struct FadeCvItem : MenuItem {
			BusDepot *module;
			int cv_mode;
			void onAction(const event::Action &e) override {
				module->fade_cv_mode = cv_mode;
			}
		};

		struct FadeCvModesItem : MenuItem {
			BusDepot *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string mode_titles[3] = {"Fade in and fade out (default)", "Fade in only", "Fade out only"};
				int cv_modes[3] = {0, 1, 2};
				for (int i = 0; i < 3; i++) {
					FadeCvItem *fade_cv_item = new FadeCvItem;
					fade_cv_item->text = mode_titles[i];
					fade_cv_item->rightText = CHECKMARK(module->fade_cv_mode == cv_modes[i]);
					fade_cv_item->module = module;
					fade_cv_item->cv_mode = cv_modes[i];
					menu->addChild(fade_cv_item);
				}
				return menu;
			}
		};

		struct AuditionItem : MenuItem {
			BusDepot *module;
			int au_mode;
			void onAction(const event::Action &e) override {
				module->audition_mode = au_mode;
			}
		};

		struct AuditionModesItem : MenuItem {
			BusDepot *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string mode_titles[2] = {"Normal (default)", "Always auditioned"};
				int au_modes[2] = {0, 1};
				for (int i = 0; i < 2; i++) {
					AuditionItem *audition_item = new AuditionItem;
					audition_item->text = mode_titles[i];
					audition_item->rightText = CHECKMARK(module->audition_mode == au_modes[i]);
					audition_item->module = module;
					audition_item->au_mode = au_modes[i];
					menu->addChild(audition_item);
				}
				return menu;
			}
		};

		struct ThemeItem : MenuItem {
			BusDepot* module;
			int theme;
			void onAction(const event::Action& e) override {
				module->color_theme = theme;
			}
		};

		struct DefaultThemeItem : MenuItem {
			BusDepot* module;
			void onAction(const event::Action &e) override {
				saveGtgPluginDefault("default_theme", rightText.empty());
			}
		};

		struct DefaultSendItem : MenuItem {
			BusDepot* module;
			void onAction(const event::Action &e) override {
				saveGtgPluginDefault("default_post_fader", rightText.empty());
			}
		};

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Mixer Settings"));

		LevelCvFiltersItem *levelCvFiltersItem = createMenuItem<LevelCvFiltersItem>("Level CV Filters");
		levelCvFiltersItem->rightText = RIGHT_ARROW;
		levelCvFiltersItem->module = module;
		menu->addChild(levelCvFiltersItem);

		FadeCvModesItem *fadeCvModesItem = createMenuItem<FadeCvModesItem>("Fade Speed Modulation");
		fadeCvModesItem->rightText = RIGHT_ARROW;
		fadeCvModesItem->module = module;
		menu->addChild(fadeCvModesItem);

		AuditionModesItem *auditionModesItem = createMenuItem<AuditionModesItem>("Audition Modes");
		auditionModesItem->rightText = RIGHT_ARROW;
		auditionModesItem->module = module;
		menu->addChild(auditionModesItem);

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

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("All Modular Bus Mixers"));

		DefaultThemeItem* defaultThemeItem = createMenuItem<DefaultThemeItem>("Default Night Ride theme");
		defaultThemeItem->rightText = CHECKMARK(loadGtgPluginDefault("default_theme", 0));
		defaultThemeItem->module = module;
		menu->addChild(defaultThemeItem);

		DefaultSendItem* defaultSendItem = createMenuItem<DefaultSendItem>("Default to post fader sends");
		defaultSendItem->rightText = CHECKMARK(loadGtgPluginDefault("default_post_fader", 0));
		defaultSendItem->module = module;
		menu->addChild(defaultSendItem);
	}

	// display the panel based on the theme
	void step() override {
		if (module) {
			panel->visible = ((((BusDepot*)module)->color_theme) == 0);
			night_panel->visible = ((((BusDepot*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelBusDepot = createModel<BusDepot, BusDepotWidget>("BusDepot");
