// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
extern "C"
{
#define LUA_COMPAT_APIINTCASTS 1
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#pragma warning(disable: 4100)
#pragma push_macro("Nil")
#undef Nil
#include "LuaBridge.h"
#pragma pop_macro("Nil")
#include "CoreMinimal.h"
#include "ArteriesObject.h"
#include "LuaContext.generated.h"

UCLASS()
class ARTERIES_API ULuaContext :public UObject
{
	GENERATED_UCLASS_BODY()
public:
	void Init();
	void Close();
	void Begin();
	void Execute(const char* Code);
	void End();
	void Push(FArteriesPoint* Point, const char* Name);
	void Push(FArteriesPrimitive* Primitive, const char* Name);
	void Push(UArteriesObject* Obj, const char* Name);
	virtual void BeginDestroy();
	virtual void PostLoad();
	lua_State* LuaState;
};