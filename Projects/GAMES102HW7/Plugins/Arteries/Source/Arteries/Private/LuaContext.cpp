// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "LuaContext.h"
#include "Arteries.h"
#include "ArteriesActor.h"

static void* LuaAlloc(void *Ud, void *Ptr, size_t OldSize, size_t NewSize)
{
	if (NewSize != 0)
		return FMemory::Realloc(Ptr, NewSize);
	FMemory::Free(Ptr);
	return NULL;
}
static int32 LuaPanic(lua_State *lua_State)
{
	UE_LOG(LogArteries, Error, TEXT("PANIC: unprotected error in call to Lua API(%s)"), ANSI_TO_TCHAR(lua_tostring(lua_State, -1)));
	return 0;
}
static int Print(lua_State* L)
{
	int nargs = lua_gettop(L);
	for (int i = 1; i <= nargs; i++)
	{
		if (lua_isstring(L, i))
		{
			UE_LOG(LogArteries, Log, TEXT("%s"), ANSI_TO_TCHAR(lua_tostring(L, i)));
			/* Pop the next arg using  and do your print */
		}
		else
		{
			/* Do something with non-strings if you like */
		}
	}
	return 0;
}
template<class ValueClass>
static ValueClass GetVariable(const char* Key)
{
	ValueClass Value;
	AArteriesActor* Object = FArteriesRunnable::GetThreadOwner();
	if (UProperty* Prop = FindField<UProperty>(Object->GetClass(), Key))
		FMemory::Memcpy(&Value, (uint8*)Object + Prop->GetOffset_ForInternal(), sizeof(ValueClass));
	else
		FMemory::Memzero(&Value, sizeof(ValueClass));
	return Value;
}
ULuaContext::ULuaContext(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), LuaState(NULL)
{
}
void ULuaContext::Init()
{
	Close();
	LuaState = lua_newstate(LuaAlloc, NULL);
	lua_atpanic(LuaState, &LuaPanic);
	luaL_openlibs(LuaState);
	luabridge::getGlobalNamespace(LuaState)
		.addFunction("GetInt", &GetVariable<int>)
		.addFunction("GetFloat", &GetVariable<float>)
		.addFunction("GetVec2", &GetVariable<FVector2D>)
		.addFunction("GetVec3", &GetVariable<FVector>)
		.addFunction("print", &Print)
		.beginClass<FMath>("Math")
		.addStaticFunction("IsNearlyEqual", static_cast<bool(*)(float, float, float)>(&FMath::IsNearlyEqual))
		.endClass()
		.beginClass<FVector2D>("Vec2")
		.addData("X", &FVector2D::X)
		.addData("Y", &FVector2D::Y)
		.endClass()
		.beginClass<FVector>("Vec3")
		.addStaticData("Forward", (FVector*)&FVector::ForwardVector, false)
		.addStaticData("Right", (FVector*)&FVector::RightVector, false)
		.addStaticData("Up", (FVector*)&FVector::UpVector, false)
		.addFunction("__add", static_cast<FVector(FVector::*)(const FVector&)const>(&FVector::operator+))
		.addFunction("__sub", static_cast<FVector(FVector::*)(const FVector&)const>(&FVector::operator-))
		.addFunction("__unm", static_cast<FVector(FVector::*)()const>(&FVector::operator-))
		.addFunction("__mul", static_cast<FVector(FVector::*)(float)const>(&FVector::operator*))
		.addFunction("__div", static_cast<FVector(FVector::*)(float)const>(&FVector::operator/))
		.addFunction("__bor", &FVector::operator|)
		.addFunction("__pow", &FVector::operator^)
		.addFunction("GetNormal", &FVector::GetUnsafeNormal)
		.addData("X", &FVector::X)
		.addData("Y", &FVector::Y)
		.addData("Z", &FVector::Z)
		.endClass()
		.beginClass<FArteriesElement>("Element")
			.addFunction("HasInt", &FArteriesElement::HasIntANSI)
			.addFunction("GetInt", &FArteriesElement::GetIntANSI)
			.addFunction("SetInt", &FArteriesElement::SetIntANSI)
			.addFunction("HasFloat", &FArteriesElement::HasFloatANSI)
			.addFunction("GetFloat", &FArteriesElement::GetFloatANSI)
			.addFunction("SetFloat", &FArteriesElement::SetFloatANSI)
			.addFunction("HasVec3", &FArteriesElement::HasVec3ANSI)
			.addFunction("GetVec3", &FArteriesElement::GetVec3ANSI)
			.addFunction("SetVec3", &FArteriesElement::SetVec3ANSI)
		.endClass()
		.deriveClass<FArteriesPoint, FArteriesElement>("Point")
			.addData("Position", &FArteriesPoint::Position)
			.addFunction("GetTargets", &FArteriesPoint::GetTargets)
			.addFunction("GetPrimitives", &FArteriesPoint::GetPrimitives)
		.endClass()
		.deriveClass<FArteriesPrimitive, FArteriesElement>("Primitive")
			.addFunction("Insert", static_cast<bool(FArteriesPrimitive::*)(FArteriesPoint*, int)>(&FArteriesPrimitive::Insert))
			.addFunction("Add", &FArteriesPrimitive::Add)
			.addFunction("NumPoints", &FArteriesPrimitive::NumPoints)
			.addFunction("GetPoint", &FArteriesPrimitive::GetPoint)
			.addFunction("Delete", &FArteriesPrimitive::Delete)
		.endClass()
		.beginClass<UArteriesObject>("Object")
			.addFunction("AddPoint", static_cast<FArteriesPoint*(UArteriesObject::*)(const FVector&)>(&UArteriesObject::AddPoint))
			.addFunction("AddPrimitive", static_cast<FArteriesPrimitive*(UArteriesObject::*)()>(&UArteriesObject::AddPrimitive))
			.addFunction("NumPoints", &UArteriesObject::NumPoints)
			.addFunction("NumPrimitives", &UArteriesObject::NumPrimitives)
			.addFunction("GetPoint", &UArteriesObject::GetPoint)
			.addFunction("GetPrimitive", &UArteriesObject::GetPrimitive)
			.addFunction("DeletePoint", static_cast<void(UArteriesObject::*)(FArteriesPoint*)>(&UArteriesObject::DeletePoint))
			.addFunction("DeletePrimitive", static_cast<void(UArteriesObject::*)(FArteriesPrimitive*)>(&UArteriesObject::DeletePrimitive))
			.addFunction("CleanPoints", &UArteriesObject::CleanPoints)
			.addFunction("CleanPrimitives", &UArteriesObject::CleanPrimitives)
			.addFunction("SetPointGroup", &UArteriesObject::SetPointGroupANSI)
			.addFunction("SetPrimitiveGroup", &UArteriesObject::SetPrimitiveGroupANSI)
		.endClass()
		//"__index" will corrupt other function mapping???
		.beginClass<TArray<FArteriesPoint*>>("PointArray")
			.addFunction("Num", &TArray<FArteriesPoint*>::Num)
			.addFunction("Get", static_cast<FArteriesPoint*&(TArray<FArteriesPoint*>::*)(int)>(&TArray<FArteriesPoint*>::operator[]))
		//	.addFunction("__index", static_cast<FArteriesPoint*&(TArray<FArteriesPoint*>::*)(int)>(&TArray<FArteriesPoint*>::operator[]))
		.endClass()
		.beginClass<TArray<FArteriesPrimitive*>>("PrimitiveArray")
			.addFunction("Num", &TArray<FArteriesPrimitive*>::Num)
			.addFunction("Get", static_cast<FArteriesPrimitive*&(TArray<FArteriesPrimitive*>::*)(int)>(&TArray<FArteriesPrimitive*>::operator[]))
		//	.addFunction("__index", static_cast<FArteriesPrimitive*&(TArray<FArteriesPrimitive*>::*)(int)>(&TArray<FArteriesPrimitive*>::operator[]))
		.endClass()
	;
}
void ULuaContext::Close()
{
	if (LuaState)
	{
		lua_close(LuaState);
		LuaState = NULL;
	}
}
void ULuaContext::Begin()
{
	lua_newtable(LuaState);
	lua_setglobal(LuaState, "pointertouserdata");
}
void ULuaContext::Execute(const char* Code)
{
	int result = luaL_dostring(LuaState, Code);
	if (result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Lua Script error: %s"), ANSI_TO_TCHAR(lua_tostring(LuaState, -1)));
	}
}
void ULuaContext::End()
{
}
void ULuaContext::Push(FArteriesPoint* Point, const char* Name)
{
	luabridge::setGlobal(LuaState, Point, Name);
}
void ULuaContext::Push(FArteriesPrimitive* Primitive, const char* Name)
{
	luabridge::setGlobal(LuaState, Primitive, Name);
}
void ULuaContext::Push(UArteriesObject* Obj, const char* Name)
{
	luabridge::setGlobal(LuaState, Obj, Name);
}
void ULuaContext::BeginDestroy()
{
	Close();
	UObject::BeginDestroy();
}
void ULuaContext::PostLoad()
{
	Init();
	UObject::PostLoad();
}