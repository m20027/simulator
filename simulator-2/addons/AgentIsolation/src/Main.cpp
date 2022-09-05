#define MY_MODULE_NAME libAgentIsolation
#include "AgentIsolation.h"
#include "AgentDelegatorBase.h"
namespace py=pybind11;

PYBIND11_MODULE(MY_MODULE_NAME,m)
{    
    using namespace pybind11::literals;
    m.doc()="AgentIsolation";
    exportAgentIsolationTools(m);
    exportAgentDelegatorBase(m);
}
