// ***
//
// Thanks to Marc Boulé (Geodesics code) for figuring out theme switching and keeping it open source.
// Components based on component library and Andrew Belt's VCV Rack work
//
// ***

#pragma once

#include <rack.hpp>

using namespace rack;

extern Plugin *pluginInstance;


// themed button and knob params
template <class TThemedParam>
TThemedParam* createThemedParamCentered(Vec pos, Module *module, int paramId, int* theme) {
	TThemedParam *o = createParamCentered<TThemedParam>(pos, module, paramId);
	o->theme = theme;
	return o;
}

struct gtgThemedSvgSwitch : SvgSwitch {
	int* theme = NULL;
	int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> framesAll;

	void addFrameAll(std::shared_ptr<Svg> svg);
	void step() override;
};

struct gtgThemedSvgKnob : SvgKnob {
	int* theme = NULL;
	int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> framesAll;

	void setOrientation(float angle);
	void addFrameAll(std::shared_ptr<Svg> svg);
	void step() override;
};

struct gtgThemedRoundBlackSnapKnob : SvgKnob {
	int* theme = NULL;
	int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> framesAll;

	void setOrientation(float angle);
	void addFrameAll(std::shared_ptr<Svg> svg);
	void step() override;
};

// themed port widgets
template <class TThemedPort>
TThemedPort* createThemedPortCentered(Vec pos, bool isInput, Module *module, int portId, int* theme) {
	TThemedPort *o = isInput ?
		createInputCentered<TThemedPort>(pos, module, portId) :
		createOutputCentered<TThemedPort>(pos, module, portId);
	o->theme = theme;
	return o;
}

struct gtgThemedSvgPort : SvgPort {
	int* theme = NULL;
	int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> frames;

	void addFrame(std::shared_ptr<Svg> svg);
	void step() override;
};

// themed widgets for screws
template <class TWidget>
TWidget *createThemedWidget(Vec pos, int* theme) {
	TWidget *o = createWidget<TWidget>(pos);
	o->theme = theme;
	return o;
}

struct gtgThemedSvgScrew : SvgScrew {
	int* theme = NULL;
	int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> frames;

	void addFrame(std::shared_ptr<Svg> svg);
	void step() override;
};

// long press button, from Audible Instruments Segments (Stages)
struct LongPressButton {
	enum Events {
		NO_PRESS,
		SHORT_PRESS,
		LONG_PRESS
	};

	float pressedTime = 0.f;
	dsp::BooleanTrigger trigger;

	Events step(Param &param) {
		Events result = NO_PRESS;

		bool pressed = param.value > 0.f;
		if (pressed && pressedTime >= 0.f) {
			pressedTime += APP->engine->getSampleTime();
			if (pressedTime >= 0.7f) {
				pressedTime = -1.f;
				result = LONG_PRESS;
			}
		}

		// Check if released
		if (trigger.process(!pressed)) {
			if (pressedTime >= 0.f) {
				result = SHORT_PRESS;
			}
			pressedTime = 0.f;
		}

		return result;
	}
};

// saving and loading default theme

void saveGtgPluginDefault(const char* setting, int value);

int loadGtgPluginDefault(const char* setting, int default_value);

// menu helpers

// fade duration for fade slider menu item
struct FadeDuration : Quantity {
	float *srcFadeRate = NULL;
	std::string label = "";

	FadeDuration(float *_srcFadeRate, std::string fade_label) {
		srcFadeRate = _srcFadeRate;
		label = fade_label;
	}
	void setValue(float value) override {
		*srcFadeRate = math::clamp(value, getMinValue(), getMaxValue());
	}
	float getValue() override {
		return *srcFadeRate;
	}
	float getMinValue() override {return 26.0f;}
	float getMaxValue() override {return 34000.0f;}
	float getDefaultValue() override {return 26.0f;}
	float getDisplayValue() override {return getValue() / 1000;}
	std::string getDisplayValueString() override {
		float value = getDisplayValue();
		return string::f("%.1f", value);
	}
	void setDisplayValue(float displayValue) override {setValue(displayValue);}
	std::string getLabel() override {return label;}
	std::string getUnit() override {return " sec";}
};

// fade automation sliders
struct FadeSliderItem : ui::Slider {
	FadeSliderItem(float *fade_rate, std::string fade_label) {
		quantity = new FadeDuration(fade_rate, fade_label);
	}
	~FadeSliderItem() {
		delete quantity;
	}
};

// custom components
struct gtgBlackButton : gtgThemedSvgSwitch {
	gtgBlackButton() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton_0.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton_1.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton_Night_0.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton_Night_1.svg")));
		momentary = true;
	}
};

struct gtgBlackTinyButton : gtgThemedSvgSwitch {
	gtgBlackTinyButton() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyButton_0.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyButton_1.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyButton_Night_0.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyButton_Night_1.svg")));
		momentary = true;
	}
};

struct gtgRedKnob : gtgThemedSvgKnob {
	gtgRedKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgOrangeKnob : gtgThemedSvgKnob {
	gtgOrangeKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgBlueKnob : gtgThemedSvgKnob {
	gtgBlueKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgGrayKnob : gtgThemedSvgKnob {
	gtgGrayKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgBlackKnob : gtgThemedSvgKnob {
	gtgBlackKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgBlackTinyKnob : gtgThemedSvgKnob {
	gtgBlackTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgGrayTinyKnob : gtgThemedSvgKnob {
	gtgGrayTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgBlueTinyKnob : gtgThemedSvgKnob {
	gtgBlueTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgOrangeTinyKnob : gtgThemedSvgKnob {
	gtgOrangeTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgRedTinyKnob : gtgThemedSvgKnob {
	gtgRedTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgBlueTinySnapKnob : gtgThemedRoundBlackSnapKnob {
	gtgBlueTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 0.6f;
	}
};

struct gtgOrangeTinySnapKnob : gtgThemedRoundBlackSnapKnob {
	gtgOrangeTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 0.6f;
	}
};

struct gtgRedTinySnapKnob : gtgThemedRoundBlackSnapKnob {
	gtgRedTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 0.6f;
	}
};

struct gtgGrayTinySnapKnob : gtgThemedRoundBlackSnapKnob {
	gtgGrayTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		speed = 2.2f;
	}
};

struct gtgNutPort : gtgThemedSvgPort {
	gtgNutPort() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/NutPort.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/NutPort_Night.svg")));
		shadow->box.size = shadow->box.size.div(1.07);   // slight improvement on huge round shadow
		shadow->box.pos = Vec(box.size.x * 0.028, box.size.y * 0.094);
	}
};

struct gtgKeyPort : gtgThemedSvgPort {
	gtgKeyPort() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/KeyPort.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/KeyPort_Night.svg")));
	}
};

struct gtgScrewUp : gtgThemedSvgScrew {
	gtgScrewUp() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/ScrewUp.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/ScrewUp_Night.svg")));
	}
};
