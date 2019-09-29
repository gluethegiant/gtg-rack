#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"

struct BusDepot : Module {
	enum ParamIds {
		ON_PARAM,
		AUX_PARAM,
		LEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ON_CV_INPUT,
		LEVEL_CV_INPUT,
		LMP_INPUT,
		R_INPUT,
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ON_LIGHT,
		ENUMS(LEFT_LIGHTS, 9),
		ENUMS(RIGHT_LIGHTS, 9),
		NUM_LIGHTS
	};

	dsp::VuMeter2 vu_meters[2];
	dsp::ClockDivider vu_divider;
	dsp::ClockDivider light_divider;
	dsp::SchmittTrigger on_trigger;
	dsp::SchmittTrigger on_cv_trigger;
	AutoFader depot_fader;

	const int fade_speed = 50;
	float peak_left = 0;
	float peak_right = 0;

	BusDepot() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Output on");
		configParam(AUX_PARAM, 0.f, 1.f, 1.f, "Aux level in");
		configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Master level");
		vu_meters[0].lambda = 15.f;
		vu_meters[1].lambda = 15.f;
		vu_divider.setDivision(512);
		light_divider.setDivision(64);
		depot_fader.setSpeed(fade_speed);
	}

	void process(const ProcessArgs &args) override {
		// on off button with fader onramp to filter pops
		if (on_trigger.process(params[ON_PARAM].getValue()) + on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			depot_fader.on = !depot_fader.on;
		}

		depot_fader.process();

		lights[ON_LIGHT].value = depot_fader.getFade();

		// process sound
		float summed_out[2] = {0.f, 0.f};
		if (depot_fader.getFade() > 0) {   // process only when sound is playing

			float stereo_in[2] = {0.f, 0.f};

			// get param levels
			float aux_level = params[AUX_PARAM].getValue();
			float master_level = clamp(inputs[LEVEL_CV_INPUT].getNormalVoltage(10.0) * 0.1, 0.0f, 1.0f) * params[LEVEL_PARAM].getValue();

			// get aux inputs
			if (inputs[R_INPUT].isConnected()) {   // get a channel from each cable
				stereo_in[0] = inputs[LMP_INPUT].getVoltage() * aux_level;
				stereo_in[1] = inputs[R_INPUT].getVoltage() * aux_level;
			} else {   // get mono polyphonic cable sum from LMP
				float lmp_in = inputs[LMP_INPUT].getVoltageSum() * aux_level;
				for (int c = 0; c < 2; c++) {
					stereo_in[c] = lmp_in;
				}
			}

			// calculate stereo mix from three stereo buses on bus input
			for (int c = 0; c < 2; c++) {
				summed_out[c] = (stereo_in[c] + inputs[BUS_INPUT].getPolyVoltage(c) + inputs[BUS_INPUT].getPolyVoltage(c + 2) + inputs[BUS_INPUT].getPolyVoltage(c + 4)) * master_level * depot_fader.getFade();
			}

			outputs[LEFT_OUTPUT].setVoltage(summed_out[0]);
			outputs[RIGHT_OUTPUT].setVoltage(summed_out[1]);
		}

		// hit peak lights
		if (summed_out[0] > 10.f) peak_left = 1;
		if (summed_out[1] > 10.f) peak_right = 1;

		// get levels for lights
		if (vu_divider.process()) {   // check levels infrequently
			for (int i = 0; i < 2; i++) {
				vu_meters[i].process(args.sampleTime * vu_divider.getDivision(), summed_out[i] / 10.f);
			}
		}

		if (light_divider.process()) {   // set lights infrequently
			// make peak lights stay on when hit
			if (peak_left > 0) peak_left -= 15 / args.sampleRate; else peak_left = 0;
			if (peak_right > 0) peak_right -= 15 / args.sampleRate; else peak_right = 0;
			lights[LEFT_LIGHTS + 0].setBrightness(peak_left);
			lights[RIGHT_LIGHTS + 0].setBrightness(peak_right);
			// green and yellow lights
			for (int i = 1; i < 9; i++) {
				lights[LEFT_LIGHTS + i].setBrightness(vu_meters[0].getBrightness((-6 * i), -6 * (i - 1)));
				lights[RIGHT_LIGHTS + i].setBrightness(vu_meters[1].getBrightness((-6 * i), -6 * (i - 1)));
			}
		}
	}

	// save on button state
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(depot_fader.on));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) depot_fader.on = json_integer_value(input_onJ);
	}

	void onSampleRateChange() override {
		depot_fader.setSpeed(fade_speed);
	}

	void onReset() override {
		depot_fader.on = true;
	}
};


struct BusDepotWidget : ModuleWidget {
	BusDepotWidget(BusDepot *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusDepot.svg")));

		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BlackButton>(mm2px(Vec(15.24, 15.20)), module, BusDepot::ON_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(15.24, 15.20)), module, BusDepot::ON_LIGHT));
		addParam(createParamCentered<BlackTinyKnob>(mm2px(Vec(23.1, 31.2)), module, BusDepot::AUX_PARAM));
		addParam(createParamCentered<BlackKnob>(mm2px(Vec(15.24, 80.65)), module, BusDepot::LEVEL_PARAM));

		addInput(createInputCentered<KeyPort>(mm2px(Vec(23.1, 21.1)), module, BusDepot::ON_CV_INPUT));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(15.24, 68.6)), module, BusDepot::LEVEL_CV_INPUT));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.45, 21.1)), module, BusDepot::LMP_INPUT));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.45, 31.2)), module, BusDepot::R_INPUT));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.45, 114.1)), module, BusDepot::BUS_INPUT));

		addOutput(createOutputCentered<NutPort>(mm2px(Vec(23.1, 103.85)), module, BusDepot::LEFT_OUTPUT));
		addOutput(createOutputCentered<NutPort>(mm2px(Vec(23.1, 114.1)), module, BusDepot::RIGHT_OUTPUT));

		// create vu lights
		for (int i = 0; i < 9; i++) {
			float spacing = i * 5.25;
			if (i < 1 ) {
				addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.45, 47.25 + spacing)), module, BusDepot::LEFT_LIGHTS + i));
				addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.1, 47.25 + spacing)), module, BusDepot::RIGHT_LIGHTS + i));
			} else {
				if (i < 2) {
					addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(5.45, 47.25 + spacing)), module, BusDepot::LEFT_LIGHTS + i));
					addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(25.1, 47.25 + spacing)), module, BusDepot::RIGHT_LIGHTS + i));
				} else {
					addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(5.45, 47.25 + spacing)), module, BusDepot::LEFT_LIGHTS + i));
					addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(25.1, 47.25 + spacing)), module, BusDepot::RIGHT_LIGHTS + i));
				}
			}
			}
	}
};


Model *modelBusDepot = createModel<BusDepot, BusDepotWidget>("BusDepot");
