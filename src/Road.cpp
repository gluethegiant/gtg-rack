#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"

struct Road : Module {
	enum ParamIds {
		ENUMS(ON_PARAMS, 6),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(BUS_INPUTS, 6),
		NUM_INPUTS
	};
	enum OutputIds {
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(ONAU_LIGHTS, 12),
		NUM_LIGHTS
	};

	LongPressButton onauButtons[6];
	dsp::ClockDivider light_divider;
	AutoFader road_fader[6];
	AutoFader mute_fader;

	const int fade_speed = 26;
	int bus_audition = 0;
	int color_theme = 0;

	Road() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAMS + 0, 0.f, 1.f, 0.f, "BUS IN 1 on (hold to audition)");
		configParam(ON_PARAMS + 1, 0.f, 1.f, 0.f, "BUS IN 2 on (hold to audition)");
		configParam(ON_PARAMS + 2, 0.f, 1.f, 0.f, "BUS IN 3 on (hold to audition)");
		configParam(ON_PARAMS + 3, 0.f, 1.f, 0.f, "BUS IN 4 on (hold to audition)");
		configParam(ON_PARAMS + 4, 0.f, 1.f, 0.f, "BUS IN 5 on (hold to audition)");
		configParam(ON_PARAMS + 5, 0.f, 1.f, 0.f, "BUS IN 6 on (hold to audition)");
		light_divider.setDivision(512);
		for (int i = 0; i < 6; i++) {
			road_fader[i].setSpeed(fade_speed);
		}
		mute_fader.setSpeed(fade_speed);
		mute_fader.fade = 1.f;
		color_theme = loadDefaultTheme();
	}

	void process(const ProcessArgs &args) override {

		// get button presses
		for (int i = 0; i < 6; i++) {
			switch (onauButtons[i].step(params[ON_PARAMS + i])) {
			default:
			case LongPressButton::NO_PRESS:
				break;
			case LongPressButton::SHORT_PRESS:
				if (!mute_fader.on) {
					mute_fader.on = true;   // stop auditioning and unmute buses
				} else {
					road_fader[i].on = !road_fader[i].on;
				}
				break;
			case LongPressButton::LONG_PRESS:
				bus_audition = i;
				road_fader[i].on = true;
				mute_fader.on = false;
				break;
			}

			road_fader[i].process();
		}

		mute_fader.process();

		// sum channels from connected buses
		float bus_sum[6] = {};

		for (int b = 0; b < 6; b++) {
			if (inputs[BUS_INPUTS + b].isConnected()) {
				if (bus_audition == b) {
					for (int c = 0; c < 6; c++) {
						bus_sum[c] += inputs[BUS_INPUTS + b].getPolyVoltage(c) * road_fader[b].getFade();
					}
				} else {
					for (int c = 0; c < 6; c++) {
						bus_sum[c] += inputs[BUS_INPUTS + b].getPolyVoltage(c) * road_fader[b].getFade() * mute_fader.getFade();
					}
				}
			}
		}

		if (light_divider.process()) {
			if (mute_fader.on) {   // not in audition mode
				for (int i = 0; i < 6; i++) {
					lights[ONAU_LIGHTS + (i * 2)].value = road_fader[i].getFade();
					lights[ONAU_LIGHTS + 1 + (i * 2)].value = 0.f;
				}
			} else {
				for (int i = 0; i < 6; i++) {
					lights[ONAU_LIGHTS + (i * 2)].value = 0.f;
					lights[ONAU_LIGHTS + 1 + (i * 2)].value = road_fader[i].getFade();   // turn temporarily muted bus ins red
				}
				lights[ONAU_LIGHTS + (bus_audition * 2)].value = 1.f;   // the auditioned channel will get green and red and turn yellow
			}
		}

		// set output bus to summed channels
		for (int c = 0; c < 6; c++) {
			outputs[BUS_OUTPUT].setVoltage(bus_sum[c], c);
		}

		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// save color theme
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "onau_1", json_integer(road_fader[0].on));
		json_object_set_new(rootJ, "onau_2", json_integer(road_fader[1].on));
		json_object_set_new(rootJ, "onau_3", json_integer(road_fader[2].on));
		json_object_set_new(rootJ, "onau_4", json_integer(road_fader[3].on));
		json_object_set_new(rootJ, "onau_5", json_integer(road_fader[4].on));
		json_object_set_new(rootJ, "onau_6", json_integer(road_fader[5].on));
		json_object_set_new(rootJ, "mute_fader", json_integer(mute_fader.on));
		json_object_set_new(rootJ, "bus_audition", json_integer(bus_audition));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *onau_1J = json_object_get(rootJ, "onau_1");
		if (onau_1J) road_fader[0].on = json_integer_value(onau_1J);
		json_t *onau_2J = json_object_get(rootJ, "onau_2");
		if (onau_2J) road_fader[1].on = json_integer_value(onau_2J);
		json_t *onau_3J = json_object_get(rootJ, "onau_3");
		if (onau_3J) road_fader[2].on = json_integer_value(onau_3J);
		json_t *onau_4J = json_object_get(rootJ, "onau_4");
		if (onau_4J) road_fader[3].on = json_integer_value(onau_4J);
		json_t *onau_5J = json_object_get(rootJ, "onau_5");
		if (onau_5J) road_fader[4].on = json_integer_value(onau_5J);
		json_t *onau_6J = json_object_get(rootJ, "onau_6");
		if (onau_6J) road_fader[5].on = json_integer_value(onau_6J);
		json_t *mute_faderJ = json_object_get(rootJ, "mute_fader");
		if (mute_faderJ) mute_fader.on = json_integer_value(mute_faderJ);
		json_t *bus_auditionJ = json_object_get(rootJ, "bus_audition");
		if (bus_auditionJ) bus_audition = json_integer_value(bus_auditionJ);
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed
	void onSampleRateChange() override {
		for (int i = 0; i < 6; i++) {
			road_fader[i].setSpeed(fade_speed);
		}
		mute_fader.setSpeed(fade_speed);
	}

	// reset on audition states when initialized
	void onReset() override {
		bus_audition = 0;
		for (int i = 0; i < 6; i++) {
			road_fader[i].on = true;
		}
		mute_fader.on = true;
	}
};


struct RoadWidget : ModuleWidget {
	SvgPanel* night_panel;

	RoadWidget(Road *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Road.svg")));

		// load night panel if not preview
		if (module) {
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Road_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(12.12, 22.613)), module, Road::ON_PARAMS + 0, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(12.12, 22.613)), module, Road::ONAU_LIGHTS + 0));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(12.12, 39.112)), module, Road::ON_PARAMS + 1, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(12.12, 39.122)), module, Road::ONAU_LIGHTS + 2));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(12.12, 55.611)), module, Road::ON_PARAMS + 2, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(12.12, 55.611)), module, Road::ONAU_LIGHTS + 4));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(12.12, 72.111)), module, Road::ON_PARAMS + 3, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(12.12, 72.111)), module, Road::ONAU_LIGHTS + 6));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(12.12, 88.61)), module, Road::ON_PARAMS + 4, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(12.12, 88.61)), module, Road::ONAU_LIGHTS + 8));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(12.12, 105.11)), module, Road::ON_PARAMS + 5, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(12.12, 105.11)), module, Road::ONAU_LIGHTS + 10));

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.12, 17.363)), true, module, Road::BUS_INPUTS + 0, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.12, 33.862)), true, module, Road::BUS_INPUTS + 1, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.12, 50.361)), true, module, Road::BUS_INPUTS + 2, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.12, 66.861)), true, module, Road::BUS_INPUTS + 3, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.12, 83.36)), true, module, Road::BUS_INPUTS + 4, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.12, 99.86)), true, module, Road::BUS_INPUTS + 5, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 114.107)), false, module, Road::BUS_OUTPUT, module ? &module->color_theme : NULL));
	}

	// build the menu
	void appendContextMenu(Menu* menu) override {
		Road* module = dynamic_cast<Road*>(this->module);

		struct ThemeItem : MenuItem {
			Road* module;
			int theme;
			void onAction(const event::Action& e) override {
				module->color_theme = theme;
			}
		};

		struct DefaultThemeItem : MenuItem {
			Road* module;
			void onAction(const event::Action &e) override {
				saveDefaultTheme(rightText.empty());
			}
		};

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Color Themes"));

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

		DefaultThemeItem* defaultThemeItem = createMenuItem<DefaultThemeItem>("Default to Night Ride theme");
		defaultThemeItem->rightText = CHECKMARK(loadDefaultTheme());
		defaultThemeItem->module = module;
		menu->addChild(defaultThemeItem);
	}

	// display the panel based on the theme
	void step() override {
		if (module) {
			panel->visible = ((((Road*)module)->color_theme) == 0);
			night_panel->visible = ((((Road*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelRoad = createModel<Road, RoadWidget>("Road");
