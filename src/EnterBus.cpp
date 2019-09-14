#include "plugin.hpp"
#include "components.hpp"


struct EnterBus : Module {
	enum ParamIds {
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

	EnterBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override {
		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);

		// process all inputs and outputs
		for (int c = 0; c < 6; c++) {
			outputs[BUS_OUTPUT].setVoltage(inputs[ENTER_INPUTS + c].getVoltage() + inputs[BUS_INPUT].getPolyVoltage(c), c);
		}
	}
};


struct EnterBusWidget : ModuleWidget {
	EnterBusWidget(EnterBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EnterBus.svg")));

		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<NutPort>(mm2px(Vec(7.62, 21.05)), module, EnterBus::ENTER_INPUTS + 0));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.62, 31.2)), module, EnterBus::ENTER_INPUTS + 1));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.62, 46.85)), module, EnterBus::ENTER_INPUTS + 2));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.62, 57.0)), module, EnterBus::ENTER_INPUTS + 3));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.62, 72.85)), module, EnterBus::ENTER_INPUTS + 4));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.62, 83.0)), module, EnterBus::ENTER_INPUTS + 5));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.62, 103.85)), module, EnterBus::BUS_INPUT));

		addOutput(createOutputCentered<NutPort>(mm2px(Vec(7.62, 114.1)), module, EnterBus::BUS_OUTPUT));
	}
};


Model *modelEnterBus = createModel<EnterBus, EnterBusWidget>("EnterBus");
