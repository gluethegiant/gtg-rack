#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


struct MiniBus : Module {
	enum ParamIds {
		ON_PARAM,
		ENUMS(LEVEL_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		ON_CV_INPUT,
		MP_INPUT,
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ON_LIGHT,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger on_trigger;
	dsp::SchmittTrigger on_cv_trigger;
	AutoFader mini_fader;

	const int fade_speed = 20;
	int color_theme = 0;

	MiniBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Level to blue bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Level to orange bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Level to red bus");
		mini_fader.setSpeed(fade_speed);
	}

	void process(const ProcessArgs &args) override {

		// on off button controls auto fader to prevent pops
		if (on_trigger.process(params[ON_PARAM].getValue()) + on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			mini_fader.on = !mini_fader.on;
		}

		mini_fader.process();

		lights[ON_LIGHT].value = mini_fader.getFade();

		// process inputs
		float mono_in = inputs[MP_INPUT].getVoltageSum() * mini_fader.getFade();

		// process outputs
		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus
			float in_level = params[LEVEL_PARAMS + sb].getValue();
			for (int c = 0; c < 2; c++) {
				int bus_channel = (2 * sb) + c;
				outputs[BUS_OUTPUT].setVoltage((mono_in * in_level) + inputs[BUS_INPUT].getPolyVoltage(bus_channel), bus_channel);
			}
		}

		// set bus outputs for 3 stereo buses out
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// save on button, gain states, and color theme
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(mini_fader.on));
		json_object_set_new(rootJ, "gain", json_real(mini_fader.getGain()));
		json_object_set_new(rootJ, "color_theme", json_integer(color_theme));
		return rootJ;
	}

	// load on button, gain states, and color theme
	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) mini_fader.on = json_integer_value(input_onJ);
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) mini_fader.setGain((float)json_real_value(gainJ));
		json_t *color_themeJ = json_object_get(rootJ, "color_theme");
		if (color_themeJ) color_theme = json_integer_value(color_themeJ);
	}

	// reset fader speed
	void onSampleRateChange() override {
		mini_fader.setSpeed(fade_speed);
	}

	// reset fader on state when initialized
	void onReset() override {
		mini_fader.on = true;
		mini_fader.setGain(1.f);
	}
};


struct MiniBusWidget : ModuleWidget {
	SvgPanel* nightPanel;

	MiniBusWidget(MiniBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MiniBus.svg")));

		// load night panel if not preview
		if (module) {
			nightPanel = new SvgPanel();
			nightPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MiniBus_Night.svg")));
			nightPanel->visible = false;
			addChild(nightPanel);
		}

		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<gtgBlackButton>(mm2px(Vec(7.62, 15.20)), module, MiniBus::ON_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(7.62, 15.20)), module, MiniBus::ON_LIGHT));
		addParam(createParamCentered<gtgBlueKnob>(mm2px(Vec(7.62, 51.0)), module, MiniBus::LEVEL_PARAMS + 0));
		addParam(createParamCentered<gtgOrangeKnob>(mm2px(Vec(7.62, 67.75)), module, MiniBus::LEVEL_PARAMS + 1));
		addParam(createParamCentered<gtgRedKnob>(mm2px(Vec(7.62, 84.5)), module, MiniBus::LEVEL_PARAMS + 2));

		addInput(createInputCentered<gtgKeyPort>(mm2px(Vec(7.62, 23.20)), module, MiniBus::ON_CV_INPUT));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 35.4)), module, MiniBus::MP_INPUT));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 103.85)), module, MiniBus::BUS_INPUT));

		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 114.1)), module, MiniBus::BUS_OUTPUT));
	}

	// add theme items to context menu
	struct ThemeItem : MenuItem {
		MiniBus* module;
		int theme;
		void onAction(const event::Action& e) override {
			module->color_theme = theme;
		}
	};

	// add gain levels to context menu
	struct GainItem : MenuItem {
		MiniBus* module;
		float gain;
		void onAction(const event::Action& e) override {
			module->mini_fader.setGain(gain);
		}
	};

	void appendContextMenu(Menu* menu) override {
		MiniBus* module = dynamic_cast<MiniBus*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Color Theme"));

		std::string themeTitles[2] = {"70's Cream", "Night Riding"};
		for (int i = 0; i < 2; i++) {
			ThemeItem* themeItem = createMenuItem<ThemeItem>(themeTitles[i]);
			themeItem->rightText = CHECKMARK(module->color_theme == i);
			themeItem->module = module;
			themeItem->theme = i;
			menu->addChild(themeItem);
		}

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Preamp on M/P Input"));

		std::string gainTitles[3] = {"No gain (default)", "2x gain", "4x gain"};
		float gainAmounts[3] = {1.f, 2.f, 4.f};
		for (int i = 0; i < 3; i++) {
			GainItem* gainItem = createMenuItem<GainItem>(gainTitles[i]);
			gainItem->rightText = CHECKMARK(module->mini_fader.getGain() == gainAmounts[i]);
			gainItem->module = module;
			gainItem->gain = gainAmounts[i];
			menu->addChild(gainItem);
		}
	}

	void step() override {
		if (module) {
			panel->visible = ((((MiniBus*)module)->color_theme) == 0);
			nightPanel->visible = ((((MiniBus*)module)->color_theme) == 1);
		}
		Widget::step();
	}
};


Model *modelMiniBus = createModel<MiniBus, MiniBusWidget>("MiniBus");
