#include "plugin.hpp"
#include "gtgComponents.hpp"


struct BusRoute : Module {
	enum ParamIds {
		ENUMS(DELAY_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(RETURN_INPUTS, 6),
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SEND_OUTPUTS, 6),
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float delay_buf[1000][6] = {};
	int delay_i = 0;
	int delay_knobs[3] = {0, 0, 0};
	int color_theme = 0;

	BusRoute() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(DELAY_PARAMS + 0, 0, 999, 0, "Sample delay on blue bus send");
		configParam(DELAY_PARAMS + 1, 0, 999, 0, "Sample delay on orange bus send");
		configParam(DELAY_PARAMS + 2, 0, 999, 0, "Sample delay on red bus send");
		color_theme = loadDefaultTheme();
	}

	void process(const ProcessArgs &args) override {

		// record bus inputs into delay buffer
		for (int c = 0; c < 6; c++) {
			delay_buf[delay_i][c] = inputs[BUS_INPUT].getPolyVoltage(c);
		}

		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus

			// get knobs, used here and in display values
			delay_knobs[sb] = params[DELAY_PARAMS + sb].getValue();

			// set channel's delay
			int delay = delay_i - delay_knobs[sb];
			if (delay < 0) delay = 1000 + delay;   // adjust delay when buffer rolls

			// set send outputs from delay buffer
			int chan = sb * 2;
			outputs[SEND_OUTPUTS + chan].setVoltage(delay_buf[delay][chan]);   // left
			outputs[SEND_OUTPUTS + chan + 1].setVoltage(delay_buf[delay][chan + 1]);   // right
		}

		// roll buffer index
		delay_i++;
		if (delay_i >= 1000) delay_i = 0;

		// get returns
		for (int c = 0; c < 6; c++) {
			outputs[BUS_OUTPUT].setVoltage(inputs[RETURN_INPUTS + c].getVoltage(), c);
		}

		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// save on color theme
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load on color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

};

// TODO use a single display widget object

// blue display widget
struct BlueDisplay : TransparentWidget {
	BusRoute *module;
	std::shared_ptr<Font> font;

	BlueDisplay() {
		box.size = mm2px(Vec(6.519, 4.0));
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/DSEG7-Classic-MINI/DSEG7ClassicMini-Bold.ttf"));
	}

	void draw(const DrawArgs &args) override {
		int value = module ? module->delay_knobs[0] : 0;
		std::string text = string::f("%03d", value);

		// background
		NVGcolor backgroundColor = nvgRGB(26, 26, 26);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.5);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);

		// display text text
		nvgFontSize(args.vg, 6);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 0.5);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
		Vec textPos = mm2px(Vec(6.05, 3.1));
		NVGcolor textColor = nvgRGB(0x90, 0xc7, 0x3e);
		nvgFillColor(args.vg, textColor);
		nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
	}
};

// orange display widget
struct OrangeDisplay : TransparentWidget {
	BusRoute *module;
	std::shared_ptr<Font> font;

	OrangeDisplay() {
		box.size = mm2px(Vec(6.519, 4.0));
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/DSEG7-Classic-MINI/DSEG7ClassicMini-Bold.ttf"));
	}

	void draw(const DrawArgs &args) override {
		int value = module ? module->delay_knobs[1] : 0;
		std::string text = string::f("%03d", value);

		// background
		NVGcolor backgroundColor = nvgRGB(26, 26, 26);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.5);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);

		// display text text
		nvgFontSize(args.vg, 6);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 0.5);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
		Vec textPos = mm2px(Vec(6.05, 3.1));
		NVGcolor textColor = nvgRGB(0x90, 0xc7, 0x3e);
		nvgFillColor(args.vg, textColor);
		nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
	}
};

// red display widget
struct RedDisplay : TransparentWidget {
	BusRoute *module;
	std::shared_ptr<Font> font;

	RedDisplay() {
		box.size = mm2px(Vec(6.519, 4.0));
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/DSEG7-Classic-MINI/DSEG7ClassicMini-Bold.ttf"));
	}

	void draw(const DrawArgs &args) override {
		int value = module ? module->delay_knobs[2] : 0;
		std::string text = string::f("%03d", value);

		// background
		NVGcolor backgroundColor = nvgRGB(26, 26, 26);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.5);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);

		// display text text
		nvgFontSize(args.vg, 6);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 0.5);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
		Vec textPos = mm2px(Vec(6.05, 3.1));
		NVGcolor textColor = nvgRGB(0x90, 0xc7, 0x3e);
		nvgFillColor(args.vg, textColor);
		nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
	}
};

struct BusRouteWidget : ModuleWidget {
	SvgPanel* night_panel;

	BusRouteWidget(BusRoute *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusRoute.svg")));

		// load night panel if not preview
		if (module) {
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusRoute_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		BlueDisplay *blueDisplay = createWidgetCentered<BlueDisplay>(mm2px(Vec(15.25, 12.12)));
		blueDisplay->module = module;
		addChild(blueDisplay);

		OrangeDisplay *orangeDisplay = createWidgetCentered<OrangeDisplay>(mm2px(Vec(15.25, 43.18)));
		orangeDisplay->module = module;
		addChild(orangeDisplay);

		RedDisplay *redDisplay = createWidgetCentered<RedDisplay>(mm2px(Vec(15.25, 74.33)));
		redDisplay->module = module;
		addChild(redDisplay);

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlueTinySnapKnob>(mm2px(Vec(15.25, 26.12)), module, BusRoute::DELAY_PARAMS + 0, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgOrangeTinySnapKnob>(mm2px(Vec(15.25, 57.18)), module, BusRoute::DELAY_PARAMS + 1, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgRedTinySnapKnob>(mm2px(Vec(15.25, 88.03)), module, BusRoute::DELAY_PARAMS + 2, module ? &module->color_theme : NULL));

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.32, 21.05)), true, module, BusRoute::RETURN_INPUTS + 0, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.32, 31.2)), true, module, BusRoute::RETURN_INPUTS + 1, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.32, 52.11)), true, module, BusRoute::RETURN_INPUTS + 2, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.32, 62.23)), true, module, BusRoute::RETURN_INPUTS + 3, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.32, 83.35)), true, module, BusRoute::RETURN_INPUTS + 4, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.32, 93.5)), true, module, BusRoute::RETURN_INPUTS + 5, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.28, 114.1)), true, module, BusRoute::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.28, 21.05)), false, module, BusRoute::SEND_OUTPUTS + 0, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.28, 31.2)), false, module, BusRoute::SEND_OUTPUTS + 1, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.28, 52.11)), false, module, BusRoute::SEND_OUTPUTS + 2, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.28, 62.23)), false, module, BusRoute::SEND_OUTPUTS + 3, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.28, 83.35)), false, module, BusRoute::SEND_OUTPUTS + 4, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(7.28, 93.5)), false, module, BusRoute::SEND_OUTPUTS + 5, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.32, 114.1)), false, module, BusRoute::BUS_OUTPUT, module ? &module->color_theme : NULL));
	}

	// build the menu
	void appendContextMenu(Menu* menu) override {
		BusRoute* module = dynamic_cast<BusRoute*>(this->module);

		struct ThemeItem : MenuItem {
			BusRoute* module;
			int theme;
			void onAction(const event::Action& e) override {
				module->color_theme = theme;
			}
		};

		struct DefaultThemeItem : MenuItem {
			BusRoute* module;
			void onAction(const event::Action &e) override {
				saveDefaultTheme(rightText.empty());
			}
		};

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

		DefaultThemeItem* defaultThemeItem = createMenuItem<DefaultThemeItem>("Night Ride theme");
		defaultThemeItem->rightText = CHECKMARK(loadDefaultTheme());
		defaultThemeItem->module = module;
		menu->addChild(defaultThemeItem);
	}

	// display the panel based on the theme
	void step() override {
		if (module) {
			panel->visible = ((((BusRoute*)module)->color_theme) == 0);
			night_panel->visible = ((((BusRoute*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelBusRoute = createModel<BusRoute, BusRouteWidget>("BusRoute");
