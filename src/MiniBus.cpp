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
		ON_LIGHT,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger on_trigger;
	dsp::SchmittTrigger on_cv_trigger;
	AutoFader mini_fader;
	SimpleSlewer post_fade_filter;

	const int fade_speed = 26;
	const int smooth_speed = 26;
	bool post_fades = false;
	int color_theme = 0;

	MiniBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Level to blue bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Level to orange bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Level to red bus");
		lights[ON_LIGHT].value = 1.f;
		mini_fader.setSpeed(fade_speed);
		post_fade_filter.setSlewSpeed(smooth_speed);
		post_fade_filter.value = 1.f;
		post_fades = loadGtgPluginDefault("default_post_fader", false);
		color_theme = loadGtgPluginDefault("default_theme", 0);
	}

	void process(const ProcessArgs &args) override {

		// on off button controls auto fader to prevent pops
		if (on_trigger.process(params[ON_PARAM].getValue()) + on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			mini_fader.on = !mini_fader.on;
			lights[ON_LIGHT].value = mini_fader.on;
		}

		mini_fader.process();

		// get inputs
		float mono_in = inputs[MP_INPUT].getVoltageSum() * mini_fader.getFade();

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
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load on button, gain states, and color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) {
			mini_fader.on = json_integer_value(input_onJ);
			lights[ON_LIGHT].value = mini_fader.on;
		}
		json_t *post_fadesJ = json_object_get(rootJ, "post_fades");
		if (post_fadesJ) {
			post_fades = json_integer_value(post_fadesJ);
		} else {
			if (input_onJ) post_fades = false;   // do not change existing patches
		}
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) mini_fader.setGain((float)json_real_value(gainJ));
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed
	void onSampleRateChange() override {
		mini_fader.setSpeed(fade_speed);
		post_fade_filter.setSlewSpeed(smooth_speed);
	}

	// reset fader on state when initialized
	void onReset() override {
		mini_fader.on = true;
		lights[ON_LIGHT].value = 1.f;
		mini_fader.setGain(1.f);
		post_fades = loadGtgPluginDefault("default_post_fader", 0);
	}
};


struct MiniBusWidget : ModuleWidget {
	SvgPanel* night_panel;

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
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(7.62, 15.20)), module, MiniBus::ON_LIGHT));
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
		MiniBus* module = dynamic_cast<MiniBus*>(this->module);


		// add theme items to context menu
		struct ThemeItem : MenuItem {
			MiniBus* module;
			int theme;
			void onAction(const event::Action& e) override {
				module->color_theme = theme;
			}
		};

		// set post fader sends on blue or orange buses
		struct PostFaderItem : MenuItem {
			MiniBus* module;
			int post_fade;
			void onAction(const event::Action& e) override {
				module->post_fades = !module->post_fades;
			}
		};

	// add gain levels to context menu
		struct GainItem : MenuItem {
			MiniBus* module;
			float gain;
			void onAction(const event::Action& e) override {
				module->mini_fader.setGain(gain);
			}
		};

		// load default theme
		struct DefaultThemeItem : MenuItem {
			MiniBus* module;
			void onAction(const event::Action &e) override {
				saveGtgPluginDefault("default_theme", rightText.empty());
			}
		};

		// default to post fader sends
		struct DefaultSendItem : MenuItem {
			MiniBus* module;
			void onAction(const event::Action &e) override {
				saveGtgPluginDefault("default_post_fader", rightText.empty());
			}
		};

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Panel Theme"));

		std::string themeTitles[2] = {"70's Cream", "Night Ride"};
		for (int i = 0; i < 2; i++) {
			ThemeItem* themeItem = createMenuItem<ThemeItem>(themeTitles[i]);
			themeItem->rightText = CHECKMARK(module->color_theme == i);
			themeItem->module = module;
			themeItem->theme = i;
			menu->addChild(themeItem);
		}

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Preamp on M/P Input"));

		std::string gainTitles[3] = {"No gain (default)", "2x gain", "4x gain"};
		float gainAmounts[3] = {1.f, 2.f, 4.f};
		for (int i = 0; i < 3; i++) {
			GainItem* gainItem = createMenuItem<GainItem>(gainTitles[i]);
			gainItem->rightText = CHECKMARK(module->mini_fader.getGain() == gainAmounts[i]);
			gainItem->module = module;
			gainItem->gain = gainAmounts[i];
			menu->addChild(gainItem);
		}

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Blue and Orange Input Levels"));

		PostFaderItem* postFaderItem = createMenuItem<PostFaderItem>("Post red fader sends");
		postFaderItem->rightText = CHECKMARK(module->post_fades == true);
		postFaderItem->module = module;
		menu->addChild(postFaderItem);

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
			panel->visible = ((((MiniBus*)module)->color_theme) == 0);
			night_panel->visible = ((((MiniBus*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelMiniBus = createModel<MiniBus, MiniBusWidget>("MiniBus");
