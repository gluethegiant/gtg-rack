
struct BlackButton : app::SvgSwitch {
	BlackButton() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackButton.svg")));
		momentary = true;
	}
};

struct RedKnob : app::SvgKnob {
	RedKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct OrangeKnob : app::SvgKnob {
	OrangeKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct BlueKnob : app::SvgKnob {
	BlueKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct GrayKnob : app::SvgKnob {
	GrayKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct BlackKnob : app::SvgKnob {
	BlackKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct BlackTinyKnob : app::SvgKnob {
	BlackTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlackTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct GrayTinyKnob : app::SvgKnob {
	GrayTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/GrayTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct BlueTinyKnob : app::SvgKnob {
	BlueTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BlueTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct OrangeTinyKnob : app::SvgKnob {
	OrangeTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/OrangeTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct RedTinyKnob : app::SvgKnob {
	RedTinyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/RedTinyKnob.svg")));
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};

struct NutPort : app::SvgPort {
	NutPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/NutPort.svg")));
		shadow->box.size = shadow->box.size.div(1.07);   // slight improvement on huge round shadow
		shadow->box.pos = Vec(box.size.x * 0.028, box.size.y * 0.08);
	}
};

struct KeyPort : app::SvgPort {
	KeyPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/KeyPort.svg")));
	}
};

struct ScrewUp : app::SvgScrew {
	ScrewUp()
		{
			setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/ScrewUp.svg")));
		}
};
