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

	int color_theme = 0;

	ExitBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		color_theme = loadDefaultTheme();
	}

	void process(const ProcessArgs &args) override {
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
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}
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

	// add theme items to context menu
	struct ThemeItem : MenuItem {
		ExitBus* module;
		int theme;
		void onAction(const event::Action& e) override {
			module->color_theme = theme;
		}
	};

	// load default theme
	struct DefaultThemeItem : MenuItem {
		ExitBus* module;
		void onAction(const event::Action &e) override {
			saveDefaultTheme(rightText.empty());
		}
	};

	// build the menu
	void appendContextMenu(Menu* menu) override {
		ExitBus* module = dynamic_cast<ExitBus*>(this->module);

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
		menu->addChild(createMenuLabel("Modular Bus Mixer Defaults"));
		menu->addChild(createMenuItem<DefaultThemeItem>("Night Ride theme", CHECKMARK(loadDefaultTheme())));
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
