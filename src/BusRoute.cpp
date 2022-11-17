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

	const int fade_speed = 26;
	float delay_buf[1000][6] = {};
	int delay_i = 0;
	int delay_knobs[3] = {0, 0, 0};
	bool bus_audition[3] = {false, false, false};
	bool auditioning = false;
	int color_theme = 0;
	bool use_default_theme = true;

	BusRoute() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(DELAY_PARAMS + 0, 0, 999, 0, "Sample delay on blue bus");
		configParam(DELAY_PARAMS + 1, 0, 999, 0, "Sample delay on orange bus");
		configParam(DELAY_PARAMS + 2, 0, 999, 0, "Sample delay on red bus");
		configParam(ONAU_PARAMS + 0, 0.f, 1.f, 0.f, "Blue bus on (hold to audition)");
		configParam(ONAU_PARAMS + 1, 0.f, 1.f, 0.f, "Orange bus on (hold to audition)");
		configParam(ONAU_PARAMS + 2, 0.f, 1.f, 0.f, "Red bus on (hold to audition)");
		configInput(RETURN_INPUTS + 0, "Blue return left");
		configInput(RETURN_INPUTS + 1, "Blue return right");
		configInput(RETURN_INPUTS + 2, "Orange return left");
		configInput(RETURN_INPUTS + 3, "Orange return right");
		configInput(RETURN_INPUTS + 4, "Red return left");
		configInput(RETURN_INPUTS + 5, "Red return right");
		configInput(BUS_INPUT, "Bus chain");
		configOutput(SEND_OUTPUTS + 0, "Blue send left");
		configOutput(SEND_OUTPUTS + 1, "Blue send right");
		configOutput(SEND_OUTPUTS + 2, "Orange send left");
		configOutput(SEND_OUTPUTS + 3, "Orange send right");
		configOutput(SEND_OUTPUTS + 4, "Red send left");
		configOutput(SEND_OUTPUTS + 5, "Red send right");
		configOutput(BUS_OUTPUT, "Bus chain");
		configOutput(MIX_L_OUTPUT, "Mixed left");
		configOutput(MIX_R_OUTPUT, "Mixed right");
		light_divider.setDivision(512);
		for (int i = 0; i < 3; i++) {
			route_fader[i].setSpeed(fade_speed);
		}
		gtg_default_theme = loadGtgPluginDefault("default_theme", 0);
		color_theme = gtg_default_theme;
	}

	void process(const ProcessArgs &args) override {

		// get button presses
		for (int i = 0; i < 3; i++) {
			switch (onauButtons[i].step(params[ONAU_PARAMS + i])) {
			default:
			case LongPressButton::NO_PRESS:
				break;
			case LongPressButton::SHORT_PRESS:
				if (auditioning) {
					auditioning = false;   // stop auditioning and unmute buses
				} else {
					route_fader[i].on = !route_fader[i].on;
				}
				break;
			case LongPressButton::LONG_PRESS:
				auditioning = true;

				if (bus_audition[i]) {
					bus_audition[i] = false;
					if (route_fader[i].temped) {
						route_fader[i].on = false;
						route_fader[i].temped = false;
					}
				} else {

					bus_audition[i] = true;

					if (!route_fader[i].on) {
						route_fader[i].temped = !route_fader[i].temped;
					}
				}
				break;
			}

			route_fader[i].process();
		}

		// set send or audtion button lights
		if (light_divider.process()) {

			if (use_default_theme) {
				color_theme = gtg_default_theme;
			}

			if (auditioning) {
				for (int i = 0; i < 3; i++) {
					if (bus_audition[i]) {
						route_fader[i].on = true;
					} else {
						if (route_fader[i].on) {
							route_fader[i].temped = true;
						}
						route_fader[i].on = false;
					}
				}
			} else {
				for (int i = 0; i < 3; i++) {
					if (route_fader[i].temped) {
						route_fader[i].temped = false;
						if (bus_audition[i]) {
							route_fader[i].on = false;
						} else {
							route_fader[i].on = true;
						}
					}

					bus_audition[i] = false;
				}
			}

			// set lights
			for (int i = 0; i < 3; i++) {
				if (route_fader[i].on) {
					if (bus_audition[i]) {
						lights[ONAU_LIGHTS + (i * 2)].value = 1.f;   // yellow when auditioned
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 1.f;
					} else {
						lights[ONAU_LIGHTS + (i * 2)].value = 1.f;   // green when on
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 0.f;
					}
				} else {
					if (route_fader[i].temped) {
						lights[ONAU_LIGHTS + (i * 2)].value = 0.f;   // red when muted
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 1.f;
					} else {
						lights[ONAU_LIGHTS + (i * 2)].value = 0.f;   // off
						lights[ONAU_LIGHTS + (i * 2) + 1].value = 0.f;
					}
				}
			}

		}

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
				outputs[SEND_OUTPUTS + chan].setVoltage(delay_buf[delay][chan] * route_fader[sb].getFade());   // left
				outputs[SEND_OUTPUTS + chan + 1].setVoltage(delay_buf[delay][chan + 1] * route_fader[sb].getFade());   // right
			} else {

				bus_out[chan] = delay_buf[delay][chan] * route_fader[sb].getFade();
				bus_out[chan + 1] = delay_buf[delay][chan + 1] * route_fader[sb].getFade();
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
	}

	// save on color theme
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "onau_1", json_integer(route_fader[0].on));
		json_object_set_new(rootJ, "onau_2", json_integer(route_fader[1].on));
		json_object_set_new(rootJ, "onau_3", json_integer(route_fader[2].on));
		json_object_set_new(rootJ, "auditioning", json_integer(auditioning));
		json_object_set_new(rootJ, "bus_audition1", json_integer(bus_audition[0]));
		json_object_set_new(rootJ, "bus_audition2", json_integer(bus_audition[1]));
		json_object_set_new(rootJ, "bus_audition3", json_integer(bus_audition[2]));
		json_object_set_new(rootJ, "temped1", json_integer(route_fader[0].temped));
		json_object_set_new(rootJ, "temped2", json_integer(route_fader[1].temped));
		json_object_set_new(rootJ, "temped3", json_integer(route_fader[2].temped));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		json_object_set_new(rootJ, "use_default_theme", json_integer(use_default_theme));
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

		json_t *auditioningJ = json_object_get(rootJ, "auditioning");
		if (auditioningJ) auditioning = json_integer_value(auditioningJ);

		json_t *bus_audition1j = json_object_get(rootJ, "bus_audition1");
		if (bus_audition1j) bus_audition[0] = json_integer_value(bus_audition1j);
		json_t *bus_audition2j = json_object_get(rootJ, "bus_audition2");
		if (bus_audition2j) bus_audition[1] = json_integer_value(bus_audition2j);
		json_t *bus_audition3j = json_object_get(rootJ, "bus_audition3");
		if (bus_audition3j) bus_audition[2] = json_integer_value(bus_audition3j);

		json_t *temped1j = json_object_get(rootJ, "temped1");
		if (temped1j) route_fader[0].temped = json_integer_value(temped1j);
		json_t *temped2j = json_object_get(rootJ, "temped2");
		if (temped2j) route_fader[1].temped = json_integer_value(temped2j);
		json_t *temped3j = json_object_get(rootJ, "temped3");
		if (temped3j) route_fader[2].temped = json_integer_value(temped3j);

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
		for (int i = 0; i < 3; i++) {
			route_fader[i].setSpeed(fade_speed);
		}
	}

	// reset on audition states when initialized
	void onReset() override {
		auditioning = false;
		for (int i = 0; i < 3; i++) {
			route_fader[i].on = true;
			bus_audition[i] = false;
		}
	}
};


// delay display widget
struct DelayDisplayWidget : TransparentWidget {
	BusRoute *module;
	std::string fontPath="res/fonts/DSEG7-Classic-MINI/DSEG7ClassicMini-Bold.ttf";
	int delay_knob = 0;

	DelayDisplayWidget() {
		box.size = mm2px(Vec(6.519, 4.0));
	}

	void draw(const DrawArgs &args) override {
		int value = module ? module->delay_knobs[delay_knob] : 0;
		std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, fontPath));
		std::string text = string::f("%03d", value);

		// background
		NVGcolor backgroundColor = nvgRGB(26, 26, 26);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.5);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);

		// display text text
		if (font) {
			nvgFontSize(args.vg, 6);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextLetterSpacing(args.vg, 0.5);
			nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
			Vec textPos = mm2px(Vec(6.05, 3.1));
			NVGcolor textColor = nvgRGB(0x90, 0xc7, 0x3e);
			nvgFillColor(args.vg, textColor);
			nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
		}
	}
};


struct BusRouteWidget : ModuleWidget {
	SvgPanel* night_panel;

	BusRouteWidget(BusRoute *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusRoute.svg")));

		// load night panel if not preview
#ifndef USING_CARDINAL_NOT_RACK
		if (module)
#endif
		{
			night_panel = new SvgPanel();
			night_panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusRoute_Night.svg")));
			night_panel->visible = false;
			addChild(night_panel);
		}

		DelayDisplayWidget *blueDisplay = createWidgetCentered<DelayDisplayWidget>(mm2px(Vec(15.25, 23.64)));
		blueDisplay->module = module;
		blueDisplay->delay_knob = 0;
		addChild(blueDisplay);

		DelayDisplayWidget *orangeDisplay = createWidgetCentered<DelayDisplayWidget>(mm2px(Vec(15.25, 52.68)));
		orangeDisplay->module = module;
		orangeDisplay->delay_knob = 1;
		addChild(orangeDisplay);

		DelayDisplayWidget *redDisplay = createWidgetCentered<DelayDisplayWidget>(mm2px(Vec(15.25, 81.68)));
		redDisplay->module = module;
		redDisplay->delay_knob = 2;
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

#ifndef USING_CARDINAL_NOT_RACK
	// build the menu
	void appendContextMenu(Menu* menu) override {
		BusRoute* module = dynamic_cast<BusRoute*>(this->module);

		struct ThemeItem : MenuItem {
			BusRoute* module;
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
			BusRoute* module;
			int theme;
			void onAction(const event::Action &e) override {
				gtg_default_theme = theme;
				saveGtgPluginDefault("default_theme", theme);
			}
		};

		struct ThemesItem : MenuItem {
			BusRoute *module;
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
			panel->visible = ((((BusRoute*)module)->color_theme) == 0);
			night_panel->visible = ((((BusRoute*)module)->color_theme) == 1);
		}
#endif
		Widget::step();
	}
};


Model *modelBusRoute = createModel<BusRoute, BusRouteWidget>("BusRoute");
