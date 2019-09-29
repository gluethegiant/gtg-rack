#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


struct GigBus : Module {
	enum ParamIds {
		ON_PARAM,
		PAN_PARAM,
		ENUMS(LEVEL_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		ON_CV_INPUT,
		LMP_INPUT,
		R_INPUT,
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
	dsp::ClockDivider pan_divider;
	AutoFader gig_fader;
	ConstantPanner gig_panner;

	const int fade_speed = 20;

	GigBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Post red level to blue stereo bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Post red level to orange stereo bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Master level to red stereo bus");
		pan_divider.setDivision(3);
	}

	void process(const ProcessArgs &args) override {

		// on off button with fader as a pop filter
		if (on_trigger.process(params[ON_PARAM].getValue()) + on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			gig_fader.on = !gig_fader.on;
		}

		gig_fader.process();

		lights[ON_LIGHT].value = gig_fader.getFade();

		// get input levels
		float in_levels[3] = {0.f, 0.f, 0.f};
		in_levels[2] = params[LEVEL_PARAMS + 2].getValue();   // master level
		for (int sb = 0; sb < 2; sb++) {   // send levels
			in_levels[sb] = params[LEVEL_PARAMS + sb].getValue() * in_levels[2];   // multiply by red for post levels
		}

		// get stereo pan levels
		if (pan_divider.process()) {   // optimization
			gig_panner.setPan(params[PAN_PARAM].getValue());
		}

		// process inputs
		float stereo_in[2] = {0.f, 0.f};
		if (inputs[R_INPUT].isConnected()) {   // get a channel from each cable input
			stereo_in[0] = inputs[LMP_INPUT].getVoltage() * gig_panner.getLevel(1) * gig_fader.getFade();
			stereo_in[1] = inputs[R_INPUT].getVoltage() * gig_panner.getLevel(2) * gig_fader.getFade();
		} else {   // split mono or sum of polyphonic cable on LMP
			float lmp_in = inputs[LMP_INPUT].getVoltageSum();
			for (int c = 0; c < 2; c++) {
				stereo_in[c] = lmp_in * gig_panner.getLevel(c) * gig_fader.getFade();
			}
		}

		// process outputs
		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus
			for (int c = 0; c < 2; c++) {
				int bus_channel = (2 * sb) + c;
				outputs[BUS_OUTPUT].setVoltage((stereo_in[c] * in_levels[sb]) + inputs[BUS_INPUT].getPolyVoltage(bus_channel), bus_channel);
			}
		}

		// set bus outputs for 3 stereo buses out
		outputs[BUS_OUTPUT].setChannels(6);
	}

	// save on button states
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(gig_fader.on));
		json_object_set_new(rootJ, "gain", json_real(gig_fader.getGain()));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) gig_fader.on = json_integer_value(input_onJ);
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) gig_fader.setGain((float)json_real_value(gainJ));
	}

	void onSampleRateChange() override {
		gig_fader.setSpeed(fade_speed);
	}

	void onReset() override {
		gig_fader.on = true;
		gig_fader.setGain(1.f);
	}
};


struct GigBusWidget : ModuleWidget {
	GigBusWidget(GigBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/GigBus.svg")));

		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BlackButton>(mm2px(Vec(10.16, 15.20)), module, GigBus::ON_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(10.16, 15.20)), module, GigBus::ON_LIGHT));
		addParam(createParamCentered<GrayKnob>(mm2px(Vec(10.127, 60.32)), module, GigBus::PAN_PARAM));
		addParam(createParamCentered<BlueTinyKnob>(mm2px(Vec(5.91, 72.65)), module, GigBus::LEVEL_PARAMS + 0));
		addParam(createParamCentered<OrangeTinyKnob>(mm2px(Vec(14.41, 72.65)), module, GigBus::LEVEL_PARAMS + 1));
		addParam(createParamCentered<RedKnob>(mm2px(Vec(10.188, 85.539)), module, GigBus::LEVEL_PARAMS + 2));

		addInput(createInputCentered<KeyPort>(mm2px(Vec(10.16, 23.233)), module, GigBus::ON_CV_INPUT));
		addInput(createInputCentered<NutPort>(mm2px(Vec(10.16, 35.583)), module, GigBus::LMP_INPUT));
		addInput(createInputCentered<NutPort>(mm2px(Vec(10.16, 45.746)), module, GigBus::R_INPUT));
		addInput(createInputCentered<NutPort>(mm2px(Vec(10.16, 103.863)), module, GigBus::BUS_INPUT));

		addOutput(createOutputCentered<NutPort>(mm2px(Vec(10.16, 114.108)), module, GigBus::BUS_OUTPUT));
	}

	// add gain levels to context menu
	void appendContextMenu(Menu* menu) override {
		GigBus* module = dynamic_cast<GigBus*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Gain Level"));

		struct GainItem : MenuItem {
			GigBus* module;
			float gain;
			void onAction(const event::Action& e) override {
				module->gig_fader.setGain(gain);
			}
		};

		std::string gainTitles[3] = {"1.0x", "1.5x", "2.0x"};
		float gainAmounts[3] = {1.f, 1.5f, 2.f};
		for (int i = 0; i < 3; i++) {
			GainItem* gainItem = createMenuItem<GainItem>(gainTitles[i]);
			gainItem->rightText = CHECKMARK(module->gig_fader.getGain() == gainAmounts[i]);
			gainItem->module = module;
			gainItem->gain = gainAmounts[i];
			menu->addChild(gainItem);
		}
	}
};


Model *modelGigBus = createModel<GigBus, GigBusWidget>("GigBus");
