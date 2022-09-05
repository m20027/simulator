#include "MathUtility.h"
#include <pybind11/operators.h>
namespace py=pybind11;

double sinc(const double &x,const double &eps){
    if(abs(x)>eps){
        return sin(x)/x;
    }else{
        double x2=x*x;
        return 1.-x2/6.+x2*x2/120.-x2*x2*x2/5040.;//+O(x8)
    }
}
double sincMinusOne(const double &x,const double &eps){
    if(abs(x)>eps){
        return sin(x)/x-1.;
    }else{
        double x2=x*x;
        return -x2/6.+x2*x2/120.-x2*x2*x2/5040.;//+O(x8)
    }
}
double sincMinusOne_x2(const double &x,const double &eps){
    if(abs(x)>eps){
        return (sin(x)/x-1.)/(x*x);
    }else{
        double x2=x*x;
        return -1./6.+x2/120.-x2*x2/5040.+x2*x2*x2/362880.;//+O(x8)
    }
}
double oneMinusCos(const double &x,const double &eps){
    if(abs(x)>eps){
        return 1-cos(x);
    }else{
        double x2=x*x;
        return x2/2.-x2*x2/24.+x2*x2*x2/720.;//+O(x8)
    }
}
double oneMinusCos_x2(const double &x,const double &eps){
    if(abs(x)>eps){
        return (1-cos(x))/(x*x);
    }else{
        double x2=x*x;
        return 1./2.-x2/24.+x2*x2/720.-x2*x2*x2/40320.;//+O(x8)
    }
}
Eigen::Vector3d getOrthogonalVector(const Eigen::Vector3d &v){
    double n=v.norm();
    if(n==0){
        return Eigen::Vector3d(1,0,0);
    }
    int idx;
    v.maxCoeff(&idx);
    int tmp=(idx+1)%3;
    return v.cross(Eigen::Vector3d(0==tmp,1==tmp,2==tmp)).normalized();
}
void exportMathUtility(py::module &m)
{
    using namespace pybind11::literals;
    m.def("sinc",&sinc,"x"_a,"eps"_a=1e-3);
    m.def("sincMinusOne",&sincMinusOne,"x"_a,"eps"_a=1e-3);
    m.def("sincMinusOne_x2",&sincMinusOne_x2,"x"_a,"eps"_a=1e-3);
    m.def("oneMinusCos",&oneMinusCos,"x"_a,"eps"_a=1e-3);
    m.def("oneMinusCos_x2",&oneMinusCos_x2,"x"_a,"eps"_a=1e-3);
    m.def("getOrthogonalVector",&getOrthogonalVector);
}
