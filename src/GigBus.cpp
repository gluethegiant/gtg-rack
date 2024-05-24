#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


struct GigBus : Module {
	enum ParamIds {
		ON_PARAM,
		PAN_PARAM,
		ENUMS(LEVEL_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		ON_CV_INPUT,
		LMP_INPUT,
		R_INPUT,
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(ON_LIGHT, 2),
		ENUMS(LEFT_LIGHTS, 11),
		ENUMS(RIGHT_LIGHTS, 11),
		NUM_LIGHTS
	};

	dsp::VuMeter2 vu_meters[2];
    dsp::ClockDivider housekeeping_divider;
	dsp::ClockDivider vu_divider;
	dsp::ClockDivider light_divider;
	dsp::ClockDivider audition_divider;
	LongPressButton on_button;
	dsp::SchmittTrigger on_cv_trigger;
	dsp::ClockDivider pan_divider;
	AutoFader gig_fader;
	ConstantPan gig_pan;
	SimpleSlewer post_fade_filter;

	const int bypass_speed = 26;
	const int smooth_speed = 26;
	float fade_in = 26.f;
	float fade_out = 26.f;
	bool auto_override = false;
	bool post_fades = true;
	bool auditioned = false;
	float peak_stereo[2] = {0.f, 0.f};
	int color_theme = 0;
	bool use_default_theme = true;

	GigBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Post red level send to blue stereo bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Post red level send to orange stereo bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Master level to red stereo bus");
		configInput(ON_CV_INPUT, "On CV");
		configInput(LMP_INPUT, "Left, mono, or poly");
		configInput(R_INPUT, "Right");
		configInput(BUS_INPUT, "Bus chain");
		configOutput(BUS_OUTPUT, "Bus chain");
		vu_meters[0].lambda = 25.f;
		vu_meters[1].lambda = 25.f;
		housekeeping_divider.setDivision(50000);
		vu_divider.setDivision(32);
		light_divider.setDivision(240);
		audition_divider.setDivision(512);
		pan_divider.setDivision(3);
		gig_fader.setSpeed(fade_in);
		post_fade_filter.setSlewSpeed(smooth_speed);
		post_fade_filter.value = 1.f;
		gtg_default_theme = loadGtgPluginDefault("default_theme", 0);
		color_theme = gtg_default_theme;
	}

	void process(const ProcessArgs &args) override {

		// check default theme and reset vu meters
		if (housekeeping_divider.process()) {
			if (use_default_theme) {
				color_theme = gtg_default_theme;
			}
			vu_meters[0].v = 0.f;
			vu_meters[1].v = 0.f;
		}

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
					gig_fader.setSpeed(bypass_speed);

					// bypass the fade even if the fade is already underway
					if (gig_fader.on) {
						gig_fader.on = (gig_fader.getFade() != gig_fader.getGain());
					} else {
						gig_fader.on = (gig_fader.getFade() == 0.f);
					}

				} else {   // normal single click
					auto_override = false;   // do not override automation
					gig_fader.on = !gig_fader.on;
					if (gig_fader.on) {
						gig_fader.setSpeed(int(fade_in));
					} else {
						gig_fader.setSpeed(int(fade_out));
					}
				}
			}
			break;
		case LongPressButton::LONG_PRESS:   // long press to audition

			audition_mixer = true;   // all mixers to audition mode

			if (auditioned) {
				auditioned = false;
				if (gig_fader.temped) {
					gig_fader.on = false;
					gig_fader.temped = false;
				}
			} else {
				auditioned = true;
				if (!gig_fader.on) {
					gig_fader.temped = !gig_fader.temped;   // remember if auditioned mixer is off
				}
			}
			break;
		}

		// process cv trigger
		if (on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			if (!audition_mixer) {
				auto_override = false;   // do not override automation
				gig_fader.on = !gig_fader.on;
			}
		}

		gig_fader.process();

		if (audition_divider.process()) {
			if (audition_mixer) {   // all mixers are in audition state

				// bypass all fade automation
				auto_override = true;
				gig_fader.setSpeed(bypass_speed);

				if (auditioned) {   // this mixer is being auditioned
					gig_fader.on =  true;
				} else {   // mute the mixers
					if (gig_fader.on) {
						gig_fader.temped = true;   // remember this mixer was on
					}
					gig_fader.on = false;
				}
			} else {   // stop auditions

				// return to states before auditions
				if (gig_fader.temped) {
					gig_fader.temped = false;
					auto_override = true;
					gig_fader.setSpeed(bypass_speed);
					if (auditioned) {
						gig_fader.on = false;
					} else {
						gig_fader.on = true;
					}
				}

				// turn off auditions
				auditioned = false;
			}

			// process fade speed changes if dragging slider
			if (!auto_override) {
				if (gig_fader.on) {
					if (int(fade_in) != gig_fader.getFade()) {
						gig_fader.setSpeed(int(fade_in));
					}
				} else {
					if (int(fade_out) != gig_fader.getFade()) {
						gig_fader.setSpeed(int(fade_out));
					}
				}
			}
		}

		// define input levels
		float in_levels[3] = {0.f, 0.f, 0.f};

		// get red level
		in_levels[2] = params[LEVEL_PARAMS + 2].getValue();   // master red level

		// slew a post fader level if needed
		float post_amount = 1.f;
		if (post_fades) {
			post_amount = post_fade_filter.slew(in_levels[2]);
		} else {
			post_amount = post_fade_filter.slew(1.f);
		}

		// get orange and blue levels
		for (int sb = 0; sb < 2; sb++) {   // send levels
			in_levels[sb] = params[LEVEL_PARAMS + sb].getValue() * post_amount;   // multiply by master for post send levels
		}

		// get stereo pan levels
		if (pan_divider.process()) {   // optimized by checking pan every few samples
			gig_pan.setPan(params[PAN_PARAM].getValue());
		}

		// get exponential fade
		float exp_fade = 0.f;
		if (gig_fader.fading) {
			exp_fade = gig_fader.getExpFade(2.5);
		} else {
			exp_fade = gig_fader.getFade();
		}

		// process inputs
		float stereo_in[2] = {0.f, 0.f};
		if (inputs[R_INPUT].isConnected()) {   // get a channel from each cable input
			stereo_in[0] = inputs[LMP_INPUT].getVoltage() * gig_pan.getLevel(0) * exp_fade;
			stereo_in[1] = inputs[R_INPUT].getVoltage() * gig_pan.getLevel(1) * exp_fade;
		} else {   // split mono or sum of polyphonic cable on LMP
			float lmp_in = inputs[LMP_INPUT].getVoltageSum();
			for (int c = 0; c < 2; c++) {
				stereo_in[c] = lmp_in * gig_pan.getLevel(c) * exp_fade;
			}
		}

		// check for peaks on red
		for (int c = 0; c < 2; c++) {
			if (stereo_in[c] * in_levels[2] > 10.f) peak_stereo[c] = 1.f;
		}

		// get levels for lights
		if (vu_divider.process()) {   // check levels for lights infrequently
			for (int i = 0; i < 2; i++) {
				float red_level = stereo_in[i] * in_levels[2];
				vu_meters[i].process(args.sampleTime * vu_divider.getDivision(), red_level / 10.f);
			}
		}

		// set lights infrequently
		if (light_divider.process()) {   // set lights infrequently

			// set on light
			if (gig_fader.getFade() == gig_fader.getGain()) {
				if (audition_mixer) {
					lights[ON_LIGHT + 0].value = 1.f;   // yellow when auditioned
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = 1.f;   // green when on
					lights[ON_LIGHT + 1].value = 0.f;
				}
			} else {
				if (gig_fader.temped) {
					lights[ON_LIGHT + 0].value = 0.f;   // red when muted by audition
					lights[ON_LIGHT + 1].value = 1.f;
				} else {
					lights[ON_LIGHT + 0].value = gig_fader.getFade();   // yellow dimmer when fading
					lights[ON_LIGHT + 1].value = gig_fader.getFade() * 0.5f;
				}
			}

			// make peak lights stay on when hit
			for (int c = 0; c < 2; c++) {
				if (peak_stereo[c] > 0) peak_stereo[c] -= 120.f / args.sampleRate; else peak_stereo[c] = 0.f;
			}
			lights[LEFT_LIGHTS + 0].setBrightness(peak_stereo[0]);
			lights[RIGHT_LIGHTS + 0].setBrightness(peak_stereo[1]);

			// green and yellow vu lights
			for (int i = 1; i < 6; i++) {
				lights[LEFT_LIGHTS + i].setBrightness(vu_meters[0].getBrightness((-3 * i), -3 * (i - 1)));
				lights[RIGHT_LIGHTS + i].setBrightness(vu_meters[1].getBrightness((-3 * i), -3 * (i - 1)));
			}
			lights[LEFT_LIGHTS + 6].setBrightness(vu_meters[0].getBrightness(-19, -15));
			lights[RIGHT_LIGHTS + 6].setBrightness(vu_meters[1].getBrightness(-19, -15));
			lights[LEFT_LIGHTS + 7].setBrightness(vu_meters[0].getBrightness(-24, -19));
			lights[RIGHT_LIGHTS + 7].setBrightness(vu_meters[1].getBrightness(-24, -19));
			lights[LEFT_LIGHTS + 8].setBrightness(vu_meters[0].getBrightness(-30, -24));
			lights[RIGHT_LIGHTS + 8].setBrightness(vu_meters[1].getBrightness(-30, -24));
			lights[LEFT_LIGHTS + 9].setBrightness(vu_meters[0].getBrightness(-36, -28));
			lights[RIGHT_LIGHTS + 9].setBrightness(vu_meters[1].getBrightness(-36, -28));
			lights[LEFT_LIGHTS + 10].setBrightness(vu_meters[0].getBrightness(-48, -36));
			lights[RIGHT_LIGHTS + 10].setBrightness(vu_meters[1].getBrightness(-48, -36));
		}

		// process outputs
		outputs[BUS_OUTPUT].setVoltage((stereo_in[0] * in_levels[0]) + inputs[BUS_INPUT].getPolyVoltage(0), 0);
		outputs[BUS_OUTPUT].setVoltage((stereo_in[1] * in_levels[0]) + inputs[BUS_INPUT].getPolyVoltage(1), 1);
		outputs[BUS_OUTPUT].setVoltage((stereo_in[0] * in_levels[1]) + inputs[BUS_INPUT].getPolyVoltage(2), 2);
		outputs[BUS_OUTPUT].setVoltage((stereo_in[1] * in_levels[1]) + inputs[BUS_INPUT].getPolyVoltage(3), 3);
		outputs[BUS_OUTPUT].setVoltage((stereo_in[0] * in_levels[2]) + inputs[BUS_INPUT].getPolyVoltage(4), 4);
		outputs[BUS_OUTPUT].setVoltage((stereo_in[1] * in_levels[2]) + inputs[BUS_INPUT].getPolyVoltage(5), 5);

		// set bus outputs for 3 stereo buses out
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// save on button and gain states
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(gig_fader.on));
		json_object_set_new(rootJ, "post_fades", json_integer(post_fades));
		json_object_set_new(rootJ, "gain", json_real(gig_fader.getGain()));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		json_object_set_new(rootJ, "use_default_theme", json_integer(use_default_theme));
		json_object_set_new(rootJ, "fade_in", json_real(fade_in));
		json_object_set_new(rootJ, "fade_out", json_real(fade_out));
		json_object_set_new(rootJ, "audition_mixer", json_integer(audition_mixer));
		json_object_set_new(rootJ, "auditioned", json_integer(auditioned));
		json_object_set_new(rootJ, "temped", json_integer(gig_fader.temped));
		return rootJ;
	}

	// load on button and gain states
	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) gig_fader.on = json_integer_value(input_onJ);
		json_t *post_fadesJ = json_object_get(rootJ, "post_fades");
		if (post_fadesJ) post_fades = json_integer_value(post_fadesJ);
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) gig_fader.setGain((float)json_real_value(gainJ));
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
		if (tempedJ) gig_fader.temped = json_integer_value(tempedJ);
		json_t *use_default_themeJ = json_object_get(rootJ, "use_default_theme");
		if (use_default_themeJ) {
			use_default_theme = json_integer_value(use_default_themeJ);
		} else {
			if (input_onJ) use_default_theme = false;   // do not change existing patches
		}
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed with new sample rate
	void onSampleRateChange() override {
		if (gig_fader.on) {
			gig_fader.setSpeed(fade_in);
		} else {
			gig_fader.setSpeed(fade_out);
		}
	}

	// reset on state on initialize
	void onReset() override {
		gig_fader.on = true;
		gig_fader.setGain(1.f);
		fade_in = 26.f;
		fade_out = 26.f;
		post_fades = true;
		audition_mixer = false;
	}
};


struct GigBusWidget : ModuleWidget {
	SvgPanel *night_panel;

	GigBusWidget(GigBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/GigBus.svg")));

		// load night panel if not preview
#ifndef USING_CARDINAL_NOT_RACK
		if (module)
#endif
		{
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/GigBus_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlackButton>(mm2px(Vec(10.13, 15.20)), module, GigBus::ON_PARAM, module ? &module->color_theme : NULL));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(10.13, 15.20)), module, GigBus::ON_LIGHT));
		addParam(createThemedParamCentered<gtgGrayKnob>(mm2px(Vec(10.13, 61.25)), module, GigBus::PAN_PARAM, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlueTinyKnob>(mm2px(Vec(5.4, 73.7)), module, GigBus::LEVEL_PARAMS + 0, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgOrangeTinyKnob>(mm2px(Vec(14.90, 73.7)), module, GigBus::LEVEL_PARAMS + 1, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgRedKnob>(mm2px(Vec(10.13, 86.52)), module, GigBus::LEVEL_PARAMS + 2, module ? &module->color_theme : NULL));

		addInput(createThemedPortCentered<gtgKeyPort>(mm2px(Vec(10.13, 23.233)), true, module, GigBus::ON_CV_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(10.13, 35.583)), true, module, GigBus::LMP_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(10.13, 45.746)), true, module, GigBus::R_INPUT, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(10.13, 103.863)), true, module, GigBus::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(10.13, 114.108)), false, module, GigBus::BUS_OUTPUT, module ? &module->color_theme : NULL));

		// create vu lights
		for (int i = 0; i < 11; i++) {
			float spacing = i * 3.25f;
			float top = 15.f;
			if (i < 1 ) {
				addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(3.27, top + spacing)), module, GigBus::LEFT_LIGHTS + i));
				addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.0, top + spacing)), module, GigBus::RIGHT_LIGHTS + i));
			} else {
				if (i < 2) {
					addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(3.27, top + spacing)), module, GigBus::LEFT_LIGHTS + i));
					addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(17.0, top + spacing)), module, GigBus::RIGHT_LIGHTS + i));
				} else {
					addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(3.27, top + spacing)), module, GigBus::LEFT_LIGHTS + i));
					addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(17.0, top + spacing)), module, GigBus::RIGHT_LIGHTS + i));
				}
			}
		}
	}

	// build the context menu
	void appendContextMenu(Menu* menu) override {
		GigBus* module = dynamic_cast<GigBus*>(this->module);

		struct GainLevelItem : MenuItem {
			GigBus* module;
			float gain;
			void onAction(const event::Action& e) override {
				module->gig_fader.setGain(gain);
			}
		};

		struct GainsItem : MenuItem {
			GigBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string gain_titles[4] = {"No gain (default)", "2x gain", "4x gain", "8x gain"};
				float gain_amounts[4] = {1.f, 2.f, 4.f, 8.f};
				for (int i = 0; i < 4; i++) {
					GainLevelItem *gain_item = new GainLevelItem;
					gain_item->text = gain_titles[i];
					gain_item->rightText = CHECKMARK(module->gig_fader.getGain() == gain_amounts[i]);
					gain_item->module = module;
					gain_item->gain = gain_amounts[i];
					menu->addChild(gain_item);
				}
				return menu;
			}
		};

		// set post fader sends on blue and orange buses
		struct PostFadeItem : MenuItem {
			GigBus *module;
			int post_fade;
			void onAction(const event::Action &e) override {
				module->post_fades = post_fade;
			}
		};

		struct PostFadesItem : MenuItem {
			GigBus *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::string fade_titles[2] = {"Normal faders", "Post red fader sends (default)"};
				int post_mode[2] = {0, 1};
				for (int i = 0; i < 2; i++) {
					PostFadeItem *post_item = new PostFadeItem;
					post_item->text = fade_titles[i];
					post_item->rightText = CHECKMARK(module->post_fades == post_mode[i]);
					post_item->module = module;
					post_item->post_fade = post_mode[i];
					menu->addChild(post_item);
				}
				return menu;
			}
		};

		struct ThemeItem : MenuItem {
			GigBus* module;
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
			GigBus* module;
			int theme;
			void onAction(const event::Action &e) override {
				gtg_default_theme = theme;
				saveGtgPluginDefault("default_theme", theme);
			}
		};

		struct ThemesItem : MenuItem {
			GigBus *module;
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

		// fade automation
		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Fade Automation"));

		FadeSliderItem *fadeInSliderItem = new FadeSliderItem(&(module->fade_in), "In");
		fadeInSliderItem->box.size.x = 190.f;
		menu->addChild(fadeInSliderItem);

		FadeSliderItem *fadeOutSliderItem = new FadeSliderItem(&(module->fade_out), "Out");
		fadeOutSliderItem->box.size.x = 190.f;
		menu->addChild(fadeOutSliderItem);

		// mixer settings
		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Mixer Settings"));

		GainsItem *gainsItem = createMenuItem<GainsItem>("Preamps on L/M/P/R Inputs");
		gainsItem->rightText = RIGHT_ARROW;
		gainsItem->module = module;
		menu->addChild(gainsItem);

		PostFadesItem *postFadesItem = createMenuItem<PostFadesItem>("Blue and Orange Levels");
		postFadesItem->rightText = RIGHT_ARROW;
		postFadesItem->module = module;
		menu->addChild(postFadesItem);

#ifndef USING_CARDINAL_NOT_RACK
		menu->addChild(new MenuEntry);

		ThemesItem *themesItem = createMenuItem<ThemesItem>("Panel Themes");
		themesItem->rightText = RIGHT_ARROW;
		themesItem->module = module;
		menu->addChild(themesItem);
#endif
	}

	// display panel based on theme
	void step() override {
#ifdef USING_CARDINAL_NOT_RACK
		Widget* panel = getPanel();
		panel->visible = !settings::preferDarkPanels;
		night_panel->visible = settings::preferDarkPanels;
#else
		if (module) {
			Widget* panel = getPanel();
			panel->visible = ((((GigBus*)module)->color_theme) == 0);
			night_panel->visible = ((((GigBus*)module)->color_theme) == 1);
		}
#endif
		Widget::step();
	}
};


Model *modelGigBus = createModel<GigBus, GigBusWidget>("GigBus");
