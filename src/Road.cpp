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

	const int fade_speed = 26;
	bool bus_audition[6] = {false, false, false, false, false, false};
	bool auditioning = false;
	int color_theme = 0;
	bool use_default_theme = true;

	Road() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAMS + 0, 0.f, 1.f, 0.f, "BUS IN 1 on");
		configParam(ON_PARAMS + 1, 0.f, 1.f, 0.f, "BUS IN 2 on");
		configParam(ON_PARAMS + 2, 0.f, 1.f, 0.f, "BUS IN 3 on");
		configParam(ON_PARAMS + 3, 0.f, 1.f, 0.f, "BUS IN 4 on");
		configParam(ON_PARAMS + 4, 0.f, 1.f, 0.f, "BUS IN 5 on");
		configParam(ON_PARAMS + 5, 0.f, 1.f, 0.f, "BUS IN 6 on");
		light_divider.setDivision(512);
		for (int i = 0; i < 6; i++) {
			road_fader[i].setSpeed(fade_speed);
		}
		gtg_default_theme = loadGtgPluginDefault("default_theme", 0);
		color_theme = gtg_default_theme;
	}

	void process(const ProcessArgs &args) override {

		// get button presses
		for (int i = 0; i < 6; i++) {
			switch (onauButtons[i].step(params[ON_PARAMS + i])) {
			default:
			case LongPressButton::NO_PRESS:
				break;
			case LongPressButton::SHORT_PRESS:
				if (auditioning) {
					auditioning = false;
				} else {
					road_fader[i].on = !road_fader[i].on;
				}
				break;
			case LongPressButton::LONG_PRESS:
				auditioning = true;

				if (bus_audition[i]) {
					bus_audition[i] = false;
					if (road_fader[i].temped) {
						road_fader[i].on = false;
						road_fader[i].temped = false;
					}
				} else {

					bus_audition[i] = true;

					if (!road_fader[i].on) {
						road_fader[i].temped = !road_fader[i].temped;   // remember if bus was off
					}
				}
				break;
			}

			road_fader[i].process();
		}

		if (light_divider.process()) {

			if (use_default_theme) {
				color_theme = gtg_default_theme;
			}

			if (auditioning) {
				for (int i = 0; i < 6; i++) {
					if (bus_audition[i]) {
						road_fader[i].on = true;
					} else {
						if (road_fader[i].on) {
							road_fader[i].temped = true;   // remember this fader was on
						}
						road_fader[i].on = false;
					}
				}
			} else {
				for (int i = 0; i < 6; i++) {
					if (road_fader[i].temped) {
						road_fader[i].temped = false;
						if (bus_audition[i]) {
							road_fader[i].on = false;
						} else {
							road_fader[i].on = true;
						}
					}

					bus_audition[i] = false;
				}
			}

			// set lights
			for (int i = 0; i < 6; i++) {
				if (road_fader[i].on) {
					if (bus_audition[i]) {
						lights[ONAU_LIGHTS + (i * 2)].value = 1.f;   // yellow when auditioned
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 1.f;
					} else {
						lights[ONAU_LIGHTS + (i * 2)].value = 1.f;   // green when on
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 0.f;
					}
				} else {
					if (road_fader[i].temped) {
						lights[ONAU_LIGHTS + (i * 2)].value = 0.f;   // red when muted
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 1.f;
					} else {
						lights[ONAU_LIGHTS + (i * 2)].value = 0.f;   // off
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 0.f;
					}
				}
			}

		}   // end light_divider.process()

		// sum channels from connected buses
		float bus_sum[6] = {};

		for (int b = 0; b < 6; b++) {
			if (inputs[BUS_INPUTS + b].isConnected()) {
				for (int c = 0; c < 6; c++) {
					bus_sum[c] += inputs[BUS_INPUTS + b].getPolyVoltage(c) * road_fader[b].getFade();
				}
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
		json_object_set_new(rootJ, "auditioning", json_integer(auditioning));
		json_object_set_new(rootJ, "bus_audition1", json_integer(bus_audition[0]));
		json_object_set_new(rootJ, "bus_audition2", json_integer(bus_audition[1]));
		json_object_set_new(rootJ, "bus_audition3", json_integer(bus_audition[2]));
		json_object_set_new(rootJ, "bus_audition4", json_integer(bus_audition[3]));
		json_object_set_new(rootJ, "bus_audition5", json_integer(bus_audition[4]));
		json_object_set_new(rootJ, "bus_audition6", json_integer(bus_audition[5]));
		json_object_set_new(rootJ, "temped1", json_integer(road_fader[0].temped));
		json_object_set_new(rootJ, "temped2", json_integer(road_fader[1].temped));
		json_object_set_new(rootJ, "temped3", json_integer(road_fader[2].temped));
		json_object_set_new(rootJ, "temped4", json_integer(road_fader[3].temped));
		json_object_set_new(rootJ, "temped5", json_integer(road_fader[4].temped));
		json_object_set_new(rootJ, "temped6", json_integer(road_fader[5].temped));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		json_object_set_new(rootJ, "use_default_theme", json_integer(use_default_theme));
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

		json_t *auditioningJ = json_object_get(rootJ, "auditioning");
		if (auditioningJ) auditioning = json_integer_value(auditioningJ);

		json_t *bus_audition1j = json_object_get(rootJ, "bus_audition1");
		if (bus_audition1j) bus_audition[0] = json_integer_value(bus_audition1j);
		json_t *bus_audition2j = json_object_get(rootJ, "bus_audition2");
		if (bus_audition2j) bus_audition[1] = json_integer_value(bus_audition2j);
		json_t *bus_audition3j = json_object_get(rootJ, "bus_audition3");
		if (bus_audition3j) bus_audition[2] = json_integer_value(bus_audition3j);
		json_t *bus_audition4j = json_object_get(rootJ, "bus_audition4");
		if (bus_audition4j) bus_audition[3] = json_integer_value(bus_audition4j);
		json_t *bus_audition5j = json_object_get(rootJ, "bus_audition5");
		if (bus_audition5j) bus_audition[4] = json_integer_value(bus_audition5j);
		json_t *bus_audition6j = json_object_get(rootJ, "bus_audition6");
		if (bus_audition6j) bus_audition[5] = json_integer_value(bus_audition6j);

		json_t *temped1j = json_object_get(rootJ, "temped1");
		if (temped1j) road_fader[0].temped = json_integer_value(temped1j);
		json_t *temped2j = json_object_get(rootJ, "temped2");
		if (temped2j) road_fader[1].temped = json_integer_value(temped2j);
		json_t *temped3j = json_object_get(rootJ, "temped3");
		if (temped3j) road_fader[2].temped = json_integer_value(temped3j);
		json_t *temped4j = json_object_get(rootJ, "temped4");
		if (temped4j) road_fader[3].temped = json_integer_value(temped4j);
		json_t *temped5j = json_object_get(rootJ, "temped5");
		if (temped5j) road_fader[4].temped = json_integer_value(temped5j);
		json_t *temped6j = json_object_get(rootJ, "temped6");
		if (temped6j) road_fader[5].temped = json_integer_value(temped6j);

		json_t *use_default_themeJ = json_object_get(rootJ, "use_default_theme");
		if (use_default_themeJ) {
			use_default_theme = json_integer_value(use_default_themeJ);
		} else {
			if (onau_1J) use_default_theme = false;   // do not change existing patches
		}
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed
	void onSampleRateChange() override {
		for (int i = 0; i < 6; i++) {
			road_fader[i].setSpeed(fade_speed);
		}
	}

	// reset on audition states when initialized
	void onReset() override {
		auditioning = false;
		for (int i = 0; i < 6; i++) {
			bus_audition[i] = false;
			road_fader[i].on = true;
		}
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
			Road* module;
			int theme;
			void onAction(const event::Action &e) override {
				gtg_default_theme = theme;
				saveGtgPluginDefault("default_theme", theme);
			}
		};

		struct ThemesItem : MenuItem {
			Road *module;
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

		ThemesItem *themesItem = createMenuItem<ThemesItem>("Panel Themes");
		themesItem->rightText = RIGHT_ARROW;
		themesItem->module = module;
		menu->addChild(themesItem);
	}

	// display the panel based on the theme
	void step() override {
		if (module) {
			Widget* panel = getPanel();
			panel->visible = ((((Road*)module)->color_theme) == 0);
			night_panel->visible = ((((Road*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelRoad = createModel<Road, RoadWidget>("Road");
