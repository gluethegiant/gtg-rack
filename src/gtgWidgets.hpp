#pragma once

struct gtgBlackButton : app::SvgSwitch {
	gtgBlackButton() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton.svg")));
		momentary = true;
	}
};

struct gtgRedKnob : app::SvgKnob {
	gtgRedKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgOrangeKnob : app::SvgKnob {
	gtgOrangeKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlueKnob : app::SvgKnob {
	gtgBlueKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgGrayKnob : app::SvgKnob {
	gtgGrayKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlackKnob : app::SvgKnob {
	gtgBlackKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlackTinyKnob : app::SvgKnob {
	gtgBlackTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgGrayTinyKnob : app::SvgKnob {
	gtgGrayTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlueTinyKnob : app::SvgKnob {
	gtgBlueTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgOrangeTinyKnob : app::SvgKnob {
	gtgOrangeTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgRedTinyKnob : app::SvgKnob {
	gtgRedTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgBlueTinySnapKnob : RoundBlackSnapKnob {
	gtgBlueTinySnapKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgOrangeTinySnapKnob : RoundBlackSnapKnob {
	gtgOrangeTinySnapKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgRedTinySnapKnob : RoundBlackSnapKnob {
	gtgRedTinySnapKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgGrayTinySnapKnob : RoundBlackSnapKnob {
	gtgGrayTinySnapKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct gtgNutPort : app::SvgPort {
	gtgNutPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/NutPort.svg")));
		shadow->box.size = shadow->box.size.div(1.07);   // slight improvement on huge round shadow
		shadow->box.pos = Vec(box.size.x * 0.028, box.size.y * 0.094);
	}
};

struct gtgKeyPort : app::SvgPort {
	gtgKeyPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/KeyPort.svg")));
	}
};

struct gtgScrewUp : app::SvgScrew {
	gtgScrewUp()
		{
			setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/ScrewUp.svg")));
		}
};
