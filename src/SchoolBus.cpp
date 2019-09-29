#include "plugin.hpp"
#include "gtgComponents.hpp"
#include "gtgDSP.hpp"


struct SchoolBus : Module {
	enum ParamIds {
		ON_PARAM,
		PAN_ATT_PARAM,
		PAN_PARAM,
		BLUE_POST_PARAM,
		ORANGE_POST_PARAM,
		ENUMS(LEVEL_PARAMS, 3),
		NUM_PARAMS
	};
	enum InputIds {
		LMP_INPUT,
		R_INPUT,
		ON_CV_INPUT,
		PAN_CV_INPUT,
		ENUMS(LEVEL_CV_INPUTS, 3),
		BUS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		BUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ON_LIGHT,
		BLUE_POST_LIGHT,
		ORANGE_POST_LIGHT,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger on_trigger;
	dsp::SchmittTrigger on_cv_trigger;
	dsp::SchmittTrigger blue_post_trigger;
	dsp::SchmittTrigger orange_post_trigger;
	dsp::ClockDivider pan_divider;
	AutoFader school_fader;
	ConstantPanner school_pan;

	const int fade_speed = 20;
	bool post_fades[2] = {false, false};

	SchoolBus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ON_PARAM, 0.f, 1.f, 0.f, "Input on");
		configParam(PAN_ATT_PARAM, 0.f, 1.f, 0.5f, "Pan attenuator");
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(LEVEL_PARAMS + 0, 0.f, 1.f, 0.f, "Level to blue stereo bus");
		configParam(LEVEL_PARAMS + 1, 0.f, 1.f, 0.f, "Level to orange stereo bus");
		configParam(LEVEL_PARAMS + 2, 0.f, 1.f, 1.f, "Level to red stereo bus");
		configParam(BLUE_POST_PARAM, 0.f, 1.f, 0.f, "Post red fader send");
		configParam(ORANGE_POST_PARAM, 0.f, 1.f, 0.f, "Post red fader send");
		pan_divider.setDivision(3);
	}

	void process(const ProcessArgs &args) override {

		// on off button with fader pop filter
		if (on_trigger.process(params[ON_PARAM].getValue()) + on_cv_trigger.process(inputs[ON_CV_INPUT].getVoltage())) {
			school_fader.on = !school_fader.on;
		}

		school_fader.process();

		lights[ON_LIGHT].value = school_fader.getFade();

		// post fader send buttons
		if (blue_post_trigger.process(params[BLUE_POST_PARAM].getValue())) post_fades[0] = !post_fades[0];
		if (orange_post_trigger.process(params[ORANGE_POST_PARAM].getValue())) post_fades[1] = !post_fades[1];

		lights[BLUE_POST_LIGHT].value = post_fades[0];
		lights[ORANGE_POST_LIGHT].value = post_fades[1];

		// get input levels
		float in_levels[3] = {0.f, 0.f, 0.f};
		for (int sb = 0; sb < 3; sb++) {   // sb = stereo bus
			in_levels[sb] = clamp(inputs[LEVEL_CV_INPUTS + sb].getNormalVoltage(10) * 0.1f, 0.f, 1.f) * params[LEVEL_PARAMS + sb].getValue();
		}

		// set post fades on levels
		for (int i = 0; i < 2; i++) {
			if (post_fades[i]) {
				in_levels[i] *= in_levels[2];
			}
		}

		// get stereo pan levels
		if (pan_divider.process()) {   // calculate pan infrequently, useful for auto panning
			if (inputs[PAN_CV_INPUT].isConnected()) {
				float pan_pos = params[PAN_PARAM].getValue() + (((inputs[PAN_CV_INPUT].getNormalVoltage(0) * 2) * params[PAN_ATT_PARAM].getValue()) * 0.1);
				school_pan.setPan(pan_pos);
			} else {
				school_pan.setPan(params[PAN_PARAM].getValue());
			}
		}

		// process inputs
		float stereo_in[2] = {0.f, 0.f};
		if (inputs[R_INPUT].isConnected()) {   // get a channel from each cable input
			stereo_in[0] = inputs[LMP_INPUT].getVoltage() * school_pan.getLevel(0) * school_fader.getFade();
			stereo_in[1] = inputs[R_INPUT].getVoltage() * school_pan.getLevel(1) * school_fader.getFade();
		} else {   // split mono or sum of polyphonic cable on LMP
			float lmp_in = inputs[LMP_INPUT].getVoltageSum();
			for (int c = 0; c < 2; c++) {
				stereo_in[c] = lmp_in * school_pan.getLevel(c) * school_fader.getFade();
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

	// save on and post_fade send button states
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "input_on", json_integer(school_fader.on));
		json_object_set_new(rootJ, "blue_post_fade", json_integer(post_fades[0]));
		json_object_set_new(rootJ, "orange_post_fade", json_integer(post_fades[1]));
		json_object_set_new(rootJ, "gain", json_real(school_fader.getGain()));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *input_onJ = json_object_get(rootJ, "input_on");
		if (input_onJ) school_fader.on = json_integer_value(input_onJ);
		json_t *blue_post_fadeJ = json_object_get(rootJ, "blue_post_fade");
		if (blue_post_fadeJ) post_fades[0] = json_integer_value(blue_post_fadeJ);
		json_t *orange_post_fadeJ = json_object_get(rootJ, "orange_post_fade");
		if (orange_post_fadeJ) post_fades[1] = json_integer_value(orange_post_fadeJ);
		json_t *gainJ = json_object_get(rootJ, "gain");
		if (gainJ) school_fader.setGain((float)json_real_value(gainJ));
	}
};


struct SchoolBusWidget : ModuleWidget {
	SchoolBusWidget(SchoolBus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SchoolBus.svg")));

		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewUp>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewUp>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BlackButton>(mm2px(Vec(15.24, 15.20)), module, SchoolBus::ON_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(15.24, 15.20)), module, SchoolBus::ON_LIGHT));
		addParam(createParamCentered<GrayTinyKnob>(mm2px(Vec(15.24, 25.9)), module, SchoolBus::PAN_ATT_PARAM));
		addParam(createParamCentered<GrayKnob>(mm2px(Vec(15.24, 41.0)), module, SchoolBus::PAN_PARAM));
		addParam(createParamCentered<BlueKnob>(mm2px(Vec(15.24, 59.2)), module, SchoolBus::LEVEL_PARAMS + 0));
		addParam(createParamCentered<OrangeKnob>(mm2px(Vec(15.24, 77.4)), module, SchoolBus::LEVEL_PARAMS + 1));
		addParam(createParamCentered<RedKnob>(mm2px(Vec(15.24, 95.55)), module, SchoolBus::LEVEL_PARAMS + 2));
		addParam(createParamCentered<BlackButton>(mm2px(Vec(5.3, 59.2)), module, SchoolBus::BLUE_POST_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.3, 59.2)), module, SchoolBus::BLUE_POST_LIGHT));
		addParam(createParamCentered<BlackButton>(mm2px(Vec(5.3, 77.4)), module, SchoolBus::ORANGE_POST_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.3, 77.4)), module, SchoolBus::ORANGE_POST_LIGHT));

		addInput(createInputCentered<NutPort>(mm2px(Vec(7.45, 21.1)), module, SchoolBus::LMP_INPUT));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.45, 31.2)), module, SchoolBus::R_INPUT));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(23.1, 21.1)), module, SchoolBus::ON_CV_INPUT));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(23.1, 31.2)), module, SchoolBus::PAN_CV_INPUT));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(24.6, 51.1)), module, SchoolBus::LEVEL_CV_INPUTS + 0));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(24.6, 69.3)), module, SchoolBus::LEVEL_CV_INPUTS + 1));
		addInput(createInputCentered<KeyPort>(mm2px(Vec(24.6, 87.5)), module, SchoolBus::LEVEL_CV_INPUTS + 2));
		addInput(createInputCentered<NutPort>(mm2px(Vec(7.45, 114.1)), module, SchoolBus::BUS_INPUT));

		addOutput(createOutputCentered<NutPort>(mm2px(Vec(23.1, 114.1)), module, SchoolBus::BUS_OUTPUT));
	}

	// add gain levels to context menu
	void appendContextMenu(Menu* menu) override {
		SchoolBus* module = dynamic_cast<SchoolBus*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Gain Level"));

		struct GainItem : MenuItem {
			SchoolBus* module;
			float gain;
			void onAction(const event::Action& e) override {
				module->school_fader.setGain(gain);
			}
		};

		std::string gainTitles[3] = {"1.0x", "1.5x", "2.0x"};
		float gainAmounts[3] = {1.f, 1.5f, 2.f};
		for (int i = 0; i < 3; i++) {
			GainItem* gainItem = createMenuItem<GainItem>(gainTitles[i]);
			gainItem->rightText = CHECKMARK(module->school_fader.getGain() == gainAmounts[i]);
			gainItem->module = module;
			gainItem->gain = gainAmounts[i];
			menu->addChild(gainItem);
		}
	}
};


Model *modelSchoolBus = createModel<SchoolBus, SchoolBusWidget>("SchoolBus");
