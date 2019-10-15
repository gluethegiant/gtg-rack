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

	EnterBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 1.f, "Blue stereo input level");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 1.f, "Orange stereo input level");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Red stereo input level");
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
};

struct EnterBusWidget : ModuleWidget {
	EnterBusWidget(EnterBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EnterBus.svg")));

		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<gtgBlueTinyKnob>(mm2px(Vec(10.37, 34.419)), module, EnterBus::LEVEL_PARAMS + 0));
		addParam(createParamCentered<gtgOrangeTinyKnob>(mm2px(Vec(10.37, 62.909)), module, EnterBus::LEVEL_PARAMS + 1));
		addParam(createParamCentered<gtgRedTinyKnob>(mm2px(Vec(10.37, 91.384)), module, EnterBus::LEVEL_PARAMS + 2));

		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(6.87, 15.302)), module, EnterBus::ENTER_INPUTS + 0));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(6.87, 25.446)), module, EnterBus::ENTER_INPUTS + 1));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(6.87, 43.85)), module, EnterBus::ENTER_INPUTS + 2));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(6.87, 53.994)), module, EnterBus::ENTER_INPUTS + 3));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(6.87, 72.354)), module, EnterBus::ENTER_INPUTS + 4));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(6.87, 82.498)), module, EnterBus::ENTER_INPUTS + 5));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 103.863)), module, EnterBus::BUS_INPUT));

		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 114.107)), module, EnterBus::BUS_OUTPUT));
	}
};


Model *modelEnterBus = createModel<EnterBus, EnterBusWidget>("EnterBus");
