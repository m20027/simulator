#include "Units.h"
#include <pybind11/numpy.h>
namespace util{
    double ft2m(const double &bef){
        return bef*0.3048;
    }
    double m2ft(const double &bef){
        return bef/0.3048;
    }
    double deg2rad(const double &bef){
        return bef*(M_PI/180.0);
    }
    double rad2deg(const double &bef){
        return bef*(180.0/M_PI);
    }
    double N2kgf(const double &bef){
        return bef/gravity;
    }
    double kgf2N(const double &bef){
        return bef*gravity;
    }
    double lb2kg(const double &bef){
        return bef*0.45359237;
    }
    double kg2lb(const double &bef){
        return bef/0.45359237;
    }
    double N2lbf(const double &bef){
        return bef/gravity/0.45359237;
    }
    double lbf2N(const double &bef){
        return bef*gravity*0.45359237;
    }
    double psi2Pa(const double &bef){
        return bef*0.45359237/0.0254/0.0254;
    }
    double Pa2psi(const double &bef){
        return bef/0.45359237*0.0254*0.0254;
    }
    double slug2N(const double &bef){
        return bef*0.45359237*gravity/0.3048;
    }
    double N2slug(const double &bef){
        return bef/(0.45359237*gravity/0.3048);
    }
    double slugft22kgm2(const double &bef){
        return bef*0.45359237*gravity*0.3048;
    }
    std::vector<double> atmosphere(const double &h){
        std::vector<double> ret(6);
        double gamma=1.4;
        double R=287.0531;
	    double HAL[]={0.,11000.,20000.0,32000.0,47000.0,51000.0,71000.0,80000.0};
	    double LR[]={-0.0065,0.0,0.001,0.0028,0.0,-0.0028,-0.002,0.0};
    	double T0[]={288.15,216.65,216.65,228.65,270.65,270.65,214.65,196.65};
	    double P0[]={101325.0,22632.0,5474.9,868.02,110.91,66.939,3.9564,0.88627};
        int k=7;
        for(int i=1;i<=7;++i){
            if(h<HAL[i]){
                k=i-1;
                break;
            }
        }
        double T=T0[k]+LR[k]*(h-HAL[k]);
        double a=sqrt(T*gamma*R);
        double P,rho,dRhodH,dPdH;
        if(LR[k]!=0.0){
            P=P0[k]*pow(T/T0[k],gravity/(-LR[k]*R));
            dPdH=P0[k]*(-gravity/LR[k]/R)*pow(T/T0[k],-gravity/LR[k]/R-1.);
        }else{
            P=P0[k]*exp(gravity*(HAL[k]-h)/(R*T0[k]));
            dPdH=P*(-gravity/R/T);
        }
        rho=P/(R*T);
        dRhodH=(dPdH/T-LR[k]*P/(T*T))/R;
		double bs=1.458e-6;//[kg/m/s/√k]
		double S=110.4;//[K]
		double nu=bs*pow(T,1.5)/(T+S);
        ret[0]=T;
        ret[1]=a;
        ret[2]=P;
        ret[3]=rho;
        ret[4]=dRhodH;
        ret[5]=nu;
        return ret;
    }
    double horizon(const double &h){
        return Req/(Req+h)*sqrt((Req+h)*(Req+h)-Req*Req);
    }
};


void exportUnits(py::module &m)
{
    m.attr("Req")=util::Req;
    m.attr("gravity")=util::gravity;
    m.def("ft2m",py::vectorize(util::ft2m));
    m.def("m2ft",py::vectorize(util::m2ft));
    m.def("deg2rad",py::vectorize(util::deg2rad));
    m.def("rad2deg",py::vectorize(util::rad2deg));
    m.def("N2kgf",py::vectorize(util::N2kgf));
    m.def("kgf2N",py::vectorize(util::kgf2N));
    m.def("lb2kg",py::vectorize(util::lb2kg));
    m.def("kg2lb",py::vectorize(util::kg2lb));
    m.def("N2lbf",py::vectorize(util::N2lbf));
    m.def("lbf2N",py::vectorize(util::lbf2N));
    m.def("psi2Pa",py::vectorize(util::psi2Pa));
    m.def("Pa2psi",py::vectorize(util::Pa2psi));
    m.def("slug2N",py::vectorize(util::slug2N));
    m.def("N2slug",py::vectorize(util::N2slug));
    m.def("slugft22kgm2",py::vectorize(util::slugft22kgm2));
    m.def("atmosphere",&util::atmosphere);
    m.def("horizon",py::vectorize(util::horizon));
}
