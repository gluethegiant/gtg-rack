#include "plugin.hpp"


Plugin *pluginInstance;

// variables used by all modules
bool audition_mixer = false;
bool audition_depot = false;

void init(Plugin *p) {
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);
	p->addModel(modelGigBus);
	p->addModel(modelMiniBus);
	p->addModel(modelSchoolBus);
	p->addModel(modelMetroCityBus);
	p->addModel(modelBusDepot);
	p->addModel(modelBusRoute);
	p->addModel(modelRoad);
	p->addModel(modelEnterBus);
	p->addModel(modelExitBus);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
