#include "Quaternion.h"
#include <pybind11/operators.h>
namespace py=pybind11;


Quaternion::Quaternion(){
    q=Eigen::Quaternion<double>(1,0,0,0);
}
Quaternion::Quaternion(const double &w,const Eigen::Vector3d &vec_){
    q=Eigen::Quaternion<double>(w,vec_[0],vec_[1],vec_[2]);
}
Quaternion::Quaternion(const double &w,const double &x,const double &y,const double &z){
    q=Eigen::Quaternion<double>(w,x,y,z);
}
Quaternion::Quaternion(const Eigen::Vector4d &v){
    q=Eigen::Quaternion<double>(v[0],v[1],v[2],v[3]);
}
Quaternion::Quaternion(const Eigen::Quaternion<double> &other){
    q=other;
}
Quaternion::Quaternion(const Quaternion &other){
    q=other.q;
}
double& Quaternion::w(){return q.w();}
const double& Quaternion::w() const{return q.w();}
void Quaternion::w(const double &w_){q.w()=w_;}
double& Quaternion::x(){return q.x();}
const double& Quaternion::x() const{return q.x();}
void Quaternion::x(const double &x_){q.x()=x_;}
double& Quaternion::y(){return q.y();}
const double& Quaternion::y() const{return q.y();}
void Quaternion::y(const double &y_){q.y()=y_;}
double& Quaternion::z(){return q.z();}
const double& Quaternion::z() const{return q.z();}
void Quaternion::z(const double &z_){q.z()=z_;}
const double& Quaternion::wGetterPY(){return q.w();}
void Quaternion::wSetterPY(const double &w_){q.w()=w_;}
const double& Quaternion::xGetterPY(){return q.x();}
void Quaternion::xSetterPY(const double &x_){q.x()=x_;}
const double& Quaternion::yGetterPY(){return q.y();}
void Quaternion::ySetterPY(const double &y_){q.y()=y_;}
const double& Quaternion::zGetterPY(){return q.z();}
void Quaternion::zSetterPY(const double &z_){q.z()=z_;}
Eigen::Map<Eigen::Vector3d> Quaternion::vec(){
    return Eigen::Map<Eigen::Vector3d>(q.vec().data());
}
void Quaternion::vec(const Eigen::Vector3d &vec_){
    q.vec()=vec_;
}
Eigen::Map<Eigen::Vector3d> Quaternion::vecGetterPY(){
    return Eigen::Map<Eigen::Vector3d>(q.vec().data());
}
void Quaternion::vecSetterPY(const Eigen::Vector3d &vec_){
    q.vec()=vec_;
}

std::string Quaternion::toString() const{
    return "Quaternion("+std::to_string(q.w())+",["+std::to_string(q.x())+","+std::to_string(q.y())+","+std::to_string(q.z())+"])";
}
Eigen::Vector4d Quaternion::toArray() const{
    return Eigen::Vector4d(q.w(),q.x(),q.y(),q.z());
}
double Quaternion::norm() const{
    return q.norm();
}
void Quaternion::normalize(){
    return q.normalize();
}
Quaternion Quaternion::normalized() const{
    return Quaternion(q.normalized());
}
Quaternion Quaternion::operator+(const Quaternion &other) const{
    return Quaternion(q.w()+other.q.w(),q.x()+other.q.x(),q.y()+other.q.y(),q.z()+other.q.z());
}
Quaternion Quaternion::operator-(const Quaternion &other) const{
    return Quaternion(q.w()-other.q.w(),q.x()-other.q.x(),q.y()-other.q.y(),q.z()-other.q.z());
}
Quaternion Quaternion::operator-() const{
    return Quaternion(-q.w(),-q.x(),-q.y(),-q.z());
}
Quaternion Quaternion::operator*(const Quaternion &other) const{
    return Quaternion(q*other.q);
}
double Quaternion::dot(const Quaternion &other) const{
    return q.dot(other.q);
}
Quaternion Quaternion::conjugate() const{
    return Quaternion(q.conjugate());
}
Eigen::Vector3d Quaternion::transformVector(const Eigen::Vector3d &v) const{
    return q._transformVector(v);
}
Eigen::Matrix3d Quaternion::toRotationMatrix() const{
    return q.toRotationMatrix();
}
Eigen::Vector3d Quaternion::toRotationVector() const{
    Eigen::AngleAxisd aa(q);
    return aa.angle()*aa.axis();
}
Quaternion Quaternion::slerp(const double &t,const Quaternion &other) const{
    return q.slerp(t,other.q);
}
Eigen::Matrix<double,3,4> Quaternion::dC1dq() const{//回転行列の1列目の自分自身による微分(3×4行列)
    Eigen::Matrix<double,3,4> ret;
    ret<<q.w(),q.x(),-q.y(),-q.z(),
        q.z(),q.y(),q.x(),q.w(),
        -q.y(),q.z(),-q.w(),q.x();
    return 2*ret;
}
Eigen::Matrix<double,3,4> Quaternion::dC2dq() const{//回転行列の2列目の自分自身による微分(3×4行列)
    Eigen::Matrix<double,3,4> ret;
    ret<<-q.z(),q.y(),q.x(),-q.w(),
        q.w(),-q.x(),q.y(),-q.z(),
        q.x(),q.w(),q.z(),q.y();
    return 2*ret;
}
Eigen::Matrix<double,3,4> Quaternion::dC3dq() const{//回転行列の3列目の自分自身による微分(3×4行列)
    Eigen::Matrix<double,3,4> ret;
    ret<<q.y(),q.z(),q.w(),q.x(),
        -q.x(),-q.w(),q.z(),q.y(),
        q.w(),-q.x(),-q.y(),q.z();
    return 2*ret;
}
Eigen::Matrix<double,3,4> Quaternion::dR1dq() const{//回転行列の1行目の自分自身による微分(3×4行列)
    Eigen::Matrix<double,3,4> ret;
    ret<<q.w(),q.x(),-q.y(),-q.z(),
        -q.z(),q.y(),q.x(),-q.w(),
        q.y(),q.z(),q.w(),q.x();
    return 2*ret;
}
Eigen::Matrix<double,3,4> Quaternion::dR2dq() const{//回転行列の2行目の自分自身による微分(3×4行列)
    Eigen::Matrix<double,3,4> ret;
    ret<<q.z(),q.y(),q.x(),q.w(),
        q.w(),-q.x(),q.y(),-q.z(),
        -q.x(),-q.w(),q.z(),q.y();
    return 2*ret;
}
Eigen::Matrix<double,3,4> Quaternion::dR3dq() const{//回転行列の3行目の自分自身による微分(3×4行列)
    Eigen::Matrix<double,3,4> ret;
    ret<<-q.y(),q.z(),-q.w(),q.x(),
        q.x(),q.w(),q.z(),q.y(),
        q.w(),-q.x(),-q.y(),q.z();
    return 2*ret;
}
Eigen::Matrix<double,4,3> Quaternion::dqdwi() const{
    double hw=0.5*q.w();
    double hx=0.5*q.x();
    double hy=0.5*q.y();
    double hz=0.5*q.z();
    Eigen::Matrix<double,4,3> ret;
    ret<<-hx,-hy,-hz,
        hw,hz,-hy,
        -hz,hw,hx,
        hy,-hx,hw;
    return ret;
}
Quaternion Quaternion::fromAngle(const Eigen::Vector3d &ax,const double &theta){
    //EigenではAngleAxisを経由して生成できるが、中身は以下の通り普通の演算。
    double ha=0.5*theta;
    Eigen::Vector3d v=sin(ha)*ax;
    return Quaternion(cos(ha),v[0],v[1],v[2]);
}
Quaternion Quaternion::fromBasis(const Eigen::Vector3d &ex,const Eigen::Vector3d &ey,const Eigen::Vector3d &ez){
    //基底が基準座標系iでの成分表示でex,ey,ezと表されるような座標系bの変換クォータニオンqを計算する。
    //回転行列Rとしては、ex,ey,ezを列ベクトルとして並べたものになる。
    //ベクトル量の成分表示間の変換としては、
    //ri=R*rb
    //ri=q*(rb,0)*q~
    //となる。
    //Eigenでは、Quaternionのコンストラクタに3×3行列を入れると得られる。
    Eigen::Matrix3d R;
    R.block(0,0,3,1)=ex;R.block(0,1,3,1)=ey;R.block(0,2,3,1)=ez;
    return Quaternion(Eigen::Quaterniond(R).normalized());
}
void to_json(nl::json& j,const Quaternion& q){
    j=q.toArray();
}
void from_json(const nl::json& j,Quaternion& q){
    if(j.is_array()){
        assert(j.size()==4);
        q.w(j[0].get<double>());
        q.x(j[1].get<double>());
        q.y(j[2].get<double>());
        q.z(j[3].get<double>());
    }else if(j.is_object()){
        q.w(j.at("w").get<double>());
        q.vec(j.at("vec").get<Eigen::Vector3d>());
    }
}
nl::json Quaternion::to_json() const{
    return *this;
}
void exportQuaternion(py::module &m)
{
    using namespace pybind11::literals;
    py::class_<Quaternion>(m,"Quaternion")
        .def(py::init<const double&,const double&,const double&,const double&>())
        .def(py::init<const double&,const Eigen::Vector3d &>())
        .def(py::init<const Eigen::Vector4d &>())
        .def_property("w",&Quaternion::wGetterPY,&Quaternion::wSetterPY)
        .def_property("x",&Quaternion::xGetterPY,&Quaternion::xSetterPY)
        .def_property("y",&Quaternion::yGetterPY,&Quaternion::ySetterPY)
        .def_property("z",&Quaternion::zGetterPY,&Quaternion::zSetterPY)
        .def_property("vec",&Quaternion::vecGetterPY,&Quaternion::vecSetterPY)
        .def("__repr__",&Quaternion::toString)
        DEF_FUNC(Quaternion,toArray)
        DEF_FUNC(Quaternion,norm)
        DEF_FUNC(Quaternion,normalize)
        DEF_FUNC(Quaternion,normalized)
        .def(py::self+py::self)
        .def(py::self-py::self)
        .def(-py::self)
        .def(py::self*py::self)
        DEF_FUNC(Quaternion,dot)
        DEF_FUNC(Quaternion,conjugate)
        DEF_FUNC(Quaternion,transformVector)
        DEF_FUNC(Quaternion,toRotationMatrix)
        DEF_FUNC(Quaternion,toRotationVector)
        DEF_FUNC(Quaternion,slerp)
        DEF_FUNC(Quaternion,dC1dq)
        DEF_FUNC(Quaternion,dC2dq)
        DEF_FUNC(Quaternion,dC3dq)
        DEF_FUNC(Quaternion,dR1dq)
        DEF_FUNC(Quaternion,dR2dq)
        DEF_FUNC(Quaternion,dR3dq)
        DEF_FUNC(Quaternion,dC1dq)
        DEF_FUNC(Quaternion,dC2dq)
        DEF_FUNC(Quaternion,dqdwi)
        DEF_FUNC(Quaternion,fromAngle)
        DEF_FUNC(Quaternion,fromBasis)
        DEF_FUNC(Quaternion,to_json)
        .def(py::pickle(
            [](Quaternion &q){//__getstate__
                return py::make_tuple(q.w(),q.vec());
            },
            [](py::tuple t){//__setstate__
                return Quaternion(t[0].cast<double>(),t[1].cast<Eigen::Vector3d>());
            }
        ))
    ;
}
