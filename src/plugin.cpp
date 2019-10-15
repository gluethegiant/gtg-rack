#include "plugin.hpp"


Plugin *pluginInstance;


void init(Plugin *p) {
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);
	p->addModel(modelMiniBus);
	p->addModel(modelGigBus);
	p->addModel(modelSchoolBus);
	p->addModel(modelMetroCityBus);
	p->addModel(modelBusRoute);
	p->addModel(modelRoad);
	p->addModel(modelEnterBus);
	p->addModel(modelExitBus);
	p->addModel(modelBusDepot);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
