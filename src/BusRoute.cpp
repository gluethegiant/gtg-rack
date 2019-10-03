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

	float blue_buf[201][2] = {};
	float orange_buf[201][2] = {};
	float red_buf[201][2] = {};
	int blue_i = 0;
	int orange_i = 0;
	int red_i = 0;

	BusRoute() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(DELAY_PARAMS + 0, 0, 200, 0, "Sample delay on blue bus return");
		configParam(DELAY_PARAMS + 1, 0, 200, 0, "Sample delay on orange bus return");
		configParam(DELAY_PARAMS + 2, 0, 200, 0, "Sample delay on red bus return");
	}

	void process(const ProcessArgs &args) override {

		// sends
		for (int c = 0; c < 6; c++) {
			outputs[SEND_OUTPUTS + c].setVoltage(inputs[BUS_INPUT].getPolyVoltage(c));
		}

		// blue bus returns

		// record inputs into buffer
		blue_buf[blue_i][0] = inputs[RETURN_INPUTS + 0].getVoltage();
		blue_buf[blue_i][1] = inputs[RETURN_INPUTS + 1].getVoltage();

		// get knob
		int blue_delay = params[DELAY_PARAMS + 0].getValue();

		// get delay index
		int delay_i = blue_i - blue_delay;
		if (delay_i < 0) delay_i = 201 + delay_i;   // adjust delay index when buffer rolls

		// get outputs from buffer
		outputs[BUS_OUTPUT].setVoltage(blue_buf[delay_i][0], 0);
		outputs[BUS_OUTPUT].setVoltage(blue_buf[delay_i][1], 1);

		// roll buffer
		blue_i++;
		if (blue_i >= 201) blue_i = 0;

		// orange bus returns

		// record inputs into buffer
		orange_buf[orange_i][0] = inputs[RETURN_INPUTS + 2].getVoltage();
		orange_buf[orange_i][1] = inputs[RETURN_INPUTS + 3].getVoltage();

		// get knob
		int orange_delay = params[DELAY_PARAMS + 1].getValue();

		// get delay index
		delay_i = orange_i - orange_delay;
		if (delay_i < 0) delay_i = 201 + delay_i;   // adjust delay index when buffer rolls

		// get outputs from buffer
		outputs[BUS_OUTPUT].setVoltage(orange_buf[delay_i][0], 2);
		outputs[BUS_OUTPUT].setVoltage(orange_buf[delay_i][1], 3);

		// roll buffer
		orange_i++;
		if (orange_i >= 201) orange_i = 0;

		// red bus returns

		// record inputs into buffer
		red_buf[red_i][0] = inputs[RETURN_INPUTS + 4].getVoltage();
		red_buf[red_i][1] = inputs[RETURN_INPUTS + 5].getVoltage();

		// get knob
		int red_delay = params[DELAY_PARAMS + 2].getValue();

		// get delay index
		delay_i = red_i - red_delay;
		if (delay_i < 0) delay_i = 201 + delay_i;   // adjust delay index when buffer rolls

		// get outputs from buffer
		outputs[BUS_OUTPUT].setVoltage(red_buf[delay_i][0], 4);
		outputs[BUS_OUTPUT].setVoltage(red_buf[delay_i][1], 5);

		// roll buffer
		red_i++;
		if (red_i >= 201) red_i = 0;

		// set output to 3 stereo buses
		outputs[BUS_OUTPUT].setChannels(6);
	}
};


struct BusRouteWidget : ModuleWidget {
	BusRouteWidget(BusRoute *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusRoute.svg")));

		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<gtgScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<gtgScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<gtgBlueTinySnapKnob>(mm2px(Vec(15.25, 26.12)), module, BusRoute::DELAY_PARAMS + 0));
		addParam(createParamCentered<gtgOrangeTinySnapKnob>(mm2px(Vec(15.25, 57.18)), module, BusRoute::DELAY_PARAMS + 1));
		addParam(createParamCentered<gtgRedTinySnapKnob>(mm2px(Vec(15.25, 88.03)), module, BusRoute::DELAY_PARAMS + 2));

		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(23.32, 21.05)), module, BusRoute::RETURN_INPUTS + 0));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(23.32, 31.2)), module, BusRoute::RETURN_INPUTS + 1));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(23.32, 52.11)), module, BusRoute::RETURN_INPUTS + 2));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(23.32, 62.23)), module, BusRoute::RETURN_INPUTS + 3));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(23.32, 83.35)), module, BusRoute::RETURN_INPUTS + 4));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(23.32, 93.5)), module, BusRoute::RETURN_INPUTS + 5));
		addInput(createInputCentered<gtgNutPort>(mm2px(Vec(7.28, 114.1)), module, BusRoute::BUS_INPUT));

		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.28, 21.05)), module, BusRoute::SEND_OUTPUTS + 0));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.28, 31.2)), module, BusRoute::SEND_OUTPUTS + 1));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.28, 52.11)), module, BusRoute::SEND_OUTPUTS + 2));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.28, 62.23)), module, BusRoute::SEND_OUTPUTS + 3));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.28, 83.35)), module, BusRoute::SEND_OUTPUTS + 4));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(7.28, 93.5)), module, BusRoute::SEND_OUTPUTS + 5));
		addOutput(createOutputCentered<gtgNutPort>(mm2px(Vec(23.32, 114.1)), module, BusRoute::BUS_OUTPUT));
	}
};


Model *modelBusRoute = createModel<BusRoute, BusRouteWidget>("BusRoute");
