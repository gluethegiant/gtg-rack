#include "plugin.hpp"
#include "gtgComponents.hpp"


struct ExitBus : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(EXIT_OUTPUTS, 6),
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	dsp::ClockDivider housekeeping_divider;

	int color_theme = 0;
	bool use_default_theme = true;

	ExitBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
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

		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);

		// process all inputs and outputs
		for (int c = 0; c < 6; c++) {
			outputs[EXIT_OUTPUTS + c].setVoltage(inputs[BUS_INPUT].getPolyVoltage(c));
			outputs[BUS_OUTPUT].setVoltage(inputs[BUS_INPUT].getPolyVoltage(c), c);
		}
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
		}	}
};


struct ExitBusWidget : ModuleWidget {
	SvgPanel* night_panel;

	ExitBusWidget(ExitBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ExitBus.svg")));

		// load night panel if not preview
		if (module) {
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ExitBus_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 103.863)), true, module, ExitBus::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 21.052)), false, module, ExitBus::EXIT_OUTPUTS + 0, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 31.196)), false, module, ExitBus::EXIT_OUTPUTS + 1, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 47.1)), false, module, ExitBus::EXIT_OUTPUTS + 2, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 57.244)), false, module, ExitBus::EXIT_OUTPUTS + 3, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 73.104)), false, module, ExitBus::EXIT_OUTPUTS + 4, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 83.248)), false, module, ExitBus::EXIT_OUTPUTS + 5, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 114.107)), false, module, ExitBus::BUS_OUTPUT, module ? &module->color_theme : NULL));
	}

	// build the menu
	void appendContextMenu(Menu* menu) override {
		ExitBus* module = dynamic_cast<ExitBus*>(this->module);

		struct ThemeItem : MenuItem {
			ExitBus* module;
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
			ExitBus* module;
			int theme;
			void onAction(const event::Action &e) override {
				gtg_default_theme = theme;
				saveGtgPluginDefault("default_theme", theme);
			}
		};

		struct ThemesItem : MenuItem {
			ExitBus *module;
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
			panel->visible = ((((ExitBus*)module)->color_theme) == 0);
			night_panel->visible = ((((ExitBus*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelExitBus = createModel<ExitBus, ExitBusWidget>("ExitBus");
