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

struct ThemedSvgSwitch : SvgSwitch {
    int* theme = NULL;
    int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> framesAll;

	void addFrameAll(std::shared_ptr<Svg> svg);
    void step() override;
};

struct ThemedSvgKnob : SvgKnob {
    int* theme = NULL;
    int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> framesAll;

	void setOrientation(float angle);
	void addFrameAll(std::shared_ptr<Svg> svg);
    void step() override;
};

struct ThemedRoundBlackSnapKnob : SvgKnob {
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

struct ThemedSvgPort : SvgPort {
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

struct ThemedSvgScrew : SvgScrew {
	int* theme = NULL;
	int old_theme = -1;
	std::vector<std::shared_ptr<Svg>> frames;

	void addFrame(std::shared_ptr<Svg> svg);
	void step() override;
};

// saving and loading default theme

void saveDefaultTheme(int default_theme);

int loadDefaultTheme();

// custom components
struct gtgBlackButton : ThemedSvgSwitch {
	gtgBlackButton() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton_Night.svg")));
		momentary = true;
	}
};

struct gtgRedKnob : ThemedSvgKnob {
	gtgRedKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgOrangeKnob : ThemedSvgKnob {
	gtgOrangeKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlueKnob : ThemedSvgKnob {
	gtgBlueKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgGrayKnob : ThemedSvgKnob {
	gtgGrayKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlackKnob : ThemedSvgKnob {
	gtgBlackKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlackTinyKnob : ThemedSvgKnob {
	gtgBlackTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgGrayTinyKnob : ThemedSvgKnob {
	gtgGrayTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlueTinyKnob : ThemedSvgKnob {
	gtgBlueTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgOrangeTinyKnob : ThemedSvgKnob {
	gtgOrangeTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgRedTinyKnob : ThemedSvgKnob {
	gtgRedTinyKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlueTinySnapKnob : ThemedRoundBlackSnapKnob {
	gtgBlueTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgOrangeTinySnapKnob : ThemedRoundBlackSnapKnob {
	gtgOrangeTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgRedTinySnapKnob : ThemedRoundBlackSnapKnob {
	gtgRedTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgGrayTinySnapKnob : ThemedRoundBlackSnapKnob {
	gtgGrayTinySnapKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob_Night.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgNutPort : ThemedSvgPort {
	gtgNutPort() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/NutPort.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/NutPort_Night.svg")));
		shadow->box.size = shadow->box.size.div(1.07);   // slight improvement on huge round shadow
		shadow->box.pos = Vec(box.size.x * 0.028, box.size.y * 0.094);
	}
};

struct gtgKeyPort : ThemedSvgPort {
	gtgKeyPort() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/KeyPort.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/KeyPort_Night.svg")));
	}
};

struct gtgScrewUp : ThemedSvgScrew {
	gtgScrewUp() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/ScrewUp.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/ScrewUp_Night.svg")));
	}
};
