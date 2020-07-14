#include "gtgComponents.hpp"


// themed button (switch)
void ThemedSvgSwitch::addFrameAll(std::shared_ptr<Svg> svg) {
	framesAll.push_back(svg);
	if (framesAll.size() == 2) {
		addFrame(framesAll[0]);
		addFrame(framesAll[1]);
	}
}

void ThemedSvgSwitch::step() {
	if(theme != NULL && *theme != old_theme) {
		if ((*theme) == 0 || framesAll.size() < 4) {
			frames[0]=framesAll[0];
			frames[1]=framesAll[1];
		}
		else {
			frames[0]=framesAll[2];
			frames[1]=framesAll[3];
		}
		old_theme = *theme;
		onChange(*(new event::Change()));
		fb->dirty = true;
	}
	SvgSwitch::step();
}

// themed knob
void ThemedSvgKnob::addFrameAll(std::shared_ptr<Svg> svg) {
	framesAll.push_back(svg);
	if (framesAll.size() == 1) {
		setSvg(svg);
	}
}

void ThemedSvgKnob::step() {
	if(theme != NULL && *theme != old_theme) {
		if ((*theme) == 0) {
			setSvg(framesAll[0]);
		}
		else {
			setSvg(framesAll[1]);
		}
		old_theme = *theme;
		fb->dirty = true;
	}
	SvgKnob::step();
}

// themed snap knob
void ThemedRoundBlackSnapKnob::addFrameAll(std::shared_ptr<Svg> svg) {
	framesAll.push_back(svg);
	if (framesAll.size() == 1) {
		setSvg(svg);
	}
}

void ThemedRoundBlackSnapKnob::step() {
	if(theme != NULL && *theme != old_theme) {
		if ((*theme) == 0) {
			setSvg(framesAll[0]);
		}
		else {
			setSvg(framesAll[1]);
		}
		old_theme = *theme;
		fb->dirty = true;
	}
	SvgKnob::step();
}

// themed port
void ThemedSvgPort::addFrame(std::shared_ptr<Svg> svg) {
	frames.push_back(svg);
	if(frames.size() == 1) {
		SvgPort::setSvg(svg);
	}
}

void ThemedSvgPort::step() {
	if(theme != NULL && *theme != old_theme) {
		sw->setSvg(frames[*theme]);
		old_theme = *theme;
		fb->dirty = true;
	}
	PortWidget::step();
}

// themed screw
void ThemedSvgScrew::addFrame(std::shared_ptr<Svg> svg) {
	frames.push_back(svg);
	if(frames.size() == 1) {
		SvgScrew::setSvg(svg);
	}
}

void ThemedSvgScrew::step() {
	if(theme != NULL && *theme != old_theme) {
		sw->setSvg(frames[*theme]);
		old_theme = *theme;
		fb->dirty = true;
	}
	SvgScrew::step();
}

// save a plugin default integer
void saveGtgPluginDefault(const char* plugin_setting, int setting_value) {
	json_t *settingsJ = json_object();
	std::string settingsFilename = asset::user("GlueTheGiant.json");

	FILE *file = fopen(settingsFilename.c_str(), "r");
	if (file) {
		json_error_t error;
		settingsJ = json_loadf(file, 0, &error);
	}

	json_object_set_new(settingsJ, plugin_setting, json_integer(setting_value));

	file = fopen(settingsFilename.c_str(), "w");
	if (file) {
		json_dumpf(settingsJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
	}

	fclose(file);
	json_decref(settingsJ);
}

// load a plugin default integer
int loadGtgPluginDefault(const char* plugin_setting, int default_value) {
	std::string settingsFilename = asset::user("GlueTheGiant.json");

	FILE *file = fopen(settingsFilename.c_str(), "r");
	if (!file) {   // file does not exist
		return default_value;
	}

	json_error_t error;
	json_t *settingsJ = json_loadf(file, 0, &error);
	if (!settingsJ) {   // file invalid
		fclose(file);
		return default_value;
	}

	json_t *default_valueJ = json_object_get(settingsJ, plugin_setting);
	if (default_valueJ) default_value = json_integer_value(default_valueJ);
	fclose(file);

	json_decref(settingsJ);
	return default_value;
}
