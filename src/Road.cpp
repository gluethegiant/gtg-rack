#include "plugin.hpp"
#include "gtgComponents.hpp"

struct Road : Module {
	enum ParamIds {
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
		NUM_LIGHTS
	};

	int color_theme = 0;

	Road() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		color_theme = loadDefaultTheme();
	}

	void process(const ProcessArgs &args) override {

		// sum channels from connected buses
		float bus_sum[6] = {};
		for (int b = 0; b < 6; b++) {
			if (inputs[BUS_INPUTS + b].isConnected()) {
				for (int c = 0; c < 6; c++) {
					bus_sum[c] += inputs[BUS_INPUTS + b].getPolyVoltage(c);
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
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
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

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 19.613)), true, module, Road::BUS_INPUTS + 0, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 35.363)), true, module, Road::BUS_INPUTS + 1, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 51.113)), true, module, Road::BUS_INPUTS + 2, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 66.863)), true, module, Road::BUS_INPUTS + 3, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 82.613)), true, module, Road::BUS_INPUTS + 4, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.62, 98.36)), true, module, Road::BUS_INPUTS + 5, module ? &module->color_theme : NULL));

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
