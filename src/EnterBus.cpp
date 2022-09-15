#include "plugin.hpp"
#include "gtgComponents.hpp"


struct EnterBus : Module {
	enum ParamIds {
		ENUMS(LEVEL_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(ENTER_INPUTS, 6),
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	dsp::ClockDivider housekeeping_divider;

	int color_theme = 0;
	bool use_default_theme = true;

	EnterBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 1.f, "Blue stereo input level");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 1.f, "Orange stereo input level");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Red stereo input level");
		configInput(ENTER_INPUTS + 0, "Blue left");
		configInput(ENTER_INPUTS + 1, "Blue right");
		configInput(ENTER_INPUTS + 2, "Orange left");
		configInput(ENTER_INPUTS + 3, "Orange right");
		configInput(ENTER_INPUTS + 4, "Red left");
		configInput(ENTER_INPUTS + 5, "Red right");
		configInput(BUS_INPUT, "Bus chain");
		configOutput(BUS_OUTPUT, "Bus chain");
		housekeeping_divider.setDivision(50000);
		gtg_default_theme = loadGtgPluginDefault("default_theme", 0);
		color_theme = gtg_default_theme;
	}

	void process(const ProcessArgs &args) override {

		if (housekeeping_divider.process()) {
			if (use_default_theme) {
				color_theme = gtg_default_theme;
			}
		}

		// process all inputs and levels to bus
		outputs[BUS_OUTPUT].setVoltage((inputs[ENTER_INPUTS + 0].getVoltage() * params[LEVEL_PARAMS + 0].getValue()) + inputs[BUS_INPUT].getPolyVoltage(0), 0);
		outputs[BUS_OUTPUT].setVoltage((inputs[ENTER_INPUTS + 1].getVoltage() * params[LEVEL_PARAMS + 0].getValue()) + inputs[BUS_INPUT].getPolyVoltage(1), 1);
		outputs[BUS_OUTPUT].setVoltage((inputs[ENTER_INPUTS + 2].getVoltage() * params[LEVEL_PARAMS + 1].getValue()) + inputs[BUS_INPUT].getPolyVoltage(2), 2);
		outputs[BUS_OUTPUT].setVoltage((inputs[ENTER_INPUTS + 3].getVoltage() * params[LEVEL_PARAMS + 1].getValue()) + inputs[BUS_INPUT].getPolyVoltage(3), 3);
		outputs[BUS_OUTPUT].setVoltage((inputs[ENTER_INPUTS + 4].getVoltage() * params[LEVEL_PARAMS + 2].getValue()) + inputs[BUS_INPUT].getPolyVoltage(4), 4);
		outputs[BUS_OUTPUT].setVoltage((inputs[ENTER_INPUTS + 5].getVoltage() * params[LEVEL_PARAMS + 2].getValue()) + inputs[BUS_INPUT].getPolyVoltage(5), 5);

		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// save color theme
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "use_default_theme", json_integer(use_default_theme));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
		json_t *use_default_themeJ = json_object_get(rootJ, "use_default_theme");
		if (use_default_themeJ) {
			use_default_theme = json_integer_value(use_default_themeJ);
		} else {
			if (color_themeJ) use_default_theme = false;   // do not change existing patches
		}
	}
};

struct EnterBusWidget : ModuleWidget {
	SvgPanel* night_panel;

	EnterBusWidget(EnterBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EnterBus.svg")));

		// load night panel if not preview
#ifndef USING_CARDINAL_NOT_RACK
		if (module)
#endif
		{
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EnterBus_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlueTinyKnob>(mm2px(Vec(10.87, 34.419)), module, EnterBus::LEVEL_PARAMS + 0, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgOrangeTinyKnob>(mm2px(Vec(10.87, 62.909)), module, EnterBus::LEVEL_PARAMS + 1, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgRedTinyKnob>(mm2px(Vec(10.87, 91.384)), module, EnterBus::LEVEL_PARAMS + 2, module ? &module->color_theme : NULL));

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.62, 15.302)), true, module, EnterBus::ENTER_INPUTS + 0, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.62, 25.446)), true, module, EnterBus::ENTER_INPUTS + 1, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.62, 43.85)), true, module, EnterBus::ENTER_INPUTS + 2, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.62, 53.994)), true, module, EnterBus::ENTER_INPUTS + 3, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.62, 72.354)), true, module, EnterBus::ENTER_INPUTS + 4, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.62, 82.498)), true, module, EnterBus::ENTER_INPUTS + 5, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 103.863)), true, module, EnterBus::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 114.107)), false, module, EnterBus::BUS_OUTPUT, module ? &module->color_theme : NULL));
	}

#ifndef USING_CARDINAL_NOT_RACK
	// build the menu
	void appendContextMenu(Menu* menu) override {
		EnterBus* module = dynamic_cast<EnterBus*>(this->module);

		struct ThemeItem : MenuItem {
			EnterBus* module;
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
			EnterBus* module;
			int theme;
			void onAction(const event::Action &e) override {
				gtg_default_theme = theme;
				saveGtgPluginDefault("default_theme", theme);
			}
		};

		struct ThemesItem : MenuItem {
			EnterBus *module;
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
#endif

	// display the panel based on the theme
	void step() override {
#ifdef USING_CARDINAL_NOT_RACK
		Widget* panel = getPanel();
		panel->visible = !settings::darkMode;
		night_panel->visible = settings::darkMode;
#else
		if (module) {
			Widget* panel = getPanel();
			panel->visible = ((((EnterBus*)module)->color_theme) == 0);
			night_panel->visible = ((((EnterBus*)module)->color_theme) == 1);
		}
#endif
		Widget::step();
	}
};


Model *modelEnterBus = createModel<EnterBus, EnterBusWidget>("EnterBus");
