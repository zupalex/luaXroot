#include "LuaRootClasses.h"

double LuaTVector3::X()
{
	return ((TVector3*) rootObj)->X();
}

double LuaTVector3::Y()
{
	return ((TVector3*) rootObj)->Y();
}

double LuaTVector3::Z()
{
	return ((TVector3*) rootObj)->Z();
}

double LuaTVector3::Px()
{
	return ((TVector3*) rootObj)->Px();
}

double LuaTVector3::Py()
{
	return ((TVector3*) rootObj)->Py();
}

double LuaTVector3::Pz()
{
	return ((TVector3*) rootObj)->Pz();
}

double LuaTVector3::Theta()
{
	return ((TVector3*) rootObj)->Theta();
}

double LuaTVector3::Phi()
{
	return ((TVector3*) rootObj)->Phi();
}

double LuaTVector3::Perp()
{
	return ((TVector3*) rootObj)->Perp();
}

double LuaTVector3::Perp2()
{
	return ((TVector3*) rootObj)->Perp2();
}

double LuaTVector3::Mag()
{
	return ((TVector3*) rootObj)->Mag();
}

double LuaTVector3::Mag2()
{
	return ((TVector3*) rootObj)->Mag2();
}

double LuaTVector3::CosTheta()
{
	return ((TVector3*) rootObj)->CosTheta();
}

double LuaTVector3::Eta()
{
	return ((TVector3*) rootObj)->Eta();
}

void LuaTVector3::RotateX(double angle)
{
	return ((TVector3*) rootObj)->RotateX(angle);
}

void LuaTVector3::RotateY(double angle)
{
	return ((TVector3*) rootObj)->RotateY(angle);
}

void LuaTVector3::RotateZ(double angle)
{
	return ((TVector3*) rootObj)->RotateZ(angle);
}

void LuaTVector3::Rotate(double angle, LuaTVector3* axis)
{
	((TVector3*) rootObj)->Rotate(angle, *((TVector3*) axis->rootObj));
}

double LuaTVector3::Angle(LuaTVector3* vec)
{
	return ((TVector3*) rootObj)->Angle(*((TVector3*) vec->rootObj));
}

double LuaTVector3::Dot(LuaTVector3* vec)
{
	return ((TVector3*) rootObj)->Dot(*((TVector3*) vec->rootObj));
}

LuaTVector3 LuaTVector3::Cross(LuaTVector3* vec)
{
	LuaTVector3 cross_vec;

	*((TVector3*) cross_vec.rootObj) = ((TVector3*) rootObj)->Cross(*((TVector3*) vec->rootObj));

	return cross_vec;
}

void LuaTVector3::SetMag(double mag)
{
	((TVector3*) rootObj)->SetMag(mag);
}

void LuaTVector3::SetMagThetaPhi(double mag, double theta, double phi)
{
	((TVector3*) rootObj)->SetMagThetaPhi(mag, theta, phi);
}

void LuaTVector3::SetPerp(double perp)
{
	((TVector3*) rootObj)->SetPerp(perp);
}

void LuaTVector3::SetPhi(double phi)
{
	((TVector3*) rootObj)->SetPhi(phi);
}

void LuaTVector3::SetPtEtaPhi(double pt, double eta, double phi)
{
	((TVector3*) rootObj)->SetPtEtaPhi(pt, eta, phi);
}

void LuaTVector3::SetPtThetaPhi(double pt, double theta, double phi)
{
	((TVector3*) rootObj)->SetPtThetaPhi(pt, theta, phi);
}

void LuaTVector3::SetTheta(double theta)
{
	((TVector3*) rootObj)->SetTheta(theta);
}

void LuaTVector3::SetX(double x)
{
	((TVector3*) rootObj)->SetX(x);
}

void LuaTVector3::SetXYZ(double x, double y, double z)
{
	((TVector3*) rootObj)->SetXYZ(x, y, z);
}

void LuaTVector3::SetY(double y)
{
	((TVector3*) rootObj)->SetY(y);
}

void LuaTVector3::SetZ(double z)
{
	((TVector3*) rootObj)->SetZ(z);
}

LuaTVector3 LuaTVector3::Unit()
{
	LuaTVector3 unit_;

	*((TVector3*) unit_.rootObj) = ((TVector3*) rootObj)->Unit();

	return unit_;
}

void LuaTVector3::MakeAccessors(lua_State* L)
{
	AddClassMethod(L, &LuaTVector3::X, "X");
	AddClassMethod(L, &LuaTVector3::Y, "Y");
	AddClassMethod(L, &LuaTVector3::Z, "Z");

	AddClassMethod(L, &LuaTVector3::Px, "Px");
	AddClassMethod(L, &LuaTVector3::Py, "Py");
	AddClassMethod(L, &LuaTVector3::Pz, "Pz");

	AddClassMethod(L, &LuaTVector3::Theta, "Theta");
	AddClassMethod(L, &LuaTVector3::Phi, "Phi");

	AddClassMethod(L, &LuaTVector3::Perp, "Perp");
	AddClassMethod(L, &LuaTVector3::Perp2, "Perp2");

	AddClassMethod(L, &LuaTVector3::Mag, "Mag");
	AddClassMethod(L, &LuaTVector3::Mag2, "Mag2");

	AddClassMethod(L, &LuaTVector3::CosTheta, "CosTheta");
	AddClassMethod(L, &LuaTVector3::Eta, "Eta");

	AddClassMethod(L, &LuaTVector3::RotateX, "RotateX");
	AddClassMethod(L, &LuaTVector3::RotateY, "RotateY");
	AddClassMethod(L, &LuaTVector3::RotateZ, "RotateZ");

	AddClassMethod(L, &LuaTVector3::Rotate, "Rotate");

	AddClassMethod(L, &LuaTVector3::Angle, "Angle");
	AddClassMethod(L, &LuaTVector3::Dot, "Dot");
	AddClassMethod(L, &LuaTVector3::Cross, "Cross");

	AddClassMethod(L, &LuaTVector3::SetMag, "SetMag");
	AddClassMethod(L, &LuaTVector3::SetMagThetaPhi, "SetMagThetaPhi");

	AddClassMethod(L, &LuaTVector3::SetPerp, "SetPerp");

	AddClassMethod(L, &LuaTVector3::SetPhi, "SetPhi");

	AddClassMethod(L, &LuaTVector3::SetPtEtaPhi, "SetPtEtaPhi");

	AddClassMethod(L, &LuaTVector3::SetPtThetaPhi, "SetPtThetaPhi");

	AddClassMethod(L, &LuaTVector3::SetTheta, "SetTheta");

	AddClassMethod(L, &LuaTVector3::SetX, "SetX");

	AddClassMethod(L, &LuaTVector3::SetXYZ, "SetXYZ");

	AddClassMethod(L, &LuaTVector3::SetY, "SetY");

	AddClassMethod(L, &LuaTVector3::SetZ, "SetZ");

	AddClassMethod(L, &LuaTVector3::Unit, "Unit");;

	AddClassMethod(L, &LuaTVector3::DoDraw, "Draw");
	AddClassMethod(L, &LuaTVector3::DoUpdate, "Update");
	AddClassMethod(L, &LuaTVector3::DoWrite, "Write");
}

void LuaTVector3::AddNonClassMethods(lua_State* L)
{
	AddMethod(L, LuaTObjectSetName<LuaTVector3>, "SetName");
	AddMethod(L, LuaTObjectGetName<LuaTVector3>, "GetName");

	AddMethod(L, LuaTClone<LuaTVector3>, "Clone");
}

extern "C" void LoadLuaTVector3Lib(lua_State* L)
{
	MakeAccessFunctions<LuaTVector3>(L, "TVector3");
	rootObjectAliases["TVector3"] = "TVector3";

	AddObjectConstructor<LuaTVector3, double, double, double>(L, "TVector3");
}
