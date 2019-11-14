#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


struct BusRoute : Module {
	enum ParamIds {
		ENUMS(DELAY_PARAMS, 3),
		ENUMS(ONAU_PARAMS, 3),
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
		MIX_L_OUTPUT,
		MIX_R_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(ONAU_LIGHTS, 6),
		NUM_LIGHTS
	};

	LongPressButton onauButtons[3];
	dsp::ClockDivider light_divider;
	AutoFader route_fader[3];
	AutoFader mute_fader;

	const int fade_speed = 26;
	float delay_buf[1000][6] = {};
	int delay_i = 0;
	int delay_knobs[3] = {0, 0, 0};
	int bus_audition = 0;
	int color_theme = 0;

	BusRoute() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(DELAY_PARAMS + 0, 0, 999, 0, "Sample delay on blue bus");
		configParam(DELAY_PARAMS + 1, 0, 999, 0, "Sample delay on orange bus");
		configParam(DELAY_PARAMS + 2, 0, 999, 0, "Sample delay on red bus");
		configParam(ONAU_PARAMS + 0, 0.f, 1.f, 0.f, "Blue bus on (hold to audition)");
		configParam(ONAU_PARAMS + 1, 0.f, 1.f, 0.f, "Orange bus on (hold to audition)");
		configParam(ONAU_PARAMS + 2, 0.f, 1.f, 0.f, "Red bus on (hold to audition)");
		light_divider.setDivision(512);
		for (int i = 0; i < 3; i++) {
			route_fader[i].setSpeed(fade_speed);
		}
		mute_fader.setSpeed(fade_speed);
		mute_fader.fade = 1.f;
		color_theme = loadGtgPluginDefault("default_theme", 0);
	}

	void process(const ProcessArgs &args) override {

		// get button presses
		for (int i = 0; i < 3; i++) {
			switch (onauButtons[i].step(params[ONAU_PARAMS + i])) {
			default:
			case LongPressButton::NO_PRESS:
				break;
			case LongPressButton::SHORT_PRESS:
				if (!mute_fader.on) {
					mute_fader.on = true;   // stop auditioning and unmute buses
				} else {
					route_fader[i].on = !route_fader[i].on;
				}
				break;
			case LongPressButton::LONG_PRESS:
				bus_audition = i;
				route_fader[i].on = true;
				mute_fader.on = false;
				break;
			}

			route_fader[i].process();
		}

		mute_fader.process();

		// record bus inputs into delay buffer
		for (int c = 0; c < 6; c++) {
			delay_buf[delay_i][c] = inputs[BUS_INPUT].getPolyVoltage(c);
		}

		// get outputs and sends
		float bus_out[6] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
		float mix_out[2] = {0.f, 0.f};

		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus

			// get delay knob settings, used here and in display values
			delay_knobs[sb] = params[DELAY_PARAMS + sb].getValue();

			// set channel's delay
			int delay = delay_i - delay_knobs[sb];
			if (delay < 0) delay = 1000 + delay;   // adjust delay when buffer rolls

			// set send outputs from delay buffer
			int chan = sb * 2;

			// buses to send outputs or directly to bus out if sends are not connected
			if (outputs[SEND_OUTPUTS + chan].isConnected() || outputs[SEND_OUTPUTS + chan + 1].isConnected()) {
				if (bus_audition == sb) {
					outputs[SEND_OUTPUTS + chan].setVoltage(delay_buf[delay][chan] * route_fader[sb].getFade());   // left
					outputs[SEND_OUTPUTS + chan + 1].setVoltage(delay_buf[delay][chan + 1] * route_fader[sb].getFade());   // right
				} else {
					outputs[SEND_OUTPUTS + chan].setVoltage(delay_buf[delay][chan] * route_fader[sb].getFade() * mute_fader.getFade());   // left
					outputs[SEND_OUTPUTS + chan + 1].setVoltage(delay_buf[delay][chan + 1] * route_fader[sb].getFade() * mute_fader.getFade());   // right
				}
			} else {

				if (bus_audition == sb) {
					bus_out[chan] = delay_buf[delay][chan] * route_fader[sb].getFade();
					bus_out[chan + 1] = delay_buf[delay][chan + 1] * route_fader[sb].getFade();
				} else {
					bus_out[chan] = delay_buf[delay][chan] * route_fader[sb].getFade() * mute_fader.getFade();   // left
					bus_out[chan + 1] = delay_buf[delay][chan + 1] * route_fader[sb].getFade() * mute_fader.getFade();   // right
				}
			}

			// get all returns, even if sends are not connected or off, allows hearing the tail of a return
			bus_out[chan] += inputs[RETURN_INPUTS + chan].getVoltage();
			bus_out[chan + 1] += inputs[RETURN_INPUTS + chan + 1].getVoltage();

			// sum mix out
			mix_out[0] += bus_out[chan];
			mix_out[1] += bus_out[chan + 1];
		}

		// final bus out
		for (int c = 0; c < 6; c++) {
			outputs[BUS_OUTPUT].setVoltage(bus_out[c], c);
		}

		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);

		// final mix out
		outputs[MIX_L_OUTPUT].setVoltage(mix_out[0]);
		outputs[MIX_R_OUTPUT].setVoltage(mix_out[1]);

		// roll buffer index
		delay_i++;
		if (delay_i >= 1000) delay_i = 0;

		// set send or audtion button lights
		if (light_divider.process()) {
			if (mute_fader.on) {   // not in audition mode
				for (int i = 0; i < 3; i++) {
					lights[ONAU_LIGHTS + (i * 2)].value = route_fader[i].getFade();
					lights[ONAU_LIGHTS + 1 + (i * 2)].value = 0.f;
				}
			} else {
				for (int i = 0; i < 3; i++) {
					lights[ONAU_LIGHTS + (i * 2)].value = 0.f;
					lights[ONAU_LIGHTS + 1 + (i * 2)].value = route_fader[i].getFade();   // turn temporarily muted send lights red
				}
				lights[ONAU_LIGHTS + (bus_audition * 2)].value = 1.f;   // the auditioned send will turn yellow
			}
		}
	}

	// save on color theme
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "onau_1", json_integer(route_fader[0].on));
		json_object_set_new(rootJ, "onau_2", json_integer(route_fader[1].on));
		json_object_set_new(rootJ, "onau_3", json_integer(route_fader[2].on));
		json_object_set_new(rootJ, "mute_fader", json_integer(mute_fader.on));
		json_object_set_new(rootJ, "bus_audition", json_integer(bus_audition));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load on color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *onau_1J = json_object_get(rootJ, "onau_1");
		if (onau_1J) route_fader[0].on = json_integer_value(onau_1J);
		json_t *onau_2J = json_object_get(rootJ, "onau_2");
		if (onau_2J) route_fader[1].on = json_integer_value(onau_2J);
		json_t *onau_3J = json_object_get(rootJ, "onau_3");
		if (onau_3J) route_fader[2].on = json_integer_value(onau_3J);
		json_t *mute_faderJ = json_object_get(rootJ, "mute_fader");
		if (mute_faderJ) mute_fader.on = json_integer_value(mute_faderJ);
		json_t *bus_auditionJ = json_object_get(rootJ, "bus_audition");
		if (bus_auditionJ) bus_audition = json_integer_value(bus_auditionJ);
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed
	void onSampleRateChange() override {
		for (int i = 0; i < 3; i++) {
			route_fader[i].setSpeed(fade_speed);
		}
		mute_fader.setSpeed(fade_speed);
	}

	// reset on audition states when initialized
	void onReset() override {
		bus_audition = 0;
		for (int i = 0; i < 3; i++) {
			route_fader[i].on = true;
		}
		mute_fader.on = true;
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

		BlueDisplay *blueDisplay = createWidgetCentered<BlueDisplay>(mm2px(Vec(15.25, 23.64)));
		blueDisplay->module = module;
		addChild(blueDisplay);

		OrangeDisplay *orangeDisplay = createWidgetCentered<OrangeDisplay>(mm2px(Vec(15.25, 52.68)));
		orangeDisplay->module = module;
		addChild(orangeDisplay);

		RedDisplay *redDisplay = createWidgetCentered<RedDisplay>(mm2px(Vec(15.25, 81.68)));
		redDisplay->module = module;
		addChild(redDisplay);

		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));
		addChild(createThemedWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH), module ? &module->color_theme : NULL));

		addParam(createThemedParamCentered<gtgBlueTinySnapKnob>(mm2px(Vec(15.24, 33.624)), module, BusRoute::DELAY_PARAMS + 0, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgOrangeTinySnapKnob>(mm2px(Vec(15.24, 62.672)), module, BusRoute::DELAY_PARAMS + 1, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgRedTinySnapKnob>(mm2px(Vec(15.24, 91.676)), module, BusRoute::DELAY_PARAMS + 2, module ? &module->color_theme : NULL));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(15.24, 13.3)), module, BusRoute::ONAU_PARAMS + 0, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(15.24, 13.3)), module, BusRoute::ONAU_LIGHTS + 0));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(15.24, 42.35)), module, BusRoute::ONAU_PARAMS + 1, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(15.24, 42.35)), module, BusRoute::ONAU_LIGHTS + 2));
		addParam(createThemedParamCentered<gtgBlackTinyButton>(mm2px(Vec(15.24, 71.35)), module, BusRoute::ONAU_PARAMS + 2, module ? &module->color_theme : NULL));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(15.24, 71.35)), module, BusRoute::ONAU_LIGHTS + 4));

		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 19.052)), true, module, BusRoute::RETURN_INPUTS + 0, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 29.196)), true, module, BusRoute::RETURN_INPUTS + 1, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 48.1)), true, module, BusRoute::RETURN_INPUTS + 2, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 58.244)), true, module, BusRoute::RETURN_INPUTS + 3, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 77.104)), true, module, BusRoute::RETURN_INPUTS + 4, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 87.248)), true, module, BusRoute::RETURN_INPUTS + 5, module ? &module->color_theme : NULL));
		addInput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 114.118)), true, module, BusRoute::BUS_INPUT, module ? &module->color_theme : NULL));

		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 19.052)), false, module, BusRoute::SEND_OUTPUTS + 0, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 29.196)), false, module, BusRoute::SEND_OUTPUTS + 1, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 48.1)), false, module, BusRoute::SEND_OUTPUTS + 2, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 58.244)), false, module, BusRoute::SEND_OUTPUTS + 3, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 77.104)), false, module, BusRoute::SEND_OUTPUTS + 4, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 87.248)), false, module, BusRoute::SEND_OUTPUTS + 5, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(6.99, 103.916)), false, module, BusRoute::BUS_OUTPUT, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 103.916)), false, module, BusRoute::MIX_L_OUTPUT, module ? &module->color_theme : NULL));
		addOutput(createThemedPortCentered<gtgNutPort>(mm2px(Vec(23.49, 114.107)), false, module, BusRoute::MIX_R_OUTPUT, module ? &module->color_theme : NULL));
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
				saveGtgPluginDefault("default_theme", rightText.empty());
			}
		};

		struct DefaultSendItem : MenuItem {
			BusRoute* module;
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
