#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin *pluginInstance;

// Declare each Model, defined in each module source file
// extern Model *modelMyModule;
extern Model *modelMiniBus;
extern Model *modelGigBus;
extern Model *modelSchoolBus;
extern Model *modelMetroCityBus;
extern Model *modelBusDepot;
extern Model *modelBusRoute;
extern Model *modelRoad;
extern Model *modelEnterBus;
extern Model *modelExitBus;
