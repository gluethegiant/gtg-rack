#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


struct MiniBus : Module {
	enum ParamIds {
		ON_PARAM,
		ENUMS(LEVEL_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		ON_CV_INPUT,
		MP_INPUT,
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(ON_LIGHT, 2),   // single red and green light
		NUM_LIGHTS
	};

	LongPressButton on_button;
	dsp::SchmittTrigger on_cv_trigger;
	dsp::ClockDivider light_divider;
	AutoFader mini_fader;
	SimpleSlewer post_fade_filter;

	const int bypass_speed = 26;
	const int smooth_speed = 26;
	float fade_in = 26.f;
	float fade_out = 26.f;
	bool auto_override = false;
	bool post_fades = false;
	bool auditioned = false;
	int color_theme = 0;
	bool use_default_theme = true;

	MiniBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Level to blue bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Level to orange bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Level to red bus");
		light_divider.setDivision(512);
		mini_fader.setSpeed(fade_in);
		post_fade_filter.setSlewSpeed(smooth_speed);
		post_fade_filter.value = 1.f;
		post_fades = loadGtgPluginDefault("default_post_fader", false);
		gtg_default_theme = loadGtgPluginDefault("default_theme", 0);
		color_theme = gtg_default_theme;
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
					mini_fader.setSpeed(bypass_speed);

					// bypass the fade even if the fade is already underway
					if (mini_fader.on) {
						mini_fader.on = (mini_fader.getFade() != mini_fader.getGain());
					} else {
						mini_fader.on = (mini_fader.getFade() == 0.f);
					}

				} else {   // normal single click
					auto_override = false;   // do not override automation
					mini_fader.on = !mini_fader.on;
					if (mini_fader.on) {
						mini_fader.setSpeed(int(fade_in));
					} else {
						mini_fader.setSpeed(int(fade_out));
					}
				}
			}
			break;
		case LongPressButton::LONG_PRESS:   // long press to audition

			audition_mixer = true;   // all mixers to audition mode

			if (auditioned) {
				auditioned = false;
				if (mini_fader.temped) {
					mini_fader.temped = false;
					mini_fader.on = false;
				}
			} else {
				auditioned = true;

				if (!mini_fader.on) {
					mini_fader.temped = !mini_fader.temped;   // remember if auditioned mixer is off
				}
			}
			break;
		}

		// process cv trigger
		if (on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			if (!audition_mixer) {
				auto_override = false;   // do not override automation
				mini_fader.on = !mini_fader.on;
			}
		}

		mini_fader.process();

		// process fade states, light, and default theme
		if (light_divider.process()) {

			if (use_default_theme) {
				color_theme = gtg_default_theme;
			}

			if (audition_mixer) {   // all mixers are in audition state

				// bypass all fade automation
				auto_override = true;
				mini_fader.setSpeed(bypass_speed);

				if (auditioned) {   // this mixer is being auditioned
					mini_fader.on =  true;
				} else {   // mute the mixers
					if (mini_fader.on) {
						mini_fader.temped = true;   // remember this mixer was on
					}
					mini_fader.on = false;
				}
			} else {   // stop auditions

				// return to states before auditions
				if (mini_fader.temped) {
					mini_fader.temped = false;
					auto_override = true;
					mini_fader.setSpeed(bypass_speed);
					if (auditioned) {
						mini_fader.on = false;
					} else {
						mini_fader.on = true;
					}
				}

				// turn off auditions
				auditioned = false;
			}

			// process fade speed changes if dragging slider
			if (!auto_override) {
				if (mini_fader.on) {
					if (int(fade_in) != mini_fader.getFade()) {
						mini_fader.setSpeed(int(fade_in));
					}
				} else {
					if (int(fade_out) != mini_fader.getFade()) {
						mini_fader.setSpeed(int(fade_out));
					}
				}
			}

			// set lights
			if (mini_fader.getFade() == mini_fader.getGain()) {
				if (audition_mixer) {
					lights[ON_LIGHT + 0].value = 1.f;   // yellow when auditioned
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = 1.f;   // green light when on
					lights[ON_LIGHT + 1].value = 0.f;
				}
			} else {
				if (mini_fader.temped) {
					lights[ON_LIGHT + 0].value = 0.f;   // red when temporarily muted
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = mini_fader.getFade();  // yellow dimmer when fading
					lights[ON_LIGHT + 1].value = mini_fader.getFade() * 0.5f;
				}
			}
		}

		// get inputs
		float mono_in = 0.f;
		if (mini_fader.fading) {
			mono_in = inputs[MP_INPUT].getVoltageSum() * mini_fader.getExpFade(2.5);
		} else {
			mono_in = inputs[MP_INPUT].getVoltageSum() * mini_fader.getFade();
		}

		// get levels
		float in_levels[3];

		// get red level
		in_levels[2] = params[LEVEL_PARAMS + 2].getValue();

		// slew post fader level
		float post_amount = 1.f;
		if (post_fades) {
			post_amount = post_fade_filter.slew(in_levels[2]);
		} else {
			post_amount = post_fade_filter.slew(1.f);
		}
		for (int sb = 0; sb < 2; sb++) {   // apply post fader level to blue and orange
			in_levels[sb] = params[LEVEL_PARAMS + sb].getValue() * post_amount;
		}

		// calculate three mono outputs
		float bus_outs[3] = {0.f, 0.f, 0.f};
		for (int sb = 0; sb < 3; sb++) {
			bus_outs[sb] = mono_in * in_levels[sb];
		}

		// step through all outputs
		outputs[BUS_OUTPUT].setVoltage(bus_outs[0] + inputs[BUS_INPUT].getPolyVoltage(0), 0);
		outputs[BUS_OUTPUT].setVoltage(bus_outs[0] + inputs[BUS_INPUT].getPolyVoltage(1), 1);
		outputs[BUS_OUTPUT].setVoltage(bus_outs[1] + inputs[BUS_INPUT].getPolyVoltage(2), 2);
		outputs[BUS_OUTPUT].setVoltage(bus_outs[1] + inputs[BUS_INPUT].getPolyVoltage(3), 3);
		outputs[BUS_OUTPUT].setVoltage(bus_outs[2] + inputs[BUS_INPUT].getPolyVoltage(4), 4);
		outputs[BUS_OUTPUT].setVoltage(bus_outs[2] + inputs[BUS_INPUT].getPolyVoltage(5), 5);

		// always set bus outputs for 3 stereo buses out
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// save on button, gain states, and color theme
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(mini_fader.on));
		json_object_set_new(rootJ, "post_fades", json_integer(post_fades));
		json_object_set_new(rootJ, "gain", json_real(mini_fader.getGain()));
		json_object_set_new(rootJ, "fade_in", json_real(fade_in));
		json_object_set_new(rootJ, "fade_out", json_real(fade_out));
		json_object_set_new(rootJ, "audition_mixer", json_integer(audition_mixer));
		json_object_set_new(rootJ, "auditioned", json_integer(auditioned));
		json_object_set_new(rootJ, "temped", json_integer(mini_fader.temped));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		json_object_set_new(rootJ, "use_default_theme", json_integer(use_default_theme));
		return rootJ;
	}

	// load on button, gain states, and color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) mini_fader.on = json_integer_value(input_onJ);
		json_t *post_fadesJ = json_object_get(rootJ, "post_fades");
		if (post_fadesJ) {
			post_fades = json_integer_value(post_fadesJ);
		} else {
			if (input_onJ) post_fades = false;   // do not change existing patches
		}
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) mini_fader.setGain((float)json_real_value(gainJ));
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
		if (tempedJ) mini_fader.temped = json_integer_value(tempedJ);
		json_t *use_default_themeJ = json_object_get(rootJ, "use_default_theme");
		if (use_default_themeJ) {
			use_default_theme = json_integer_value(use_default_themeJ);
		} else {
			if (input_onJ) use_default_theme = false;   // do not change existing patches
		}
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed
	void onSampleRateChange() override {
		if (mini_fader.on) {
			mini_fader.setSpeed(fade_in);
		} else {
			mini_fader.setSpeed(fade_out);
		}
		post_fade_filter.setSlewSpeed(smooth_speed);
	}

	// reset fader on state when initialized
	void onReset() override {
		mini_fader.on = true;
		mini_fader.setGain(1.f);
		fade_in = 26.f;
		fade_out = 26.f;
		post_fades = loadGtgPluginDefault("default_post_fader", 0);
		audition_mixer = false;
	}
};


struct MiniBusWidget : ModuleWidget {
	SvgPanel *night_panel;

	MiniBusWidget(MiniBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MiniBus.svg")));

		// load night panel if not preview
		if (module) {
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MiniBus_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(7.62, 15.20)), module, MiniBus::ON_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.62, 15.20)), module, MiniBus::ON_LIGHT));
		addParam(createThemedParamCentered<gtgBlueKnob>(mm2px(Vec(7.62, 51.0)), module, MiniBus::LEVEL_PARAMS + 0, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgOrangeKnob>(mm2px(Vec(7.62, 67.75)), module, MiniBus::LEVEL_PARAMS + 1, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgRedKnob>(mm2px(Vec(7.62, 84.5)), module, MiniBus::LEVEL_PARAMS + 2, module ? &module->color_theme : NULL));

		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(7.62, 23.20)), true, module, MiniBus::ON_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 35.4)), true, module, MiniBus::MP_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 103.85)), true, module, MiniBus::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 114.1)), false, module, MiniBus::BUS_OUTPUT, module ? &module->color_theme : NULL));
	}

	// build the menu
	void appendContextMenu(Menu* menu) override {
		MiniBus *module = dynamic_cast<MiniBus*>(this->module);

		// add gain levels
		struct GainLevelItem : MenuItem {
			MiniBus *module;
			float gain;
			void onAction(const event::Action &e) override {
				module->mini_fader.setGain(gain);
			}
		};

		struct GainsItem : MenuItem {
			MiniBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string gain_titles[4] = {"No gain (default)", "2x gain", "4x gain", "8x gain"};
				float gain_amounts[4] = {1.f, 2.f, 4.f, 8.f};
				for (int i = 0; i < 4; i++) {
					GainLevelItem *gain_item = new GainLevelItem;
					gain_item->text = gain_titles[i];
					gain_item->rightText = CHECKMARK(module->mini_fader.getGain() == gain_amounts[i]);
					gain_item->module = module;
					gain_item->gain = gain_amounts[i];
					menu->addChild(gain_item);
				}
				return menu;
			}
		};

		// set post fader sends on blue and orange buses
		struct PostFadeItem : MenuItem {
			MiniBus *module;
			int post_fade;
			void onAction(const event::Action &e) override {
				module->post_fades = post_fade;
			}
		};

		struct DefaultFadeItem : MenuItem {
			MiniBus *module;
			int default_fade;
			void onAction(const event::Action &e) override {
				saveGtgPluginDefault("default_post_fader", default_fade);
			}
		};

		struct PostFadesItem : MenuItem {
			MiniBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string fade_titles[2] = {"Normal faders", "Post red fader sends"};
				int post_mode[2] = {0, 1};
				for (int i = 0; i < 2; i++) {
					PostFadeItem *post_item = new PostFadeItem;
					post_item->text = fade_titles[i];
					post_item->rightText = CHECKMARK(module->post_fades == post_mode[i]);
					post_item->module = module;
					post_item->post_fade = post_mode[i];
					menu->addChild(post_item);
				}
		        menu->addChild(new MenuEntry);
				std::string default_fade_titles[2] = {"Default to normal faders", "Default to post fader sends"};
				for (int i = 0; i < 2; i++) {
					DefaultFadeItem *default_item = new DefaultFadeItem;
					default_item->text = default_fade_titles[i];
					default_item->rightText = CHECKMARK(loadGtgPluginDefault("default_post_fader", 0) == i);
					default_item->module = module;
					default_item->default_fade = i;
					menu->addChild(default_item);
				}
				return menu;
			}
		};

		struct ThemeItem : MenuItem {
			MiniBus* module;
			int theme;
			void onAction(const event::Action& e) override {
				if (theme == 10) {
					module->use_default_theme = true;
					module->color_theme = gtg_default_theme;
				} else {
					module->use_default_theme = false;
					module->color_theme = theme;
				}
			}
		};

		struct DefaultThemeItem : MenuItem {
			MiniBus* module;
			int theme;
			void onAction(const event::Action &e) override {
				gtg_default_theme = theme;
				saveGtgPluginDefault("default_theme", theme);
			}
		};

		struct ThemesItem : MenuItem {
			MiniBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string theme_titles[3] = {"Default", "70's Cream", "Night Ride"};
				int theme_selected[3] = {10, 0, 1};
				for (int i = 0; i < 3; i++) {
					ThemeItem *theme_item = new ThemeItem;
					theme_item->text = theme_titles[i];
					if (i == 0) {
						theme_item->rightText = CHECKMARK(module->use_default_theme);
					} else {
						if (!module->use_default_theme) {
							theme_item->rightText = CHECKMARK(module->color_theme == theme_selected[i]);
						}
					}
					theme_item->module = module;
					theme_item->theme = theme_selected[i];
					menu->addChild(theme_item);
				}
		        menu->addChild(new MenuEntry);
				std::string default_theme_titles[2] = {"Default to 70's Cream", "Default to Night Ride"};
				for (int i = 0; i < 2; i++) {
					DefaultThemeItem *default_theme_item = new DefaultThemeItem;
					default_theme_item->text = default_theme_titles[i];
					default_theme_item->rightText = CHECKMARK(gtg_default_theme == i);
					default_theme_item->module = module;
					default_theme_item->theme = i;
					menu->addChild(default_theme_item);
				}
				return menu;
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

		GainsItem *gainsItem = createMenuItem<GainsItem>("Preamp on M/P Input");
		gainsItem->rightText = RIGHT_ARROW;
		gainsItem->module = module;
		menu->addChild(gainsItem);

		PostFadesItem *postFadesItem = createMenuItem<PostFadesItem>("Blue and Orange Levels");
		postFadesItem->rightText = RIGHT_ARROW;
		postFadesItem->module = module;
		menu->addChild(postFadesItem);

		menu->addChild(new MenuEntry);

		ThemesItem *themesItem = createMenuItem<ThemesItem>("Panel Themes");
		themesItem->rightText = RIGHT_ARROW;
		themesItem->module = module;
		menu->addChild(themesItem);

	}

	// display panel based on theme
	void step() override {
		if (module) {
			panel->visible = ((((MiniBus*)module)->color_theme) == 0);
			night_panel->visible = ((((MiniBus*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelMiniBus = createModel<MiniBus, MiniBusWidget>("MiniBus");
