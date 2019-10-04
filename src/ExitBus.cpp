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

	ExitBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
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
};


struct ExitBusWidget : ModuleWidget {
	ExitBusWidget(ExitBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ExitBus.svg")));

		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 103.85)), module, ExitBus::BUS_INPUT));

		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 21.05)), module, ExitBus::EXIT_OUTPUTS + 0));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 31.2)), module, ExitBus::EXIT_OUTPUTS + 1));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 46.85)), module, ExitBus::EXIT_OUTPUTS + 2));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 57.0)), module, ExitBus::EXIT_OUTPUTS + 3));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 72.85)), module, ExitBus::EXIT_OUTPUTS + 4));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 83.1)), module, ExitBus::EXIT_OUTPUTS + 5));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 114.1)), module, ExitBus::BUS_OUTPUT));
	}
};


Model *modelExitBus = createModel<ExitBus, ExitBusWidget>("ExitBus");
