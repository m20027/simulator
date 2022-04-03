#define MY_MODULE_NAME libCore
#include "Common.h"
#include <cmath>
#include <iostream>
#include <pybind11/pybind11.h>
#include "Units.h"
#include "Utility.h"
#include "JsonMutableFromPython.h"
#include "ConfigDispatcher.h"
#include "Quaternion.h"
#include "MathUtility.h"
#include "CommunicationBuffer.h"
#include "Entity.h"
#include "Asset.h"
#include "MotionState.h"
#include "PhysicalAsset.h"
#include "Controller.h"
#include "Track.h"
#include "Fighter.h"
#include "MassPointFighter.h"
#include "CoordinatedFighter.h"
#include "Missile.h"
#include "Sensor.h"
#include "Agent.h"
#include "R3InitialFighterAgent01.h"
#include "Callback.h"
#include "Ruler.h"
#include "R3BVRRuler01.h"
#include "Reward.h"
#include "R3BVRBasicReward01.h"
#include "Viewer.h"
#include "SimulationManager.h"
#include "Factory.h"
namespace py=pybind11;

PYBIND11_MODULE(MY_MODULE_NAME,m)
{    
    using namespace pybind11::literals;
    m.doc()="ASRCAISim1";
    exportCommon(m);
    exportUnits(m);
    exportUtility(m);
    exportJsonMutableFromPython(m);
    exportQuaternion(m);
    exportMathUtility(m);
    exportCommunicationBuffer(m);
    exportEntity(m);
    exportAsset(m);
    exportMotionState(m);
    exportPhysicalAsset(m);
    exportController(m);
    exportTrack(m);
    exportFighter(m);
    exportMassPointFighter(m);
    exportCoordinatedFighter(m);
    exportMissile(m);
    exportSensor(m);
    exportAgent(m);
    exportR3InitialFighterAgent01(m);
    exportCallback(m);
    exportRuler(m);
    exportReward(m);
    exportR3BVRRuler01(m);
    exportR3BVRBasicReward01(m);
    exportViewer(m);
    exportSimulationManager(m);
    exportFactory(m);
    setupBuiltIns();
}
