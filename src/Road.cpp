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

	Road() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
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
};


struct RoadWidget : ModuleWidget {
	RoadWidget(Road *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Road.svg")));

		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 19.613)), module, Road::BUS_INPUTS + 0));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 35.363)), module, Road::BUS_INPUTS + 1));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 51.113)), module, Road::BUS_INPUTS + 2));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 66.863)), module, Road::BUS_INPUTS + 3));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 82.613)), module, Road::BUS_INPUTS + 4));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.62, 98.36)), module, Road::BUS_INPUTS + 5));

		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.62, 114.107)), module, Road::BUS_OUTPUT));
	}
};


Model *modelRoad = createModel<Road, RoadWidget>("Road");
