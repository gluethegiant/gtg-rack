#include "plugin.hpp"
#include "gtg-components.hpp"


struct BusRoute : Module {
	enum ParamIds {
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

	BusRoute() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override {
		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);

		// process sends and returns
		for (int c = 0; c < 6; c++) {
			outputs[SEND_OUTPUTS + c].setVoltage(inputs[BUS_INPUT].getPolyVoltage(c));
			outputs[BUS_OUTPUT].setVoltage(inputs[RETURN_INPUTS + c].getVoltage(), c);
		}

	}
};


struct BusRouteWidget : ModuleWidget {
	BusRouteWidget(BusRoute *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusRoute.svg")));

		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<NutPort>(mm2px(Vec(23.1, 21.05)), module, BusRoute::RETURN_INPUTS + 0));
		addInput(createInputCentered<NutPort>(mm2px(Vec(23.1, 31.2)), module, BusRoute::RETURN_INPUTS + 1));
		addInput(createInputCentered<NutPort>(mm2px(Vec(23.1, 46.85)), module, BusRoute::RETURN_INPUTS + 2));
		addInput(createInputCentered<NutPort>(mm2px(Vec(23.1, 57.0)), module, BusRoute::RETURN_INPUTS + 3));
		addInput(createInputCentered<NutPort>(mm2px(Vec(23.1, 72.85)), module, BusRoute::RETURN_INPUTS + 4));
		addInput(createInputCentered<NutPort>(mm2px(Vec(23.1, 83.0)), module, BusRoute::RETURN_INPUTS + 5));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.75, 114.1)), module, BusRoute::BUS_INPUT));

		addOutput(createOutputCentered<NutPort>(mm2px(Vec(7.75, 21.05)), module, BusRoute::SEND_OUTPUTS + 0));
		addOutput(createOutputCentered<NutPort>(mm2px(Vec(7.75, 31.2)), module, BusRoute::SEND_OUTPUTS + 1));
		addOutput(createOutputCentered<NutPort>(mm2px(Vec(7.75, 46.85)), module, BusRoute::SEND_OUTPUTS + 2));
		addOutput(createOutputCentered<NutPort>(mm2px(Vec(7.75, 57.0)), module, BusRoute::SEND_OUTPUTS + 3));
		addOutput(createOutputCentered<NutPort>(mm2px(Vec(7.75, 72.85)), module, BusRoute::SEND_OUTPUTS + 4));
		addOutput(createOutputCentered<NutPort>(mm2px(Vec(7.75, 83.0)), module, BusRoute::SEND_OUTPUTS + 5));
		addOutput(createOutputCentered<NutPort>(mm2px(Vec(23.1, 114.1)), module, BusRoute::BUS_OUTPUT));
	}
};


Model *modelBusRoute = createModel<BusRoute, BusRouteWidget>("BusRoute");
