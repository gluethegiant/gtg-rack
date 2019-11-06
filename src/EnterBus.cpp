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

	int color_theme = 0;

	EnterBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 1.f, "Blue stereo input level");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 1.f, "Orange stereo input level");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Red stereo input level");
		color_theme = loadDefaultTheme();
	}

	void process(const ProcessArgs &args) override {

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
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}
};

struct EnterBusWidget : ModuleWidget {
	SvgPanel* night_panel;

	EnterBusWidget(EnterBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EnterBus.svg")));

		// load night panel if not preview
		if (module) {
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

	// build the menu
	void appendContextMenu(Menu* menu) override {
		EnterBus* module = dynamic_cast<EnterBus*>(this->module);

		struct ThemeItem : MenuItem {
			EnterBus* module;
			int theme;
			void onAction(const event::Action& e) override {
				module->color_theme = theme;
			}
		};

		struct DefaultThemeItem : MenuItem {
			EnterBus* module;
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
			panel->visible = ((((EnterBus*)module)->color_theme) == 0);
			night_panel->visible = ((((EnterBus*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelEnterBus = createModel<EnterBus, EnterBusWidget>("EnterBus");
