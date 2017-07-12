#include "..\..\source\fun3d\ScriptUtils.h"
#include "..\Shared\ScriptUtilsExtention.h"
#include "LuaMission.h"
#include <math.h>
#include <string.h>
#include <iostream>
//#include <shlobj.h>

LuaMission::LuaMission(void)
{
	EnableHighTPS(m_GameTPS);
	AllowRandomTracks(true);
	b_count = &b_last - &b_first - 1;
	b_array = &b_first + 1;

	/*
	f_count = &f_last - &f_first - 1;
	f_array = &f_first + 1;

	h_count = &h_last - &h_first - 1;
	h_array = &h_first + 1;
	*/

	i_count = &i_last - &i_first - 1;
	i_array = &i_first + 1;

	// Zero things out first off. If you don't assign a default value, then it will be assigned whatever value was in that memory it is assigned to use. Essentially it would be filled with random values. We don't want that. This zero's out everything under each array at the very beginning.
	if(i_array)
		memset(i_array, 0, i_count*sizeof(int));
//	if(f_array)
//		memset(f_array, 0, f_count*sizeof(float));
//	if(h_array)
//		memset(h_array, 0, h_count*sizeof(Handle));
	if(b_array)
		memset(b_array, 0, b_count*sizeof(bool));

	L = NULL;
}

LuaMission::~LuaMission()
{
	CloseOpenFiles();

	if (L)
		lua_close(L);
}


// Lua Specific things...

extern "C" static void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushstring(L, l->name);
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_settable(L, -(nup + 3));
  }
  lua_pop(L, nup);  /* remove upvalues */
}

extern "C" void *luaL_testudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      luaL_getmetatable(L, tname);  /* get correct metatable */
      if (!lua_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      lua_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}

extern "C" static int LuaPrint(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	int i;
	char buffer[4096];
	int len = 0;
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		size_t l;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tolstring(L, -1, &l);  /* get result */
		if (s == NULL)
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		if (i>1) buffer[len++] = '\t';
		strcpy_s(buffer + len, sizeof(buffer) - len, s);
		len += strlen(buffer + len);
		lua_pop(L, 1);  /* pop result */
	}
	PrintConsoleMessage(buffer);
	return 0;
}

#if 0
// output to the debug console
extern "C" static int LuaPrint(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	int i;
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		size_t l;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tolstring(L, -1, &l);  /* get result */
		if (s == NULL)
			return luaL_error(L,
			LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		if (i>1) OutputDebugString("\t");
		PrintConsoleMessage(s); //OutputDebugString(s);
		lua_pop(L, 1);  /* pop result */
	}
	//OutputDebugString("\n");
	return 0;
}
#endif

// panic handler
extern "C" static int LuaPanic(lua_State *L)
{
	const char * error = lua_tostring(L, -1);
//	OutputDebugString(error);
//	OutputDebugString("\n");
	//FormatLogMessage("Lua panic: %s", error);
	FormatConsoleMessage("Lua panic: %s", error);
	FormatToMessagesBox("Lua panic: %s", RED, error);
	return 0;
}

static bool LuaCheckStatus(int status, lua_State *L, const char *format)
{
	if (status)
	{
		// error?
		const char *error = lua_tostring(L, -1);
//		OutputDebugString(error);
//		OutputDebugString("\n");
		//FormatLogMessage(const_cast<char *>(format), error);
		FormatConsoleMessage(const_cast<char *>(format), error);
		FormatToMessagesBox(const_cast<char *>(format), RED, error);
		return false;
	}
	return true;
}

// push a handle onto the lua stack
static void PushHandle(lua_State *L, Handle h)
{
	if (h)
		lua_pushlightuserdata(L, (void *)(h));
	else
		lua_pushnil(L);
}

// Handle GetHandle(Name n)
//DLLEXPORT Handle DLLAPI GetHandle(int seqNo);
static int GetHandle(lua_State *L)
{
	if (lua_isnumber(L, 1))
	{
		int seqNo = lua_tointeger(L, 1);
		PushHandle(L, GetHandle(seqNo)); // ScriptUtils
	}
	else
	{
		Name name = Name(luaL_checkstring(L, 1));
		PushHandle(L, GetHandle(name)); // ScriptUtils
	}
	return 1;
}

// get a handle from the lua stack
static Handle GetHandle(lua_State *L, int n)
{
	return Handle(luaL_testudata(L, n, "Handle"));
}

// Require a handle, or warn the player there's not one.
static Handle RequireHandle(lua_State *L, int n)
{
	if (lua_isnil(L, n))
		return NULL;

	return Handle(luaL_checkudata(L, n, "Handle"));
}

// Handle to string
static int Handle_ToString(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	char buf[MAX_ODF_LENGTH];
	sprintf_s(buf, "%08x", h);
	lua_pushstring(L, buf);
	return 1;
}


// get a vector from the lua stack
// returns NULL if the item is not a vector
static Vector *GetVector(lua_State *L, int n)
{
	return static_cast<Vector *>(luaL_testudata(L, n, "Vector"));
}

// get a required vector from the lua stack
static Vector *RequireVector(lua_State *L, int n)
{
	return static_cast<Vector *>(luaL_checkudata(L, n, "Vector"));
}

// create a vector on the lua stack
static Vector *NewVector(lua_State *L)
{
	Vector *v = static_cast<Vector *>(lua_newuserdata(L, sizeof(Vector)));
	luaL_getmetatable(L, "Vector");
	lua_setmetatable(L, -2);
	return v;
}

// get a matrix from the lua stack
// returns NULL if the item is not a matrix
static Matrix *GetMatrix(lua_State *L, int n)
{
	return static_cast<Matrix *>(luaL_testudata(L, n, "Matrix"));
}

// get a required Matrix from the lua stack
static Matrix *RequireMatrix(lua_State *L, int n)
{
	return static_cast<Matrix *>(luaL_checkudata(L, n, "Matrix"));
}

// create a matrix on the lua stack
static Matrix *NewMatrix(lua_State *L)
{
	Matrix *m = static_cast<Matrix *>(lua_newuserdata(L, sizeof(Matrix)));
	luaL_getmetatable(L, "Matrix");
	lua_setmetatable(L, -2);
	return m;
}

// Optional Boolean.
static bool luaL_optboolean(lua_State *L, int n, int defval) 
{ 
	return luaL_opt(L, lua_toboolean, n, defval); 
} 

// vector index (read)
// receives (userdata, key)
static int Vector_Index(lua_State *L)
{
	Vector *v = GetVector(L, 1);
	if (!v)
		return 0;
	const char *key = luaL_checkstring(L, 2);
	switch (Hash(key))
	{
	case 0xfd0c5087 /* "x" */:
		lua_pushnumber(L, v->x);
		return 1;
	case 0xfc0c4ef4 /* "y" */:
		lua_pushnumber(L, v->y);
		return 1;
	case 0xff0c53ad /* "z" */:
		lua_pushnumber(L, v->z);
		return 1;
	default:
		return 0;
	}
}

// vector newindex (write)
// receives (userdata, key, value)
static int Vector_NewIndex(lua_State *L)
{
	Vector *v = GetVector(L, 1);
	if (!v)
		return 0;
	const char *key = luaL_checkstring(L, 2);
	switch (Hash(key))
	{
	case 0xfd0c5087 /* "x" */:
		v->x = float(luaL_checknumber(L, 3));
		return 0;
	case 0xfc0c4ef4 /* "y" */:
		v->y = float(luaL_checknumber(L, 3));
		return 0;
	case 0xff0c53ad /* "z" */:
		v->z = float(luaL_checknumber(L, 3));
		return 0;
	default:
		return 0;
	}
}

// vector unary minus
static int Vector_Neg(lua_State *L)
{
	Vector *v = RequireVector(L, 1);
	*NewVector(L) = Neg_Vector(*v);
	return 1;
}

// vector add
static int Vector_Add(lua_State *L)
{
	Vector *v1 = RequireVector(L, 1);
	Vector *v2 = RequireVector(L, 2);
	*NewVector(L) = Add_Vectors(*v1, *v2);
	return 1;
}

// vector subtract
static int Vector_Sub(lua_State *L)
{
	Vector *v1 = RequireVector(L, 1);
	Vector *v2 = RequireVector(L, 2);
	*NewVector(L) = Sub_Vectors(*v1, *v2);
	return 1;
}

// vector multiply
static int Vector_Mul(lua_State *L)
{
	if (lua_isnumber(L, 1))
	{
		float s = float(lua_tonumber(L, 1));
		Vector *v = RequireVector(L, 2);
		*NewVector(L) = Vector_Scale(s, *v);
		return 1;
	}
	else if (lua_isnumber(L, 2))
	{
		Vector *v = RequireVector(L, 1);
		float s = float(lua_tonumber(L, 2));
		*NewVector(L) = Vector_Scale(s, *v);
		return 1;
	}
	else
	{
		Vector *v1 = RequireVector(L, 1);
		Vector *v2 = RequireVector(L, 2);
		*NewVector(L) = Vector(v1->x * v2->x, v1->y * v2->y, v1->z * v2->z);
		return 1;
	}
}

// vector divide
static int Vector_Div(lua_State *L)
{
	if (lua_isnumber(L, 1))
	{
		float s = float(lua_tonumber(L, 1));
		Vector *v = RequireVector(L, 2);
		*NewVector(L) = Vector(s / v->x, s / v->y, s / v->z);
		return 1;
	}
	else if (lua_isnumber(L, 2))
	{
		Vector *v = RequireVector(L, 1);
		float s = float(lua_tonumber(L, 2));
		*NewVector(L) = Vector(v->x / s, v->y / s, v->z / s);
		return 1;
	}
	else
	{
		Vector *v1 = RequireVector(L, 1);
		Vector *v2 = RequireVector(L, 2);
		*NewVector(L) = Vector(v1->x / v2->x, v1->y / v2->y, v1->z / v2->z);
		return 0;
	}
}

// vector equality
static int Vector_Eq(lua_State *L)
{
	Vector *v1 = RequireVector(L, 1);
	Vector *v2 = RequireVector(L, 2);
	lua_pushboolean(L, v1->x == v2->x && v1->y == v2->y && v1->z == v2->z);
	return 1;
}

// vector to string
static int Vector_ToString(lua_State *L)
{
	Vector *v = RequireVector(L, 1);
	char buf[MAX_ODF_LENGTH] = {0};
	sprintf_s(buf, "{x=%f, y=%f, z=%f}", v->x, v->y, v->z);
	lua_pushstring(L, buf);
	return 1;
}

// matrix index (read)
// receives (userdata, key)
static int Matrix_Index(lua_State *L)
{
	Matrix *m = GetMatrix(L, 1);
	const char *key = luaL_checkstring(L, 2);
	switch (Hash(key))
	{
	case 0x78e32de5 /* "right" */:
		*NewVector(L) = m->right;
		return 1;
	case 0xb86c3f82 /* "right_x" */:
		lua_pushnumber(L, m->right.x);
		return 1;
	case 0xb96c4115 /* "right_y" */:
		lua_pushnumber(L, m->right.y);
		return 1;
	case 0xb66c3c5c /* "right_z" */:
		lua_pushnumber(L, m->right.z);
		return 1;
	case 0x43430b20 /* "up" */:
		*NewVector(L) = m->up;
		return 1;
	case 0x22f7b28f /* "up_x" */:
		lua_pushnumber(L, m->up.x);
		return 1;
	case 0x21f7b0fc /* "up_y" */:
		lua_pushnumber(L, m->up.y);
		return 1;
	case 0x24f7b5b5 /* "up_z" */:
		lua_pushnumber(L, m->up.z);
		return 1;
	case 0xe179dbd8 /* "front" */:
		*NewVector(L) = m->front;
		return 1;
	case 0xc6a81d47 /* "front_x" */:
		lua_pushnumber(L, m->front.x);
		return 1;
	case 0xc5a81bb4 /* "front_y" */:
		lua_pushnumber(L, m->front.y);
		return 1;
	case 0xc8a8206d /* "front_z" */:
		lua_pushnumber(L, m->front.z);
		return 1;
	case 0x44e771bc /* "posit" */:
		*NewVector(L) = m->posit;
		return 1;
	case 0xecf98bf3 /* "posit_x" */:
		lua_pushnumber(L, m->posit.x);
		return 1;
	case 0xebf98a60 /* "posit_y" */:
		lua_pushnumber(L, m->posit.y);
		return 1;
	case 0xeef98f19 /* "posit_z" */:
		lua_pushnumber(L, m->posit.z);
		return 1;
	default:
		return 0;
	}
}

// matrix newindex (write)
// receives (userdata, key, value)
static int Matrix_NewIndex(lua_State *L)
{
	Matrix *m = GetMatrix(L, 1);
	if (!m)
		return 0;
	const char *key = luaL_checkstring(L, 2);
	switch (Hash(key))
	{
	case 0x78e32de5 /* "right" */:
		if (Vector *v = GetVector(L, 2))
			m->right = *v;
		return 0;
	case 0xb86c3f82 /* "right_x" */:
		m->right.x = float(luaL_checknumber(L, 3));
		return 0;
	case 0xb96c4115 /* "right_y" */:
		m->right.y = float(luaL_checknumber(L, 3));
		return 0;
	case 0xb66c3c5c /* "right_z" */:
		m->right.z = float(luaL_checknumber(L, 3));
		return 0;
	case 0x43430b20 /* "up" */:
		if (Vector *v = GetVector(L, 2))
			m->up = *v;
		return 0;
	case 0x22f7b28f /* "up_x" */:
		m->up.x = float(luaL_checknumber(L, 3));
		return 0;
	case 0x21f7b0fc /* "up_y" */:
		m->up.y = float(luaL_checknumber(L, 3));
		return 0;
	case 0x24f7b5b5 /* "up_z" */:
		m->up.z = float(luaL_checknumber(L, 3));
		return 0;
	case 0xe179dbd8 /* "front" */:
		if (Vector *v = GetVector(L, 2))
			m->front = *v;
		return 0;
	case 0xc6a81d47 /* "front_x" */:
		m->front.x = float(luaL_checknumber(L, 3));
		return 0;
	case 0xc5a81bb4 /* "front_y" */:
		m->front.y = float(luaL_checknumber(L, 3));
		return 0;
	case 0xc8a8206d /* "front_z" */:
		m->front.z = float(luaL_checknumber(L, 3));
		return 0;
	case 0x44e771bc /* "posit" */:
		if (Vector *v = GetVector(L, 2))
			m->posit = Vector(v->x, v->y, v->z);
		return 0;
	case 0xecf98bf3 /* "posit_x" */:
		m->posit.x = float(luaL_checknumber(L, 3));
		return 0;
	case 0xebf98a60 /* "posit_y" */:
		m->posit.y = float(luaL_checknumber(L, 3));
		return 0;
	case 0xeef98f19 /* "posit_z" */:
		m->posit.z = float(luaL_checknumber(L, 3));
		return 0;
	default:
		return 0;
	}
}

// matrix multiply
static int Matrix_Mul(lua_State *L)
{
	Matrix *m1 = GetMatrix(L, 1);
	if (!m1)
		return 0;
	if (Matrix *m2 = GetMatrix(L, 2))
	{
		*NewMatrix(L) = Matrix_Multiply(*m1, *m2);
		return 1;
	}
	else if (Vector *v = GetVector(L, 2))
	{
		*NewVector(L) = Vector_Transform(*m1, *v);
		return 1;
	}
	else
	{
		return 0;
	}
}

// matrix to string
static int Matrix_ToString(lua_State *L)
{
	Matrix *m = GetMatrix(L, 1);
	if (!m)
		return 0;
	char buf[MAX_NAME_LENGTH];
	sprintf_s(buf, "{right_x=%f, right_y=%f, right_z=%f, up_x=%f, up_y=%f, up_z=%f, front_x=%f, front_y=%f, front_z=%f, posit_x=%f, posit_y=%f, posit_z=%f}",
		m->right.x, m->right.y, m->right.z,
		m->up.x,    m->up.y,    m->up.z,
		m->front.x, m->front.y, m->front.z,
		m->posit.x, m->posit.y, m->posit.z
		);
	lua_pushstring(L, buf);
	return 1;
}

// construct a vector from three numbers
static int SetVector(lua_State *L)
{
	float x = float(luaL_optnumber(L, 1, 0.0f));
	float y = float(luaL_optnumber(L, 2, 0.0f));
	float z = float(luaL_optnumber(L, 3, 0.0f));
	*NewVector(L) = Vector(x, y, z);
	return 1;
}

// vector dot product
static int DotProduct(lua_State *L)
{
	Vector *v1 = RequireVector(L, 1);
	Vector *v2 = RequireVector(L, 2);
	lua_pushnumber(L, Dot_Product(*v1, *v2));
	return 1;
}

// vector cross product
static int CrossProduct(lua_State *L)
{
	Vector *v1 = RequireVector(L, 1);
	Vector *v2 = RequireVector(L, 2);
	*NewVector(L) = Cross_Product(*v1, *v2);
	return 1;
}

// vector normalize
static int Normalize(lua_State *L)
{
	Vector *v = RequireVector(L, 1);
	*NewVector(L) = Normalize_Vector(*v);
	return 1;
}

// vector length
static int Length(lua_State *L)
{
	Vector *v = RequireVector(L, 1);
	lua_pushnumber(L, GetLength2D(*v));
	return 1;
}

// vector length squared
static int LengthSquared(lua_State *L)
{
	Vector *v = RequireVector(L, 1);
	lua_pushnumber(L, Dot_Product(*v, *v));
	return 1;
}

// vector distance
static int Distance2D(lua_State *L)
{
	Vector *v1 = RequireVector(L, 1);
	Vector *v2 = RequireVector(L, 2);
	lua_pushnumber(L, sqrtf(GetDistance2DSquared(*v1, *v2)));
	return 1;
}

// vector distance
static int Distance2DSquared(lua_State *L)
{
	Vector *v1 = RequireVector(L, 1);
	Vector *v2 = RequireVector(L, 2);
	lua_pushnumber(L, GetDistance2DSquared(*v1, *v2));
	return 1;
}

// vector distance
static int Distance3D(lua_State *L)
{
	if (Vector *pos = GetVector(L, 1))
	{
		Vector *v1 = RequireVector(L, 1);
		Vector *v2 = RequireVector(L, 2);
		lua_pushnumber(L, GetDistance3D(*v1, *v2));
	}
	else
	{
		Handle h1 = RequireHandle(L, 1);
		Handle h2 = RequireHandle(L, 2);
		lua_pushnumber(L, GetDistance3D(h1, h2));
	}
	return 1;
}

// vector distance squared
static int Distance3DSquared(lua_State *L)
{
	if (Vector *pos = GetVector(L, 1))
	{
		Vector *v1 = RequireVector(L, 1);
		Vector *v2 = RequireVector(L, 2);
		lua_pushnumber(L, GetDistance3DSquared(*v1, *v2));
	}
	else
	{
		Handle h1 = RequireHandle(L, 1);
		Handle h2 = RequireHandle(L, 2);
		lua_pushnumber(L, GetDistance3DSquared(h1, h2));
	}
	return 1;
}

// construct a matrix from 12 numbers
// TO DO: construct a matrix from 4 vectors
static int SetMatrix(lua_State *L)
{
	float right_x = float(luaL_optnumber(L, 1, 0.0f));
	float right_y = float(luaL_optnumber(L, 2, 0.0f));
	float right_z = float(luaL_optnumber(L, 3, 0.0f));
	float up_x = float(luaL_optnumber(L, 4, 0.0f));
	float up_y = float(luaL_optnumber(L, 5, 0.0f));
	float up_z = float(luaL_optnumber(L, 6, 0.0f));
	float front_x = float(luaL_optnumber(L, 7, 0.0f));
	float front_y = float(luaL_optnumber(L, 8, 0.0f));
	float front_z = float(luaL_optnumber(L, 9, 0.0f));
	float posit_x = float(luaL_optnumber(L, 10, 0.0f));
	float posit_y = float(luaL_optnumber(L, 11, 0.0f));
	float posit_z = float(luaL_optnumber(L, 12, 0.0f));
	*NewMatrix(L) = Matrix(
		Vector(right_x, right_y, right_z),
		Vector(up_x,    up_y,    up_z),
		Vector(front_x, front_y, front_z),
		Vector(posit_x, posit_y, posit_z));
	return 1;
}

static int BuildAxisRotationMatrix(lua_State *L)
{
	float angle = float(luaL_optnumber(L, 1, 0.0f));
	float x, y, z;
	if (const Vector *axis = GetVector(L, 2))
	{
		x = axis->x;
		y = axis->y;
		z = axis->z;
	}
	else
	{
		x = float(luaL_optnumber(L, 2, 0.0f));
		y = float(luaL_optnumber(L, 3, 0.0f));
		z = float(luaL_optnumber(L, 4, 0.0f));
	}
	*NewMatrix(L) = Build_Axis_Rotation_Matrix(angle, Vector(x, y, z));
	return 1;
}

static int BuildPositionRotationMatrix(lua_State *L)
{
	float pitch = float(luaL_optnumber(L, 1, 0.0f));
	float yaw = float(luaL_optnumber(L, 2, 0.0f));
	float roll = float(luaL_optnumber(L, 3, 0.0f));
	float x, y, z;
	if (const Vector *posit = GetVector(L, 4))
	{
		x = posit->x;
		y = posit->y;
		z = posit->z;
	}
	else
	{
		x = float(luaL_optnumber(L, 4, 0.0f));
		y = float(luaL_optnumber(L, 5, 0.0f));
		z = float(luaL_optnumber(L, 6, 0.0f));
	}
	*NewMatrix(L) = Build_Position_Rotation_Matrix(pitch, yaw, roll, Vector(x, y, z));
	return 1;
}

static const Vector zero(0.0f, 0.0f, 0.0f);
static const Vector y_axis(0.0f, 1.0f, 0.0f);
static const Vector z_axis(0.0f, 0.0f, 1.0f);

static int BuildOrthogonalMatrix(lua_State *L)
{
	const Vector *up = GetVector(L, 1);
	if (!up)
		up = &y_axis;
	const Vector *heading = GetVector(L, 2);
	if (!heading)
		heading = &z_axis;
	*NewMatrix(L) = Build_Orthogonal_Matrix(*up, *heading);
	return 1;
}

static int BuildDirectionalMatrix(lua_State *L)
{
	const Vector *position = GetVector(L, 1);
	if (!position)
		position = &zero;
	const Vector *direction = GetVector(L, 2);
	if (!direction)
		direction = &z_axis;
	*NewMatrix(L) = Build_Directinal_Matrix(*position, *direction);
	return 1;
}


// Script Util Functions:


static int Make_RGB(lua_State *L)
{
	int r = luaL_optinteger(L, 1, 0);
	int g = luaL_optinteger(L, 2, 0);
	int b = luaL_optinteger(L, 3, 0);
	lua_pushinteger(L, RGB(r,g,b));
	return 1;
}
static int Make_RGBA(lua_State *L)
{
	int r = luaL_optinteger(L, 1, 0);
	int g = luaL_optinteger(L, 2, 0);
	int b = luaL_optinteger(L, 3, 0);
	int a = luaL_optinteger(L, 4, 0);
	lua_pushinteger(L, RGBA_MAKE(r,g,b,a));
	return 1;
}

//void FailMission(Time t, char *fileName = NULL);
static int FailMission(lua_State *L)
{
	Time t = Time(luaL_checknumber(L, 1));
	char *fileName = const_cast<char *>(luaL_optstring(L, 2, NULL));
	FailMission(t, fileName);
	return 0;
}

//void SucceedMission(Time t, char *fileName = NULL);
static int SucceedMission(lua_State *L)
{
	Time t = Time(luaL_checknumber(L, 1));
	char *fileName = const_cast<char *>(luaL_optstring(L, 2, NULL));
	SucceedMission(t, fileName);
	return 0;
}

//DLLAPI *ChangeSide)(void);
static int ChangeSide(lua_State *L)
{
	ChangeSide();
	return 0;
}

//ScrapValue AddScrap(TeamNum t, ScrapValue v);
static int AddScrap(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	ScrapValue v = ScrapValue(luaL_checkinteger(L, 2));
	lua_pushinteger(L, AddScrap(t, v));
	return 1;
}

//ScrapValue SetScrap(TeamNum t, ScrapValue v);
static int SetScrap(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	ScrapValue v = ScrapValue(luaL_checkinteger(L, 2));
	lua_pushinteger(L, SetScrap(t, v));
	return 1;
}

//ScrapValue GetScrap(TeamNum t);
static int GetScrap(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	lua_pushinteger(L, GetScrap(t));
	return 1;
}

//void AddMaxScrap(TeamNum t, ScrapValue v);
static int AddMaxScrap(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	ScrapValue v = ScrapValue(luaL_checkinteger(L, 2));
	AddMaxScrap(t, v);
	lua_pushinteger(L, GetMaxScrap(t)); // Report back the new value?
	return 1;
}

//void SetMaxScrap(TeamNum t, ScrapValue v);
static int SetMaxScrap(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	ScrapValue v = ScrapValue(luaL_checkinteger(L, 2));
	SetMaxScrap(t, v);
	lua_pushinteger(L, GetMaxScrap(t)); // Report back the new value?
	return 1;
}

//ScrapValue GetMaxScrap(TeamNum t);
static int GetMaxScrap(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	lua_pushinteger(L, GetMaxScrap(t));
	return 1;
}

// Time GetTime(void)
static int GetTime(lua_State *L)
{
	lua_pushnumber(L, GetTime());
	return 1;
}

//Handle GetTug(Handle cargo);
static int GetTug(lua_State *L)
{
	Handle cargo = RequireHandle(L, 1);
	Handle tug = GetTug(cargo);
	PushHandle(L, tug);
	return 1;
}

//bool HasCargo(Handle me);
static int HasCargo(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushboolean(L, HasCargo(me));
	return 1;
}

//Distance GetDistance(Handle &h1, const Vector &pos)
//Distance GetDistance(Handle &h1, Handle &h2)
//Distance GetDistance(Handle &h1, Name path, int point = 0);
//inline float GetDistance(Handle h, Vector v) { return GetDistance2D(GetPosition(h), v); }
//inline float GetDistance(Handle h, Matrix m) { return GetDistance2D(GetPosition(h), m.posit); }
static int GetDistance(lua_State *L)
{
	Handle h1 = RequireHandle(L, 1);
	Dist d;
	if (Matrix *mat = GetMatrix(L, 2))
	{
		d = GetDistance(h1, mat->posit);
	}
	else if (Vector *pos = GetVector(L, 2))
	{
		d = GetDistance(h1, *pos);
	}
	else if (lua_isstring(L, 2))
	{
		Name path = Name(lua_tostring(L, 2));
		int point = luaL_optinteger(L, 3, 0);
		d = GetDistance(h1, path, point);
	}
	else
	{
		Handle h2 = RequireHandle(L, 2);
		d = GetDistance(h1, h2);
	}
	lua_pushnumber(L, d);
	return 1;
}

//Handle GetNearestObject(Handle h);
static int GetNearestObject(lua_State *L)
{
	Handle n = 0;
	/* // Need a version that takes a Vector for the postion to enable all of these. -GBD //Handle GetNearestObject(const Vector &pos); //Handle GetNearestObect(Name path,int point);
	if (Matrix *mat = GetMatrix(L, 1))
	{
		n = GetNearestObject(mat->posit);
	}
	else if (Vector *pos = GetVector(L, 1))
	{
		n = GetNearestObject(*pos);
	}
	else if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		n = GetNearestObject(GetVectorFromPath(path, point));
	}
	else
	{
	*/
		Handle h = RequireHandle(L, 1);
		n = GetNearestObject(h);
	//}
	PushHandle(L, n);
	return 1;
}

//Handle GetNearestVehicle(Handle h);
//Handle GetNearestVehicle(Name path,int point);
static int GetNearestVehicle(lua_State *L)
{
	Handle n = 0;
	/* // Need a version that takes a Vector for the postion to enable all of these. -GBD //Handle GetNearestVehicle(const Vector &pos);
	if (Matrix *mat = GetMatrix(L, 1))
	{
		n = GetNearestVehicle(mat->posit);
	}
	else if (Vector *pos = GetVector(L, 1))
	{
		n = GetNearestVehicle(*pos);
	}
	else 
	*/
	if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		n = GetNearestVehicle(path, point);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		n = GetNearestVehicle(h);
	}
	PushHandle(L, n);
	return 1;
}

//Handle GetNearestBuilding(Handle h);
static int GetNearestBuilding(lua_State *L)
{
	Handle n = 0;
	/* // Need a version that takes a Vector for the postion to enable all of these. -GBD //Handle GetNearestBuilding(const Vector &pos); //Handle GetNearestBuilding(Name path,int point);
	if (Matrix *mat = GetMatrix(L, 1))
	{
		n = GetNearestBuilding(mat->posit);
	}
	else if (Vector *pos = GetVector(L, 1))
	{
		n = GetNearestBuilding(*pos);
	}
	else if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		n = GetNearestBuilding(GetVectorFromPath(path, point));
	}
	else
	{
	*/
		Handle h = RequireHandle(L, 1);
		n = GetNearestBuilding(h);
	//}
	PushHandle(L, n);
	return 1;
}

//Handle GetNearestEnemy(Handle h);
//DLLEXPORT Handle DLLAPI GetNearestEnemy(Handle h, bool ignorePilots, bool ignoreScavs, float maxDist = 450.0f);
static int GetNearestEnemy(lua_State *L)
{
	Handle n = 0;
	/* // Need a version that takes a Vector for the postion to enable all of these. -GBD //Handle GetNearestEnemy(const Vector &pos); //Handle GetNearestEnemy(Name path,int point);
	if (Matrix *mat = GetMatrix(L, 1))
	{
		n = GetNearestEnemy(mat->posit);
	}
	else if (Vector *pos = GetVector(L, 1))
	{
		n = GetNearestEnemy(*pos);
	}
	else if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		n = GetNearestEnemy(path, point);
	}
	else 
	*/
	if(lua_isboolean(L, 2))
	{
		Handle h = RequireHandle(L, 1);
		bool ignorepilot = lua_toboolean(L, 2);
		bool ignorescav = lua_toboolean(L, 3);
		float maxdist = float(luaL_optnumber(L, 4, 450.0f));
		n = GetNearestEnemy(h, ignorepilot, ignorescav, maxdist);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		n = GetNearestEnemy(h);
	}
	PushHandle(L, n);
	return 1;
}

//Handle BuildObject(char *odf, int team, Handle h);
//Handle BuildObject(char *odf, int team, Name path, int point = 0)
//Handle BuildObject(char *odf, int team, Vector v);
//Handle BuildObject(char *odf, int team, Matrix m);
static int BuildObject(lua_State *L)
{
	char *odf = const_cast<char *>(luaL_checkstring(L, 1));
	int team = luaL_checkinteger(L, 2);
	Handle o = 0;
	if (Matrix *mat = GetMatrix(L, 3))
	{
		o = BuildObject(odf, team, *mat);
	}
	else if (Vector *pos = GetVector(L, 3))
	{
		o = BuildObject(odf, team, *pos);
	}
	else if (lua_isstring(L, 3))
	{
		Name path = Name(lua_tostring(L, 3));
		int point = luaL_optinteger(L, 4, 0);
		if(point)
		{
			Vector pos = GetVectorFromPath(path, point);
			o = BuildObject(odf, team, pos);
		}
		else
		{
			o = BuildObject(odf, team, path);
		}
	}
	else
	{
		Handle h = RequireHandle(L, 3);
		o = BuildObject(odf, team, h);
	}
	PushHandle(L, o);
	return 1;
}

// void RemoveObject(Handle h)
static int RemoveObject(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	RemoveObject(h);
	return 0;
}

//DLLEXPORT int DLLAPI GetFirstEmptyGroup(void);
//DLLEXPORT int DLLAPI GetFirstEmptyGroup(int t);
static int GetFirstEmptyGroup(lua_State *L)
{
	int group = 0;
	if (lua_isnumber(L, 1))
	{
		TeamNum team = TeamNum(lua_tointeger(L, 1));
		group = GetFirstEmptyGroup(team);
	}
	else
	{
		group = GetFirstEmptyGroup();
	}

	lua_pushinteger(L, group);
	return 1;
}

//DLLEXPORT void DLLAPI SetGroup(Handle h, int group);
static int SetGroup(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int group = luaL_optinteger(L, 2, 0);
	SetGroup(h, group);
	return 0;
}

//void Attack(Handle me, Handle him, int priority = 1);
static int Attack(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	Attack(me, him, priority);
	return 0;
}

//DLLEXPORT void DLLAPI Service(Handle me, Handle him, int priority = 1);
static int Service(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	Service(me, him, priority);
	return 0;
}

//void Goto(Handle me, const Vector &pos, int priority = 1);
//void Goto(Handle me, Name path, int priority = 1);
//void Goto(Handle me, Handle him, int priority = 1);
//inline void Goto(Handle h, Matrix Position, int Priority) { Goto(h, Position.posit, Priority); }
//inline void Goto(Handle h, char* Path, int Point, int Priority) { Goto(h, GetVectorFromPath(Path, Point), Priority); }
static int Goto(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int priority = luaL_optinteger(L, 3, 1);
	if (Matrix *mat = GetMatrix(L, 2))
	{
		Goto(me, *mat, priority);
	}
	else if (Vector *pos = GetVector(L, 2))
	{
		Goto(me, *pos, priority);
	}
	else if (lua_isstring(L, 2))
	{
		Name path = Name(lua_tostring(L, 2));

		if (lua_isnumber(L, 4))
		{
			int point = luaL_optinteger(L, 3, 0);
			priority = luaL_optinteger(L, 4, 1);
			Goto(me, path, point, priority);
		}
		else
		{
			Goto(me, path, priority);
		}
	}
	else
	{
		Handle him = RequireHandle(L, 2);
		Goto(me, him, priority);
	}
	return 0;
}

//void Mine(Handle me, Name path, int priority = 1);
//DLLEXPORT void DLLAPI Mine(Handle me, const Vector& pos, int priority = 1);
//inline void Mine(Handle h, Matrix Where, int Priority = 1) { Mine(h, Where.posit, Priority); }
//inline void Mine(Handle me, Handle him, int Priority = 1) { Mine(me, GetPosition(him), Priority); }
//inline void Mine(Handle h, char* Path, int Point, int Priority) { Mine(h, GetVectorFromPath(Path, Point), Priority); }
static int Mine(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int priority = luaL_optinteger(L, 3, 1);
	if (Matrix *mat = GetMatrix(L, 2))
	{
		Mine(me, mat->posit, priority);
	}
	else if (Vector *pos = GetVector(L, 2))
	{
		Mine(me, *pos, priority);
	}
	else if (lua_isstring(L, 2))
	{
		Name path = Name(lua_tostring(L, 2));

		if (lua_isnumber(L, 4))
		{
			int point = luaL_optinteger(L, 3, 0);
			priority = luaL_optinteger(L, 4, 1);
			Mine(me, path, point, priority);
		}
		else
		{
			Mine(me, path, priority);
		}
	}
	else
	{
		Handle him = RequireHandle(L, 2);
		Mine(me, him, priority);
	}
	return 0;
}

//void Follow(Handle me, Handle him, int priority = 1);
static int Follow(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	Follow(me, him, priority);
	return 0;
}

//void Defend(Handle me, int priority = 1);
static int Defend(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int priority = luaL_optinteger(L, 2, 1);
	Defend(me, priority);
	return 0;
}

//void Defend2(Handle me, Handle him, int priority = 1);
static int Defend2(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	Defend2(me, him, priority);
	return 0;
}

//void Stop(Handle me, int priority = 1);
static int Stop(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int priority = luaL_optinteger(L, 2, 1);
	Stop(me, priority);
	return 0;
}

//void Patrol(Handle me, Name path, int priority = 1);
static int Patrol(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Name path = Name(luaL_checkstring(L, 2));
	int priority = luaL_optinteger(L, 3, 1);
	Patrol(me, path, priority);
	return 0;
}

//void Retreat(Handle me, Name path, int priority = 1);
//void Retreat(Handle me, Handle him, int priority = 1);
// Vector + Matrix improvised with Goto/Independence. -GBD
static int Retreat(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int priority = luaL_optinteger(L, 3, 1);
	if (Matrix *mat = GetMatrix(L, 2))
	{
		Retreat(me, *mat, priority);
	}
	else if (Vector *pos = GetVector(L, 2))
	{
		Retreat(me, *pos, priority);
	}
	else if (lua_isstring(L, 2))
	{
		Name path = Name(lua_tostring(L, 2));

		if (lua_isnumber(L, 4))
		{
			int point = luaL_optinteger(L, 3, 0);
			priority = luaL_optinteger(L, 4, 1);
			Retreat(me, path, point, priority);
		}
		else
		{
			Retreat(me, path, priority);
		}
	}
	else
	{
		Handle him = RequireHandle(L, 2);
		Retreat(me, him, priority);
	}
	return 0;
}

//void GetIn(Handle me, Handle him, int priority = 1);
static int GetIn(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	GetIn(me, him, priority);
	return 0;
}

//void Pickup(Handle me, Handle him, int priority = 1);
static int Pickup(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	Pickup(me, him, priority);
	return 0;
}

//void Dropoff(Handle me, const Matrix &mat, int priority = 1);
//void Dropoff(Handle me, const Vector &pos, int priority = 1);
//void Dropoff(Handle me, Name path, int priority = 1);
//inline void Dropoff(Handle h, char* Path, int Point, int Priority) { Dropoff(h, GetVectorFromPath(Path, Point), Priority); }
//inline void Dropoff(Handle me, Handle him, int Priority = 1) { Dropoff(me, GetPosition(him), Priority); }
static int Dropoff(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int priority = luaL_optinteger(L, 3, 1);

	if (Matrix *mat = GetMatrix(L, 2))
	{
		Dropoff(me, *mat, priority);
	}
	else if (Vector *pos = GetVector(L, 2))
	{
		Dropoff(me, *pos, priority);
	}
	else if (lua_isstring(L, 2))
	{
		Name path = Name(lua_tostring(L, 2));

		if (lua_isnumber(L, 4))
		{
			int point = luaL_optinteger(L, 3, 0);
			priority = luaL_optinteger(L, 4, 1);
			Dropoff(me, path, point, priority);
		}
		else
		{
			Dropoff(me, path, priority);
		}
	}
	else
	{
		Handle him = RequireHandle(L, 2);
		Dropoff(me, him, priority);
	}
	return 0;
}

//void Build(Handle me, char *odf, int priority = 1);
static int Build(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	char *odf = const_cast<char *>(luaL_checkstring(L, 2));
	int priority = luaL_optinteger(L, 3, 1);
	Build(me, odf, priority);
	return 0;
}

//DLLEXPORT void DLLAPI LookAt(Handle me, Handle him, int priority = 1);
static int LookAt(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	LookAt(me, him, priority);
	return 0;
}

//DLLEXPORT void DLLAPI AllLookAt(int team, Handle him, int priority = 1);
static int AllLookAt(lua_State *L)
{
	int team = int(luaL_checknumber(L, 1));
	Handle him = RequireHandle(L, 2);
	int priority = luaL_optinteger(L, 3, 1);
	AllLookAt(team, him, priority);
	return 0;
}

//bool IsOdf(Handle h, char *odf)
static int IsOdf(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	char *odf = const_cast<char *>(luaL_checkstring(L, 2));
	lua_pushboolean(L, IsOdf(h, odf));
	return 1;
}

//DLLEXPORT char DLLAPI GetRace(Handle h);
static int GetRace(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	char race = GetRace(h);
	lua_pushlstring(L, &race, sizeof(char));
	return 1;
}

//Handle GetPlayerHandle(void);
//Handle GetPlayerHandle(int team);
static int GetPlayerHandle(lua_State *L)
{
	Handle p = 0;
	if (lua_isnumber(L, 1))
	{
		TeamNum team = TeamNum(lua_tointeger(L, 1));
		p = GetPlayerHandle(team);
	}
	else
	{
		p = GetPlayerHandle();
	}
	PushHandle(L, p);
	return 1;
}

//bool IsAlive(Handle &h);
static int IsAlive(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsAlive(h));
	return 1;
}

//DLLEXPORT bool DLLAPI IsFlying(Handle &h);
static int IsFlying(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsFlying(h));
	return 1;
}

//bool IsAliveAndPilot(Handle &h);
static int IsAliveAndPilot(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsAliveAndPilot(h));
	return 1;
}

//bool IsAround(Handle &h); //was bool IsVaid(Handle &h); in BZ1. -GBD
static int IsAround(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsAround(h));
	return 1;
}

//DLLEXPORT Handle DLLAPI InBuilding(Handle h);
static int InBuilding(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	PushHandle(L, InBuilding(h));
	return 1;
}

//DLLEXPORT Handle DLLAPI AtTerminal(Handle h);
static int AtTerminal(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	PushHandle(L, AtTerminal(h));
	return 1;
}

//Vector GetPosition(Handle h);
//Vector GetPosition(Name path, int point = 0);
static int GetPosition(lua_State *L)
{
	if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		*NewVector(L) = GetPosition(path, point);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		*NewVector(L) = GetPosition(h);
	}
	return 1;
}

//DLLEXPORT void DLLAPI GetPosition2(Handle h, Vector &pos);
static int GetPosition2(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	GetPosition2(h, *NewVector(L));
	return 1;
}

//Vector GetFront(Handle h);
static int GetFront(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	*NewVector(L) = GetFront(h);
	return 1;
}


//void SetPosition(Handle h, Matrix &mat);
//void SetPosition(Handle h, Vector &pos);
//void SetPosition(Handle h, Name path, int point = 0);
static int SetPosition(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	if (Matrix *mat = GetMatrix(L, 2))
	{
		SetPosition(h, *mat);
		SetVectorPosition(h, mat->posit); // Corrects height. -GBD
	}
	else if (Vector *pos = GetVector(L, 2))
	{
		SetVectorPosition(h, *pos);
	}
	else if (lua_isstring(L, 2))
	{
		Name path = Name(lua_tostring(L, 2));
		int point = luaL_optinteger(L, 3, 0);
		if(point)
			SetPositionPath(h, path, point);
		else
			SetPosition(h, path);
	}
	else
	{
		Handle h2 = RequireHandle(L, 2);
		SetPosition(h, h2);
	}
	return 0;
}

//void Damage(Handle him, long amt);
//void DamageF(Handle him, float amt);
//void Damage(Handle him, Handle me, long amt);
static int Damage(lua_State *L)
{
	Handle him = RequireHandle(L, 1);
	// Damager version.
	if (lua_isnumber(L, 3))
	{
		Handle me = RequireHandle(L, 2);
		long amt = long(lua_tonumber(L, 3));
		Damage(him, me, amt); // New version that takes a damager.
	}
	else
	{
		long amt = long(luaL_checknumber(L, 2));
		DamageF(him, float(amt)); // We use DamageF now, because F it, Lua numbers are doubles. -GBD
	}
	return 0;
}

//float GetHealth(Handle h);
static int GetHealth(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushnumber(L, GetHealth(me));
	return 1;
}

//long GetCurHealth(Handle h);
static int GetCurHealth(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushinteger(L, GetCurHealth(me));
	return 1;
}

//long GetMaxHealth(Handle h);
static int GetMaxHealth(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushinteger(L, GetMaxHealth(me));
	return 1;
}

//void SetCurHealth(Handle h, long NewHealth);
static int SetCurHealth(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	long health = luaL_checklong(L, 2);
	SetCurHealth(me, health);
	return 0;
}

//void SetMaxHealth(Handle h, long NewHealth);
static int SetMaxHealth(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	long health = luaL_checklong(L, 2);
	SetMaxHealth(me, health);
	return 0;
}

//void AddHealth(Handle h, long health);
static int AddHealth(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	long health = luaL_checklong(L, 2);
	AddHealth(me, health);
	return 0;
}

//float GetAmmo(Handle h);
static int GetAmmo(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushnumber(L, GetAmmo(me));
	return 1;
}

//long GetCurAmmo(Handle h);
static int GetCurAmmo(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushinteger(L, GetCurAmmo(me));
	return 1;
}

//long GetMaxAmmo(Handle h);
static int GetMaxAmmo(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushinteger(L, GetMaxAmmo(me));
	return 1;
}

//void SetCurAmmo(Handle h, long NewAmmo);
static int SetCurAmmo(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	long ammo = luaL_checklong(L, 2);
	SetCurAmmo(me, ammo);
	return 0;
}

//void SetMaxAmmo(Handle h, long NewAmmo);
static int SetMaxAmmo(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	long ammo = luaL_checklong(L, 2);
	SetMaxAmmo(me, ammo);
	return 0;
}

//void AddAmmo(Handle h, long ammo);
static int AddAmmo(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	long ammo = luaL_checklong(L, 2);
	AddAmmo(me, ammo);
	return 0;
}

//TeamNum GetTeamNum(Handle h)
static int GetTeamNum(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetTeamNum(h));
	return 1;
}

//void SetTeamNum(Handle h, TeamNum t);
static int SetTeamNum(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	TeamNum t = TeamNum(luaL_checkinteger(L, 2));
	SetTeamNum(h, t);
	return 0;
}

//void SetVelocity(Handle, Vector &vel);
static int SetVelocity(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	if (Vector *v = GetVector(L, 2))
		SetVelocity(h, *v);

	return 0;
}

//DLLEXPORT void DLLAPI SetControls(Handle h, const VehicleControls &controls, unsigned long whichControls = -1);
static int SetControls(lua_State *L)
{
	Handle h = GetHandle(L, 1);

	VehicleControls controls;
	unsigned long whichControls = 0;

	if (lua_istable(L, 2))
	{
		lua_getfield(L, -1, "braccel");
		if (lua_isnumber(L, -1)) {
			whichControls |= CTRL_BRACCEL;
			controls.braccel = float(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "steer");
		if (lua_isnumber(L, -1)) {
			whichControls |= CTRL_STEER;
			controls.steer = float(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "pitch");
		if (lua_isnumber(L, -1)) {
			whichControls |= CTRL_PITCH;
			controls.pitch = float(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "strafe");
		if (lua_isnumber(L, -1)) {
			whichControls |= CTRL_STRAFE;
			controls.strafe = float(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "jump");
		if (lua_isboolean(L, -1)) {
			whichControls |= CTRL_JUMP;
			controls.jump = char(lua_toboolean(L, -1));
		}
		else if (lua_isnumber(L, -1))
		{
			whichControls |= CTRL_JUMP;
			controls.jump = char(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "deploy");
		if (lua_isboolean(L, -1)) {
			whichControls |= CTRL_DEPLOY;
			controls.deploy = char(lua_toboolean(L, -1));
		}
		else if (lua_isnumber(L, -1))
		{
			whichControls |= CTRL_DEPLOY;
			controls.deploy = char(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "eject");
		if (lua_isboolean(L, -1)) {
			whichControls |= CTRL_EJECT;
			controls.eject = char(lua_toboolean(L, -1));
		}
		else if (lua_isnumber(L, -1))
		{
			whichControls |= CTRL_EJECT;
			controls.eject = char(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "abandon");
		if (lua_isboolean(L, -1)) {
			whichControls |= CTRL_ABANDON;
			controls.abandon = char(lua_toboolean(L, -1));
		}
		else if (lua_isnumber(L, -1))
		{
			whichControls |= CTRL_ABANDON;
			controls.abandon = char(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "fire");
		if (lua_isboolean(L, -1)) {
			whichControls |= CTRL_FIRE;
			controls.fire = char(lua_toboolean(L, -1));
		}
		else if (lua_isnumber(L, -1))
		{
			whichControls |= CTRL_FIRE;
			controls.fire = char(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);
	}

	SetControls(h, controls, whichControls);
	return 0;
}

//Handle GetWhoShotMe(Handle h)
static int GetWhoShotMe(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	PushHandle(L, GetWhoShotMe(h));
	return 1;
}

//float GetLastEnemyShot(Handle h)
static int GetLastEnemyShot(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushnumber(L, GetLastEnemyShot(h));
	return 1;
}

//float GetLastFriendShot(Handle h)
static int GetLastFriendShot(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushnumber(L, GetLastFriendShot(h));
	return 1;
}

//void DefaultAllies(void);
static int DefaultAllies(lua_State *L)
{
	DefaultAllies();
	return 0;
}

//DLLEXPORT void DLLAPI TeamplayAllies(void);
static int TeamplayAllies(lua_State *L)
{
	TeamplayAllies();
	return 0;
}

//void Ally(TeamNum t1, TeamNum t2);
static int Ally(lua_State *L)
{
	TeamNum t1 = TeamNum(luaL_checkinteger(L, 1));
	TeamNum t2 = TeamNum(luaL_checkinteger(L, 2));
	Ally(t1, t2);
	return 0;
}

//void UnAlly(TeamNum t1, TeamNum t2);
static int UnAlly(lua_State *L)
{
	TeamNum t1 = TeamNum(luaL_checkinteger(L, 1));
	TeamNum t2 = TeamNum(luaL_checkinteger(L, 2));
	UnAlly(t1, t2);
	return 0;
}

//bool IsTeamAllied(TeamNum t1, TeamNum t2);
static int IsTeamAllied(lua_State *L)
{
	TeamNum t1 = TeamNum(luaL_checkinteger(L, 1));
	TeamNum t2 = TeamNum(luaL_checkinteger(L, 2));
	lua_pushboolean(L, IsTeamAllied(t1, t2));
	return 1;
}

//bool IsAlly(Handle me, Handle him);
static int IsAlly(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	lua_pushboolean(L, IsAlly(me, him));
	return 1;
}

//int AudioMessage(char *msg);
static int AudioMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	lua_pushinteger(L, AudioMessage(msg));
	return 1;
}

//bool IsAudioMessageDone(int msg);
static int IsAudioMessageDone(lua_State *L)
{
	int msg = luaL_checkinteger(L, 1);
	lua_pushboolean(L, IsAudioMessageDone(msg));
	return 1;
}

//void StopAudioMessage(int Msg);
static int StopAudioMessage(lua_State *L)
{
	int msg = luaL_checkinteger(L, 1);
	StopAudioMessage(msg);
	return 0;
}

//DLLEXPORT void DLLAPI PreloadAudioMessage(const char* msg);
static int PreloadAudioMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	PreloadAudioMessage(msg);
	return 0;
}

//DLLEXPORT void DLLAPI PurgeAudioMessage(const char* msg);
static int PurgeAudioMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	PurgeAudioMessage(msg);
	return 0;
}

//DLLEXPORT void DLLAPI PreloadMusicMessage(const char* msg);
static int PreloadMusicMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	PreloadMusicMessage(msg);
	return 0;
}

//DLLEXPORT void DLLAPI PurgeMusicMessage(const char* msg);
static int PurgeMusicMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	PurgeMusicMessage(msg);
	return 0;
}

//DLLEXPORT void DLLAPI LoadJukeFile(char* msg);
static int LoadJukeFile(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	LoadJukeFile(const_cast<char*>(msg));
	return 0;
}

//DLLEXPORT void DLLAPI SetMusicIntensity(int intensity);
static int SetMusicIntensity(lua_State *L)
{
	int msg = luaL_checkinteger(L, 1);
	SetMusicIntensity(msg);
	return 0;
}

/* // AiPath pointer data type not supported from Lua. -GBD
// create an AiPath pointer on the lua stack
static AiPath **NewAiPath(lua_State *L)
{
	AiPath **p = static_cast<AiPath **>(lua_newuserdata(L, sizeof(AiPath *)));
	luaL_getmetatable(L, "AiPath");
	lua_setmetatable(L, -2);
	return p;
}

// get an AiPath pointer from the lua stack
// returns NULL if the item is not an AiPath pointer
static AiPath **GetAiPath(lua_State *L, int n)
{
	return static_cast<AiPath **>(luaL_testudata(L, n, "AiPath"));
}

//DLLEXPORT AiPath *DLLAPI FindAiPath(const Vector &start, const Vector &goal);
static int FindAiPath(lua_State *L)
{
	Vector *start = GetVector(L, 1);
	Vector *goal = GetVector(L, 2);
	AiPath* path = FindAiPath(*start, *goal);
	*NewAiPath(L) = path;
	return 1;
}

//DLLEXPORT void DLLAPI FreeAiPath(AiPath *path);
static int FreeAiPath(lua_State *L)
{
	if (AiPath **p = GetAiPath(L, 1))
	{
		if (AiPath *path = *p)
			FreeAiPath(path);
	}
	return 0;
}
*/

//DLLEXPORT void DLLAPI GetAiPaths(int &pathCount, char* *&pathNames);
static int GetAiPaths(lua_State *L)
{
	int count;
	char **paths;
	GetAiPaths(count, paths);

	lua_createtable(L, count, 0);
	for (int i = 0; i < count; ++i)
	{
		lua_pushstring(L, paths[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

//void SetPathType(Name path, PathType pathType);
static int SetPathType(lua_State *L)
{
	Name path = Name(luaL_checkstring(L, 1));
	PathType type = PathType(luaL_checkinteger(L, 2));
	SetPathType(path, type);
	return 0;
}

//void SetPathType(Name path, PathType pathType);
static int SetPathOneWay(lua_State *L)
{
	Name path = Name(luaL_checkstring(L, 1));
	SetPathType(path, ONE_WAY_PATH);
	return 0;
}
static int SetPathRoundTrip(lua_State *L)
{
	Name path = Name(luaL_checkstring(L, 1));
	SetPathType(path, ROUND_TRIP_PATH);
	return 0;
}
static int SetPathLoop(lua_State *L)
{
	Name path = Name(luaL_checkstring(L, 1));
	SetPathType(path, LOOP_PATH);
	return 0;
}

//void SetIndependence(Handle me, int independence);
static int SetIndependence(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int independence = luaL_checkinteger(L, 2);
	SetIndependence(me, independence);
	return 0;
}

//bool IsInfo(Handle h);
//bool IsInfo(Name odf);
static int IsInfo(lua_State *L)
{
	if (lua_isstring(L, 1))
	{
		Name odf = Name(lua_tostring(L, 1));
		lua_pushboolean(L, IsInfo(odf));
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		lua_pushboolean(L, IsInfo(h));
	}
	return 1;
}

//void StartCockpitTimer(long time, long warn = LONG_MIN, long alert = LONG_MIN);
static int StartCockpitTimer(lua_State *L)
{
	long time = luaL_checklong(L, 1);
	long warn = luaL_optlong(L, 2, LONG_MIN);
	long alert = luaL_optlong(L, 3, LONG_MIN);
	StartCockpitTimer(time, warn, alert);
	return 0;
}

//void StartCockpitTimerUp(long time, long warn = LONG_MAX, long alert = LONG_MAX);
static int StartCockpitTimerUp(lua_State *L)
{
	long time = luaL_checklong(L, 1);
	long warn = luaL_optlong(L, 2, LONG_MAX);
	long alert = luaL_optlong(L, 3, LONG_MAX);
	StartCockpitTimerUp(time, warn, alert);
	return 0;
}

//void StopCockpitTimer(void);
static int StopCockpitTimer(lua_State *L)
{
	StopCockpitTimer();
	return 0;
}

//void HideCockpitTimer(void);
static int HideCockpitTimer(lua_State *L)
{
	HideCockpitTimer();
	return 0;
}

//long GetCockpitTimer(void);
static int GetCockpitTimer(lua_State *L)
{
	long t = GetCockpitTimer();
	lua_pushinteger(L, t);
	return 1;
}

//void StartEarthquake(float magnitude);
static int StartEarthQuake(lua_State *L)
{
	float magnitude = float(luaL_checknumber(L, 1));
	StartEarthQuake(magnitude);
	return 0;
}

//void UpdateEarthQuake(float magnitude);
static int UpdateEarthQuake(lua_State *L)
{
	float magnitude = float(luaL_checknumber(L, 1));
	UpdateEarthQuake(magnitude);
	return 0;
}

//void StopEarthquake(void);
static int StopEarthQuake(lua_State *L)
{
	StopEarthQuake();
	return 0;
}

//bool IsPerson(Handle &h);
static int IsPerson(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsPerson(h));
	return 1;
}

//DLLEXPORT int DLLAPI GetCurWorld(void);
static int GetCurWorld(lua_State *L)
{
	lua_pushnumber(L, GetCurWorld());
	return 1;
}

//DLLEXPORT const char* DLLAPI GetVarItemStr(char* VarItemName);
static int GetVarItemStr(lua_State *L)
{
	const char *var = luaL_checkstring(L, 1);
	lua_pushstring(L, GetVarItemStr(const_cast<char*>(var)));
	return 1;
}

//DLLEXPORT const int DLLAPI GetVarItemInt(char* VarItemName);
static int GetVarItemInt(lua_State *L)
{
	const char *var = luaL_checkstring(L, 1);
	lua_pushnumber(L, GetVarItemInt(const_cast<char*>(var)));
	return 1;
}

//DLLEXPORT const int DLLAPI GetCVarItemInt(int TeamNum,int Idx);
static int GetCVarItemInt(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	int index = luaL_checkinteger(L, 2);
	lua_pushnumber(L, GetCVarItemInt(team, index));
	return 1;
}

//DLLEXPORT const char*  DLLAPI GetCVarItemStr(int TeamNum,int Idx);
static int GetCVarItemStr(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	int index = luaL_checkinteger(L, 2);
	lua_pushstring(L, GetCVarItemStr(team, index));
	return 1;
}

//DLLEXPORT void DLLAPI PreloadODF(char* cfg);
static int PreloadODF(lua_State *L)
{
	const char *odf = luaL_checkstring(L, 1);
	PreloadODF(const_cast<char*>(odf));
	return 0;
}

//DLLEXPORT float DLLAPI TerrainFindFloor(float x,float z);
//inline float TerrainFindFloor(Vector pos) { return TerrainFindFloor(pos.x, pos.z); }
//inline float TerrainFindFloor(Matrix mat) { return TerrainFindFloor(mat.posit.x, mat.posit.z); }
//inline float TerrainFindFloor(Name Path, int Point = 0) { Vector Test = GetVectorFromPath(Path, Point); return TerrainFindFloor(Test.x, Test.z); }
//inline float TerrainFindFloor(Handle h) { return TerrainFindFloor(GetPosition(h)); }
//-number, vector GetTerrainHeightAndNormal ( handle h [ bool UseWater = false ] )
static int TerrainFindFloor(lua_State *L)
{
	//Handle h = RequireHandle(L, 1);

	if (Matrix *mat = GetMatrix(L, 1))
	{
		lua_pushnumber(L, TerrainFindFloor(*mat));
	}
	else if (Vector *pos = GetVector(L, 1))
	{
		lua_pushnumber(L, TerrainFindFloor(*pos));
	}
	else if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		lua_pushnumber(L, TerrainFindFloor(path, point));
	}
	else if (Handle h = GetHandle(L, 1))
	{
		lua_pushnumber(L, TerrainFindFloor(h));
	}
	else
	{
		float x = float(luaL_checknumber(L, 1));
		float z = float(luaL_checknumber(L, 2));
		lua_pushnumber(L, TerrainFindFloor(x, z));
	}

	return 1;
}

//DLLEXPORT void DLLAPI AddPilotByHandle(Handle h);
static int AddPilotByHandle(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	AddPilotByHandle(h);
	return 0;
}

//DLLEXPORT void DLLAPI PrintConsoleMessage(char* msg);
static int PrintConsoleMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	PrintConsoleMessage(const_cast<char *>(msg));
	return 0;
}

//DLLEXPORT float DLLAPI GetRandomFloat(float range);
// inline float GetRandomFloat(float min, float max);
static int GetRandomFloat(lua_State *L)
{
	float minrange = float(luaL_checknumber(L, 1));
	if (lua_isnumber(L, 2))
	{
		float maxrange = float(lua_tonumber(L, 2));
		lua_pushnumber(L, GetRandomFloat(minrange, maxrange));
	}
	else
	{
		lua_pushnumber(L, GetRandomFloat(minrange));
	}
	return 1;
}

//bool IsDeployed(Handle h);
static int IsDeployed(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsDeployed(h));
	return 1;
}

//void Deploy(Handle h);
static int Deploy(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Deploy(h);
	return 0;
}

//bool IsSelected(Handle h);
static int IsSelected(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsSelected(h));
	return 1;
}

//void SetWeaponMask(Handle me, long mask);
static int SetWeaponMask(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	long mask = luaL_checklong(L, 2);
	SetWeaponMask(me, mask);
	return 0;
}

//bool GiveWeapon(Handle me, Name weapon);
//bool GiveWeapon(Handle me, Name weapon, int slot);
static int GiveWeapon(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Name weapon = Name(luaL_checkstring(L, 2));
	int slot = luaL_optinteger(L, 3, -1);
	if (slot >= 0)
		GiveWeapon(me, slot, weapon);
	else
		GiveWeapon(me, weapon);
	return 0;
}

//void FireAt(Handle me, Handle him = 0);
static int FireAt(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = GetHandle(L, 2);
	bool aim = luaL_optboolean(L, 3, FALSE);
	FireAt(me, him, aim);
	return 0;
}

//DLLEXPORT bool DLLAPI IsFollowing(Handle h);
//DLLEXPORT bool DLLAPI IsFollowing(Handle me, Handle him);
static int IsFollowing(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	if (Handle h2 = GetHandle(L, 2))
		lua_pushboolean(L, IsFollowing(h, h2));
	else
		lua_pushboolean(L, IsFollowing(h));
	return 1;
}

//DLLEXPORT Handle DLLAPI WhoFollowing(Handle h);
static int WhoFollowing(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	PushHandle(L, WhoFollowing(h));
	return 1;
}
//void SetUserTarget(Handle h);
static int SetUserTarget(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	SetUserTarget(h);
	return 0;
}

//Handle GetUserTarget(void);
// DLLEXPORT Handle DLLAPI GetUserTarget(int TeamNum);
static int GetUserTarget(lua_State *L)
{
	Handle h = 0;
	int team = luaL_optinteger(L, 1, -1);
	if (team >= 0)
		h = GetUserTarget(team);
	else
		h = GetUserTarget();
	PushHandle(L, h);
	return 1;
}

//void SetPerceivedTeam(Handle h, TeamNum t);
static int SetPerceivedTeam(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	TeamNum t = TeamNum(luaL_checkinteger(L, 2));
	SetPerceivedTeam(h, t);
	return 0;
}

//AiCommand GetCurrentCommand(Handle me);
static int GetCurrentCommand(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	lua_pushinteger(L, GetCurrentCommand(me));
	return 1;
}

//Handle GetCurrentWho(Handle me);
static int GetCurrentWho(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	PushHandle(L, GetCurrentWho(me));
	return 1;
}

//void EjectPilot(Handle h);
static int EjectPilot(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	EjectPilot(me);
	return 0;
}

//void HopOut(Handle h);
static int HopOut(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	HopOut(me);
	return 0;
}

//void KillPilot(Handle h);
static int KillPilot(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	KillPilot(me);
	return 0;
}

//void RemovePilotAI(Handle h);
static int RemovePilot(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	RemovePilotAI(me);
	return 0;
}

//Handle HoppedOutOf(Handle h);
static int HoppedOutOf(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	PushHandle(L, HoppedOutOf(me));
	return 0;
}

//DLLEXPORT void DLLAPI GetCameraPosition(Vector &pos, Vector &dir);
static int GetCameraPosition(lua_State *L)
{
	Vector *pos = GetVector(L, 1);
	Vector *dir = GetVector(L, 2);
	GetCameraPosition(*pos, *dir);
	return 0;
}

//DLLEXPORT void DLLAPI SetCameraPosition(const Vector &pos, const Vector &dir);
static int SetCameraPosition(lua_State *L)
{
	Vector *pos = GetVector(L, 1);
	Vector *dir = GetVector(L, 2);
	SetCameraPosition(*pos, *dir);
	return 0;
}

//DLLEXPORT void DLLAPI ResetCameraPosition();
static int ResetCameraPosition(lua_State *L)
{
	ResetCameraPosition();
	return 0;
}

//DLLEXPORT unsigned long DLLAPI CalcCRC(Name n);
static int CalcCRC(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	lua_pushinteger(L, CalcCRC(n));
	return 1;
}

//DLLEXPORT void DLLAPI IFace_Exec(Name n);
static int IFace_Exec(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	IFace_Exec(n);
	return 0;
}
//DLLEXPORT void DLLAPI IFace_Activate(Name n);
static int IFace_Activate(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	IFace_Activate(n);
	return 0;
}
//DLLEXPORT void DLLAPI IFace_Deactivate(Name n);
static int IFace_Deactivate(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	IFace_Deactivate(n);
	return 0;
}
//DLLEXPORT void DLLAPI IFace_CreateCommand(Name n);
static int IFace_CreateCommand(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	IFace_CreateCommand(n);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_CreateString(Name name, Name value);
static int IFace_CreateString(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	Name v = Name(luaL_checkstring(L, 2));
	IFace_CreateString(n, v);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_SetString(Name name, Name value);
static int IFace_SetString(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	Name v = Name(luaL_checkstring(L, 2));
	IFace_SetString(n, v);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_GetString(Name name, Name value, int maxSize);
static int IFace_GetString(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	int size = luaL_checkinteger(L, 2);
	Name v = Name(alloca(size+1));
	IFace_GetString(n, v, size);
	v[size] = '\0';
	lua_pushstring(L, v);
	return 1;
}

//DLLEXPORT void DLLAPI IFace_CreateInteger(Name name, int value);
static int IFace_CreateInteger(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	int v = luaL_checkinteger(L, 2);
	IFace_CreateInteger(n, v);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_SetInteger(Name name, int value);
static int IFace_SetInteger(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	int v = luaL_checkinteger(L, 2);
	IFace_SetInteger(n, v);
	return 0;
}

//DLLEXPORT int DLLAPI IFace_GetInteger(Name name);
static int IFace_GetInteger(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	lua_pushnumber(L, IFace_GetInteger(n));
	return 1;
}

//DLLEXPORT void DLLAPI IFace_SetIntegerRange(Name name, int low, int high);
static int IFace_SetIntegerRange(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	int min = luaL_checkinteger(L, 2);
	int max = luaL_checkinteger(L, 3);
	IFace_SetIntegerRange(n, min, max);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_CreateFloat(Name name, float value);
static int IFace_CreateFloat(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	float v = float(luaL_checknumber(L, 2));
	IFace_CreateFloat(n, v);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_SetFloat(Name name, float value);
static int IFace_SetFloat(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	float v = float(luaL_checknumber(L, 2));
	IFace_SetFloat(n, v);
	return 0;
}

//DLLEXPORT float DLLAPI IFace_GetFloat(Name name);
static int IFace_GetFloat(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	lua_pushnumber(L, IFace_GetFloat(n));
	return 1;
}

//DLLEXPORT void DLLAPI IFace_ClearListBox(Name name);
static int IFace_ClearListBox(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	IFace_ClearListBox(n);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_AddTextItem(Name name, Name value);
static int IFace_AddTextItem(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	Name v = Name(luaL_checkstring(L, 2));
	IFace_AddTextItem(n, v);
	return 0;
}

//DLLEXPORT void DLLAPI IFace_GetSelectedItem(Name name, Name value, int maxSize);
static int IFace_GetSelectedItem(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	Name v = Name(luaL_checkstring(L, 2));
	int size = luaL_checkinteger(L, 3);
	IFace_GetSelectedItem(n, v, size);
	return 0;
}

//DLLEXPORT void DLLAPI SetSkill(Handle h,int s);
static int SetSkill(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int skill = luaL_checkinteger(L, 2);
	SetSkill(me, skill);
	return 0;
}

//void SetAIP(Name n, TeamNum team = 2);
static int SetAIP(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	TeamNum team = TeamNum(luaL_optinteger(L, 2, 2));
	SetPlan(n, team);
	return 0;
}

//DLLEXPORT void DLLAPI LogFloat(float v);
static int LogFloat(lua_State *L)
{
	float v = float(luaL_checknumber(L, 1));
	LogFloat(v);
	return 0;
}

//DLLEXPORT int DLLAPI GetInstantMyForce(void);
static int GetInstantMyForce(lua_State *L)
{
	lua_pushnumber(L, GetInstantMyForce());
	return 1;
}
//DLLEXPORT int DLLAPI GetInstantCompForce(void);
static int GetInstantCompForce(lua_State *L)
{
	lua_pushnumber(L, GetInstantCompForce());
	return 1;
}
//DLLEXPORT int DLLAPI GetInstantDifficulty(void);
static int GetInstantDifficulty(lua_State *L)
{
	lua_pushnumber(L, GetInstantDifficulty());
	return 1;
}
//DLLEXPORT int DLLAPI GetInstantGoal(void);
static int GetInstantGoal(lua_State *L)
{
	lua_pushnumber(L, GetInstantGoal());
	return 1;
}
//DLLEXPORT int DLLAPI GetInstantType(void);
static int GetInstantType(lua_State *L)
{
	lua_pushnumber(L, GetInstantType());
	return 1;
}
//DLLEXPORT int DLLAPI GetInstantFlag(void);
static int GetInstantFlag(lua_State *L)
{
	lua_pushnumber(L, GetInstantFlag());
	return 1;
}
//DLLEXPORT int DLLAPI GetInstantMySide(void);
static int GetInstantMySide(lua_State *L)
{
	lua_pushnumber(L, GetInstantMySide());
	return 1;
}

/* // Function exists in ScriptUtils.h, but doesn't exist in the .cpp or bzone.lib. No such function.
//DLLEXPORT bool DLLAPI StoppedPlayback(void);
static int StoppedPlayback(lua_State *L)
{
	lua_pushboolean(L, StoppedPlayback());
	return 1;
}
*/

//bool CameraReady();
static int CameraReady(lua_State *L)
{
	lua_pushboolean(L, CameraReady());
	return 1;
}

//bool CameraPath(Name path_name,int height,int speed,Handle target_handle);
static int CameraPath(lua_State *L)
{
	Name path = Name(luaL_checkstring(L, 1));
	int height = luaL_checkinteger(L, 2);
	int speed = luaL_checkinteger(L, 3);
	Handle target = RequireHandle(L, 4);
	lua_pushboolean(L, CameraPath(path, height, speed, target));
	return 1;
}

//bool CameraPathDir(Name path_name,int height,int speed);
static int CameraPathDir(lua_State *L)
{
	Name path = Name(luaL_checkstring(L, 1));
	int height = luaL_checkinteger(L, 2);
	int speed = luaL_checkinteger(L, 3);
	lua_pushboolean(L, CameraPathDir(path, height, speed));
	return 1;
}

//bool PanDone();
static int PanDone(lua_State *L)
{
	lua_pushboolean(L, PanDone());
	return 1;
}

//bool CameraObject(Handle object_handle,float x,float y,float z,Handle target_handle);
//inline bool CameraObject(Handle object_handle, Vector pos, Handle target_handle) { return CameraObject(object_handle, pos.x, pos.y, pos.z, target_handle); }
static int CameraObject(lua_State *L)
{
	Handle object = RequireHandle(L, 1);
	if (Vector *pos = GetVector(L, 2))
	{
		Handle target = RequireHandle(L, 3);
		lua_pushboolean(L, CameraObject(object, *pos, target));
	}
	else
	{
		float x = float(luaL_checknumber(L, 2));
		float y = float(luaL_checknumber(L, 3));
		float z = float(luaL_checknumber(L, 4));
		Handle target = RequireHandle(L, 5);
		lua_pushboolean(L, CameraObject(object, x, y, z, target));
	}
	return 1;
}

//bool CameraFinish();
static int CameraFinish(lua_State *L)
{
	lua_pushboolean(L, CameraFinish());
	return 1;
}

//bool CameraCancelled(void);
static int CameraCancelled(lua_State *L)
{
	lua_pushboolean(L, CameraCancelled());
	return 1;
}

//DLLEXPORT bool DLLAPI FreeCamera(void);
static int FreeCamera(lua_State *L)
{
	lua_pushboolean(L, FreeCamera());
	return 1;
}

//DLLEXPORT bool DLLAPI FreeFinish(void);
static int FreeFinish(lua_State *L)
{
	lua_pushboolean(L, FreeFinish());
	return 1;
}

//DLLEXPORT bool DLLAPI PlayMovie(char name[20]);
static int PlayMovie(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	lua_pushboolean(L, PlayMovie(n));
	return 1;
}

//DLLEXPORT void DLLAPI StopMovie(void);
static int StopMovie(lua_State *L)
{
	StopMovie();
	return 0;
}

//DLLEXPORT bool DLLAPI PlayMove(void);
static int PlayMove(lua_State *L)
{
	PlayMove();
	return 0;
}

//DLLEXPORT bool DLLAPI PlayRecording(char name[20]);
//DLLEXPORT bool DLLAPI PlayRecording(char name[20], bool updateCam);
static int PlayRecording(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	if(lua_isboolean(L, 2))
		lua_pushboolean(L, PlayRecording(n, lua_toboolean(L, 2)));
	else
		lua_pushboolean(L, PlayRecording(n));
	return 1;
}

//DLLEXPORT bool DLLAPI PlaybackVehicle(char name[20]);
static int PlaybackVehicle(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	lua_pushboolean(L, PlaybackVehicle(n));
	return 1;
}

//DLLEXPORT float DLLAPI SetAnimation(Handle h, Name n, int animType = 0);
static int SetAnimation(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Name n = Name(luaL_checkstring(L, 2));
	int animtype = luaL_optinteger(L, 3, 0);
	lua_pushnumber(L, SetAnimation(h, n, animtype));
	return 1;
}

//DLLEXPORT float DLLAPI GetAnimationFrame(Handle h, Name n);
static int GetAnimationFrame(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Name n = Name(luaL_checkstring(L, 2));
	lua_pushnumber(L, GetAnimationFrame(h, n));
	return 1;
}

//DLLEXPORT void DLLAPI StartAnimation(Handle h);
static int StartAnimation(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	StartAnimation(h);
	return 0;
}

//DLLEXPORT void DLLAPI MaskEmitter(Handle h, DWORD mask);
static int MaskEmitter(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	DWORD mask = DWORD(luaL_checkinteger(L, 2));
	MaskEmitter(h, mask);
	return 0;
}

//DLLEXPORT void DLLAPI StartEmitter(Handle h, int slot); // slot starts at 1, incruments.
static int StartEmitter(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int slot = luaL_checkinteger(L, 2);
	StartEmitter(h, slot);
	return 0;
}

//DLLEXPORT void DLLAPI StopEmitter(Handle h, int slot);
static int StopEmitter(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int slot = luaL_checkinteger(L, 2);
	StopEmitter(h, slot);
	return 0;
}

/* // Depreciated functions.
//DLLEXPORT void DLLAPI SaveObjects(char* &buffer, unsigned long &size);
static int SaveObjects(lua_State *L)
{
	return 0;
}

//DLLEXPORT void DLLAPI LoadObjects(char* buffer, unsigned long size);
static int LoadObjects(lua_State *L)
{
	return 0;
}

//DLLEXPORT void DLLAPI IgnoreSync(bool on);
static int IgnoreSync(lua_State *L)
{
	bool on = lua_toboolean(L, 1);
	IgnoreSync(on);
	return 0;
}

//DLLEXPORT bool DLLAPI IsRecording(void);
static int IsRecording(lua_State *L)
{
	lua_pushboolean(L, IsRecording());
	return 1;
}
*/

//void SetObjectiveOn(Handle h);
static int SetObjectiveOn(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	SetObjectiveOn(h);
	return 0;
}

//void SetObjectiveOff(Handle h);
static int SetObjectiveOff(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	SetObjectiveOff(h);
	return 0;
}

//void SetObjectiveName(Handle h, Name n);
static int SetObjectiveName(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Name n = Name(luaL_checkstring(L, 2));
	SetObjectiveName(h, n);
	return 0;
}

//void ClearObjectives(void);
static int ClearObjectives(lua_State *L)
{
	ClearObjectives();
	return 0;
}

// GetColor functions, retrieve color from a specified color string. Added all the BZ1 colors for backwards compatability.
static long GetColor(const char * const colorname)
{
	switch(Hash(colorname))
	{
	case 0x568f4ba4 /* "BLACK" */: return BLACK;
	case 0x3657edc3 /* "DKGREY" */: return DKGREY;
	case 0xb29019a6 /* "GREY" */: return GREY;
	default:
	case 0xde020766 /* "WHITE" */: return WHITE;
	case 0x82fbf5cd /* "BLUE" */: return BLUE;
	case 0x3450d0d4 /* "DKBLUE" */: return DKBLUE;
	case 0x011decbc /* "GREEN" */: return GREEN;
	case 0x5960cc5b /* "DKGREEN" */: return DKGREEN;
	case 0x05bf6449 /* "YELLOW" */: return YELLOW;
	case 0x7de4f740 /* "DKYELLOW" */: return DKYELLOW;
	case 0x40f480dc /* "RED" */: return RED;
	case 0x23dbfbcf /* "DKRED" */: return DKRED;
	// BZC additions.
	case 0x9a6e02ff /* "PURPLE" */: return PURPLE;
	case 0x4961533a /* "CYAN" */: return CYAN;
	case 0xd8e26163 /* "ALLYBLUE" */: return ALLYBLUE;
	case 0x32475d3a /* "LAVACOLOR" */: return LAVACOLOR;
	}
}

// AddObjective(char *name, long color, float showTime = 8.0f)
static int AddObjective(lua_State *L)
{
	char *name = const_cast<char *>(luaL_checkstring(L, 1));
	unsigned long color = WHITE;

	if (lua_isstring(L, 2))
		color = GetColor(luaL_optstring(L, 2, "WHITE"));
	else if (lua_isnumber(L, 2))
		color = unsigned long(luaL_checklong(L, 2));

	float showtime = float(luaL_optnumber(L, 3, 8.0f));

	AddObjective(name, color, showtime);

	// No results
	return 0;
}

//bool IsWithin(Handle &h1, Handle &h2, Distance d);
static int IsWithin(lua_State *L)
{
	Handle h1 = RequireHandle(L, 1);
	Handle h2 = RequireHandle(L, 2);
	Dist d = Dist(luaL_checknumber(L, 3));
	lua_pushboolean(L, IsWithin(h1, h2, d));
	return 1;
}

//int CountUnitsNearObject(Handle h, Distance d, TeamNum t, char *odf);
static int CountUnitsNearObject(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Dist d = Dist(luaL_checknumber(L, 2));
	TeamNum t = TeamNum(luaL_checkinteger(L, 3));
	char *odf = const_cast<char *>(luaL_checkstring(L, 4));
	lua_pushinteger(L, CountUnitsNearObject(h, d, t, odf));
	return 1;
}

//DLLEXPORT void DLLAPI SetAvoidType(Handle h, int avoidType);
static int SetAvoidType(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int avoidType = luaL_checkinteger(L, 2);
	SetAvoidType(h, avoidType);
	return 0;
}

//DLLEXPORT void DLLAPI Annoy(Handle me, Handle him);
static int Annoy(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	Handle him = RequireHandle(L, 2);
	Annoy(me, him);
	return 0;
}

//DLLEXPORT void DLLAPI ClearThrust(Handle h);
static int ClearThrust(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	ClearThrust(h);
	return 0;
}

//DLLEXPORT void DLLAPI SetVerbose(Handle h, bool on);
static int SetVerbose(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	bool on = lua_toboolean(L, 2);
	SetVerbose(h, on);
	return 0;
}

//DLLEXPORT void DLLAPI ClearIdleAnims(Handle h);
static int ClearIdleAnims(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	ClearIdleAnims(h);
	return 0;
}

//DLLEXPORT void DLLAPI AddIdleAnim(Handle h, Name anim);
static int AddIdleAnim(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Name n = Name(luaL_checkstring(L, 2));
	AddIdleAnim(h, n);
	return 0;
}

//DLLEXPORT bool DLLAPI IsIdle(Handle h);
static int IsIdle(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsIdle(h));
	return 1;
}

//DLLEXPORT void DLLAPI CountThreats(Handle h, int &here, int &coming);
static int CountThreats(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int here = 0, coming = 0;
	CountThreats(h, here, coming);
	lua_pushinteger(L, here);
	lua_pushinteger(L, coming);
	return 2;
}

//DLLEXPORT void DLLAPI SpawnBirds(int group, int count, Name odf, TeamNum t, Name path);
//DLLEXPORT void DLLAPI SpawnBirds(int group, int count, Name odf, TeamNum t, Handle startObj, Handle destObj);
static int SpawnBirds(lua_State *L)
{
	int group = luaL_checkinteger(L, 1);
	int count = luaL_checkinteger(L, 2);
	Name odf = Name(luaL_checkstring(L, 3));
	int team = luaL_checkinteger(L, 4);
	if (lua_isstring(L, 5))
	{
		Name path = Name(lua_tostring(L, 5));
		SpawnBirds(group, count, odf, team, path);
	}
	else
	{
		Handle startobj = RequireHandle(L, 5);
		Handle destobj = RequireHandle(L, 6);
		SpawnBirds(group, count, odf, team, startobj, destobj);
	}
	return 0;
}

//DLLEXPORT void DLLAPI RemoveBirds(int group);
static int RemoveBirds(lua_State *L)
{
	int group = luaL_checkinteger(L, 1);
	RemoveBirds(group);
	return 0;
}

//DLLEXPORT void DLLAPI SetColorFade(float ratio, float rate, unsigned long color);
static int SetColorFade(lua_State *L)
{
	float ratio = float(luaL_checknumber(L, 2));
	float fade = float(luaL_checknumber(L, 2));
	unsigned long color = unsigned long(luaL_checklong(L, 3));
	SetColorFade(ratio, fade, color);
	return 0;
}

//DLLEXPORT void DLLAPI StopCheats(void);
static int StopCheats(lua_State *L)
{
	StopCheats();
	return 0;
}

//DLLEXPORT void DLLAPI CalcCliffs(Handle h1, Handle h2, float radius);
//DLLEXPORT void DLLAPI CalcCliffs(Name path);
static int CalcCliffs(lua_State *L)
{
	if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		CalcCliffs(path);
	}
	else
	{
		Handle h1 = RequireHandle(L, 1);
		Handle h2 = RequireHandle(L, 2);
		float radius = float(luaL_checknumber(L, 3));
		CalcCliffs(h1, h2, radius);
	}
	return 0;
}

//DLLEXPORT int DLLAPI StartSoundEffect(const char* file, Handle h = 0);
static int StartSoundEffect(lua_State *L)
{
	const char *file = luaL_checkstring(L, 1);
	Handle h = GetHandle(L, 2);
	lua_pushinteger(L, StartSoundEffect(file, h));
	return 1;
}

//DLLEXPORT int DLLAPI FindSoundEffect(const char* file, Handle h = 0);
static int FindSoundEffect(lua_State *L)
{
	const char *file = luaL_checkstring(L, 1);
	Handle h = GetHandle(L, 2);
	lua_pushinteger(L, FindSoundEffect(file, h));
	return 1;
}

//DLLEXPORT void DLLAPI StopSoundEffect(int sound);
static int StopSoundEffect(lua_State *L)
{
	int sound = luaL_checkinteger(L, 1);
	StopSoundEffect(sound);
	return 0;
}

//DLLEXPORT Handle DLLAPI GetObjectByTeamSlot(int TeamNum, int Slot);
static int GetObjectByTeamSlot(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	int slot = luaL_checkinteger(L, 2);
	PushHandle(L, GetObjectByTeamSlot(team, slot));
	return 1;
}

//bool IsNetworkOn(); // Was bool Net_IsNetGame() in BZ1.
static int IsNetworkOn(lua_State *L)
{
	lua_pushboolean(L, IsNetworkOn());
	return 1;
}

//bool ImServer(); // Was bool Net_IsHosting() in BZ1.
static int ImServer(lua_State *L)
{
	lua_pushboolean(L, ImServer());
	return 1;
}

//DLLEXPORT bool DLLAPI ImDedicatedServer(void);
static int ImDedicatedServer(lua_State *L)
{
	lua_pushboolean(L, ImDedicatedServer());
	return 1;
}

//DLLEXPORT bool DLLAPI IsTeamplayOn(void);
static int IsTeamplayOn(lua_State *L)
{
	lua_pushboolean(L, IsTeamplayOn());
	return 1;
}

//DLLEXPORT int DLLAPI CountPlayers(void);
static int CountPlayers(lua_State *L)
{
	lua_pushinteger(L, CountPlayers());
	return 1;
}

//DLLEXPORT char DLLAPI GetRaceOfTeam(int TeamNum);
static int GetRaceOfTeam(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	char race = GetRaceOfTeam(team);
	lua_pushlstring(L, &race, sizeof(char));
	return 1;
}

//DLLEXPORT bool DLLAPI IsPlayer(Handle h);
static int IsPlayer(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsPlayer(h));
	return 1;
}

//DLLEXPORT const char*  DLLAPI GetPlayerName(Handle h);
static int GetPlayerName(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	const char *Name = GetPlayerName(h);
	lua_pushstring(L, Name);
	return 1;
}

//DLLEXPORT int DLLAPI WhichTeamGroup(int Team);
static int WhichTeamGroup(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	lua_pushinteger(L, WhichTeamGroup(team));
	return 1;
}

//DLLEXPORT int DLLAPI CountAllies(int Team);
static int CountAllies(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	lua_pushinteger(L, CountAllies(team));
	return 1;
}

//DLLEXPORT int DLLAPI GetCommanderTeam(int Team);
static int GetCommanderTeam(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	lua_pushinteger(L, GetCommanderTeam(team));
	return 1;
}

//DLLEXPORT int DLLAPI GetFirstAlliedTeam(int Team);
static int GetFirstAlliedTeam(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	lua_pushinteger(L, GetFirstAlliedTeam(team));
	return 1;
}

//DLLEXPORT int DLLAPI GetLastAlliedTeam(int Team);
static int GetLastAlliedTeam(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	lua_pushinteger(L, GetLastAlliedTeam(team));
	return 1;
}

//DLLEXPORT void DLLAPI GetTeamplayRanges(int WhichTeam,int &DefenseTeamNum,int &OffenseMinTeamNum,int &OffenseMaxTeamNum);
static int GetTeamplayRanges(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	int DefenseTeamNum = luaL_checkinteger(L, 2);
	int OffenseMinTeamNum = luaL_checkinteger(L, 3);
	int OffenseMaxTeamNum = luaL_checkinteger(L, 4);
	GetTeamplayRanges(team, DefenseTeamNum, OffenseMinTeamNum, OffenseMaxTeamNum);
	lua_pushnumber(L, DefenseTeamNum);
	lua_pushnumber(L, OffenseMinTeamNum);
	lua_pushnumber(L, OffenseMaxTeamNum);
	return 3;
}

//DLLEXPORT void DLLAPI SetRandomHeadingAngle(Handle h);
static int SetRandomHeadingAngle(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	SetRandomHeadingAngle(h);
	return 0;
}

//DLLEXPORT void DLLAPI ClearTeamColors(void); 
static int ClearTeamColors(lua_State *L)
{
	ClearTeamColors();
	return 0;
}

//DLLEXPORT void DLLAPI DefaultTeamColors(void); // For FFA/DM, uses colors sent from server
static int DefaultTeamColors(lua_State *L)
{
	DefaultTeamColors();
	return 0;
}

//DLLEXPORT void DLLAPI TeamplayTeamColors(void); // For MPI/Team strat, colors sent from server
static int TeamplayTeamColors(lua_State *L)
{
	TeamplayTeamColors();
	return 0;
}

//DLLEXPORT void DLLAPI SetTeamColor(int team, int red, int green, int blue);
static int SetTeamColor(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);

	if (Vector *color = GetVector(L, 2))
	{
		SetTeamColor(team, *color);
	}
	else
	{
		int r = luaL_checkinteger(L, 2);
		int g = luaL_checkinteger(L, 3);
		int b = luaL_checkinteger(L, 4);
		SetTeamColor(team, r, g, b);
	}
	return 0;
}

//DLLEXPORT void DLLAPI ClearTeamColor(int team);
static int ClearTeamColor(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	ClearTeamColor(team);
	return 0;
}

//DLLEXPORT void DLLAPI MakeInert(Handle h);
static int MakeInert(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	MakeInert(h);
	return 0;
}

//Vector GetPositionNear(const Vector &CenterPos, const float MinRadiusAway, const float MaxRadiusAway)
//inline Vector GetPositionNear(Handle h, float MinRadiusAway, float MaxRadiusAway) { return GetPositionNear(GetPosition(h), MinRadiusAway, MaxRadiusAway); }
//inline Vector GetPositionNear(Matrix m, float MinRadiusAway, float MaxRadiusAway) { return GetPositionNear(m.posit, MinRadiusAway, MaxRadiusAway); }
//inline Vector GetPositionNear(Name Path, int Point, float MinRadiusAway, float MaxRadiusAway) { return GetPositionNear(GetVectorFromPath(Path, Point), MinRadiusAway, MaxRadiusAway); }
static int GetPositionNear(lua_State *L)
{
	float minradius = float(luaL_optnumber(L, 2, 0.0f));
	float maxradius = float(luaL_optnumber(L, 3, 0.0f));

	if (Matrix *mat = GetMatrix(L, 1))
	{
		*NewVector(L) = GetPositionNear(*mat, minradius, maxradius);
		return 1;
	}
	else if (Vector *pos = GetVector(L, 1))
	{
		*NewVector(L) = GetPositionNear(*pos, minradius, maxradius);
		return 1;
	}
	else if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		minradius = float(luaL_optnumber(L, 3, 0.0f));
		maxradius = float(luaL_optnumber(L, 4, 0.0f));
		*NewVector(L) = GetPositionNear(path, point, minradius, maxradius);
		return 1;
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		*NewVector(L) = GetPositionNear(h, minradius, maxradius);
		return 1;
	}
}

//DLLEXPORT char*  DLLAPI GetPlayerODF(int TeamNum, RandomizeType RType = Randomize_None);
static int GetPlayerODF(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	RandomizeType rtype = RandomizeType(luaL_optinteger(L, 2, 0));
	char *odf = GetPlayerODF(team, rtype);
	lua_pushstring(L, odf);
	return 1;
}

//DLLEXPORT Handle DLLAPI BuildEmptyCraftNear(Handle h, char* ODF, int Team, float MinRadiusAway, float MaxRadiusAway);
static int BuildEmptyCraftNear(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	const char *odf = luaL_checkstring(L, 2);
	int team = luaL_checkinteger(L, 3);
	float min = float(luaL_checknumber(L, 4));
	float max = float(luaL_checknumber(L, 5));
	PushHandle(L, BuildEmptyCraftNear(h, const_cast<char *>(odf), team, min, max));
	return 1;
}

//Vector GetCircularPos(const Vector &CenterPos, const float Radius, const float Angle)
static int GetCircularPos(lua_State *L)
{
	if (Vector *pos = GetVector(L, 1))
	{
		const float radius = float(luaL_optnumber(L, 2, 0.0f));
		const float angle = float(luaL_optnumber(L, 3, 0.0f));
		*NewVector(L) = GetCircularPos(*pos, radius, angle);
		return 1;
	}
	return 0;
}

//DLLEXPORT Vector DLLAPI GetSafestSpawnpoint(void);
static int GetSafestSpawnpoint(lua_State *L)
{
	*NewVector(L) = GetSafestSpawnpoint();
	return 1;
}

//DLLEXPORT Vector DLLAPI GetSpawnpoint(int TeamNum);
static int GetSpawnpoint(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	*NewVector(L) = GetSpawnpoint(team);
	return 1;
}

//DLLEXPORT Handle DLLAPI GetSpawnpointHandle(int TeamNum);
static int GetSpawnpointHandle(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	PushHandle(L, GetSpawnpointHandle(team));
	return 1;
}

//DLLEXPORT Vector DLLAPI GetRandomSpawnpoint(int TeamNum = -1);
static int GetRandomSpawnpoint(lua_State *L)
{
	int team = luaL_optinteger(L, 1, -1);
	*NewVector(L) = GetRandomSpawnpoint(team);
	return 1;
}

//DLLEXPORT void DLLAPI SetTimerBox(const char* message);
static int SetTimerBox(lua_State *L)
{
	char *msg = const_cast<char *>(luaL_checkstring(L, 1));
	SetTimerBox(msg);
	return 0;
}

//DLLEXPORT void DLLAPI AddToMessagesBox(const char* message);
//DLLEXPORT void DLLAPI AddToMessagesBox2(const char* message, unsigned long color);
static int AddToMessagesBox(lua_State *L)
{
	char *msg = const_cast<char *>(luaL_checkstring(L, 1));
	if (lua_isnumber(L, 2))
	{
		unsigned long color = unsigned long(lua_tointeger(L, 2));
		AddToMessagesBox2(msg, color);
	}
	else if (lua_isstring(L, 2))
	{
		long color = GetColor(lua_tostring(L, 2));
		AddToMessagesBox2(msg, color);
	}
	else
	{
		AddToMessagesBox(msg);
	}
	return 0;
}

//DLLEXPORT int DLLAPI GetDeaths(Handle h);
static int GetDeaths(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetDeaths(h));
	return 1;
}

//DLLEXPORT int DLLAPI GetKills(Handle h);
static int GetKills(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetKills(h));
	return 1;
}

//DLLEXPORT int DLLAPI GetScore(Handle h);
static int GetScore(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetScore(h));
	return 1;
}

//DLLEXPORT void DLLAPI SetDeaths(Handle h, const int NewValue);
static int SetDeaths(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int value = luaL_checkinteger(L, 2);
	SetDeaths(h, value);
	return 0;
}

//DLLEXPORT void DLLAPI SetKills(Handle h, const int NewValue);
static int SetKills(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int value = luaL_checkinteger(L, 2);
	SetKills(h, value);
	return 0;
}

//DLLEXPORT void DLLAPI SetScore(Handle h, const int NewValue);
static int SetScore(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int value = luaL_checkinteger(L, 2);
	SetScore(h, value);
	return 0;
}

//DLLEXPORT void DLLAPI AddDeaths(Handle h, const int DeltaValue);
static int AddDeaths(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int value = luaL_checkinteger(L, 2);
	AddDeaths(h, value);
	return 0;
}

//DLLEXPORT void DLLAPI AddKills(Handle h, const int DeltaValue);
static int AddKills(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int value = luaL_checkinteger(L, 2);
	AddKills(h, value);
	return 0;
}

//DLLEXPORT void DLLAPI AddScore(Handle h, const int DeltaValue);
static int AddScore(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int value = luaL_checkinteger(L, 2);
	AddScore(h, value);
	return 0;
}

//DLLEXPORT void DLLAPI SetAsUser(Handle h, int Team);
static int SetAsUser(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int team = luaL_checkinteger(L, 2);
	SetAsUser(h, team);
	return 0;
}

//DLLEXPORT void DLLAPI SetNoScrapFlagByHandle(Handle h);
static int SetNoScrapFlagByHandle(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	SetNoScrapFlagByHandle(h);
	return 0;
}

//DLLEXPORT void DLLAPI ClearNoScrapFlagByHandle(Handle h);
static int ClearNoScrapFlagByHandle(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	ClearNoScrapFlagByHandle(h);
	return 0;
}

//DLLEXPORT int DLLAPI GetLocalPlayerTeamNumber(void);
static int GetLocalPlayerTeamNumber(lua_State *L)
{
	lua_pushinteger(L, GetLocalPlayerTeamNumber());
	return 1;
}

//DLLEXPORT DPID DLLAPI GetLocalPlayerDPID(void);
static int GetLocalPlayerDPID(lua_State *L)
{
	lua_pushinteger(L, GetLocalPlayerDPID());
	return 1;
}

//DLLEXPORT void DLLAPI FlagSteal(Handle flag, Handle holder);
static int FlagSteal(lua_State *L)
{
	Handle flag = RequireHandle(L, 1);
	Handle holder = RequireHandle(L, 2);
	FlagSteal(flag, holder);
	return 0;
}

//DLLEXPORT void DLLAPI FlagRecover(Handle flag, Handle holder);
static int FlagRecover(lua_State *L)
{
	Handle flag = RequireHandle(L, 1);
	Handle holder = RequireHandle(L, 2);
	FlagRecover(flag, holder);
	return 0;
}

//DLLEXPORT void DLLAPI FlagScore(Handle flag, Handle holder);
static int FlagScore(lua_State *L)
{
	Handle flag = RequireHandle(L, 1);
	Handle holder = RequireHandle(L, 2);
	FlagScore(flag, holder);
	return 0;
}

//DLLEXPORT void DLLAPI MoneyScore(int amount, Handle bagman);
static int MoneyScore(lua_State *L)
{
	int amount = luaL_checkinteger(L, 1);
	Handle bagman = RequireHandle(L, 2);
	MoneyScore(amount, bagman);
	return 0;
}

//DLLEXPORT void DLLAPI NoteGameoverByTimelimit(void);
static int NoteGameoverByTimelimit(lua_State *L)
{
	NoteGameoverByTimelimit();
	return 0;
}

//DLLEXPORT void DLLAPI NoteGameoverByKillLimit(Handle h);
static int NoteGameoverByKillLimit(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	NoteGameoverByKillLimit(h);
	return 0;
}

//DLLEXPORT void DLLAPI NoteGameoverByScore(Handle h);
static int NoteGameoverByScore(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	NoteGameoverByScore(h);
	return 0;
}

//DLLEXPORT void DLLAPI NoteGameoverByLastWithBase(Handle h);
static int NoteGameoverByLastWithBase(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	NoteGameoverByLastWithBase(h);
	return 0;
}

//DLLEXPORT void DLLAPI NoteGameoverByLastTeamWithBase(int Teamgroup);
static int NoteGameoverByLastTeamWithBase(lua_State *L)
{
	int teamgroup = luaL_checkinteger(L, 1);
	NoteGameoverByLastTeamWithBase(teamgroup);
	return 0;
}

//DLLEXPORT void DLLAPI NoteGameoverByNoBases(void);
static int NoteGameoverByNoBases(lua_State *L)
{
	NoteGameoverByNoBases();
	return 0;
}

//DLLEXPORT void DLLAPI DoGameover(float SecondsFromNow);
static int DoGameover(lua_State *L)
{
	float seconds = float(luaL_checknumber(L, 1));
	DoGameover(seconds);
	return 0;
}

//DLLEXPORT void DLLAPI SetMPTeamRace(int WhichTeamGroup, char RaceIdentifier);
static int SetMPTeamRace(lua_State *L)
{
	int teamgroup = luaL_checkinteger(L, 1);
	if (const char *race = luaL_checkstring(L, 2))
	{
		SetMPTeamRace(teamgroup, *race);
	}
	return 0;
}

//Handle GetTarget(Handle h);
static int GetTarget(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Handle t = GetTarget(h);
	PushHandle(L, t);
	return 1;
}

//DLLEXPORT void DLLAPI IFace_ConsoleCmd(const char* pStr, bool bSquelchOutput = true);
static int IFace_ConsoleCmd(lua_State *L)
{
	char *msg = const_cast<char *>(luaL_checkstring(L, 1));
	bool squelch = luaL_optboolean(L, 2, TRUE);
	IFace_ConsoleCmd(msg, squelch);
	return 0;
}

//DLLEXPORT void DLLAPI Network_SetString(Name name, Name value);
static int Network_SetString(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	Name v = Name(luaL_checkstring(L, 2));
	Network_SetString(n, v);
	return 0;
}

//DLLEXPORT void DLLAPI Network_SetInteger(Name name, int value);
static int Network_SetInteger(lua_State *L)
{
	Name n = Name(luaL_checkstring(L, 1));
	int v = luaL_checkinteger(L, 2);
	Network_SetInteger(n, v);
	return 0;
}

//Vector GetVelocity(Handle h);
static int GetVelocity(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	*NewVector(L) = GetVelocity(h);
	return 1;
}

//DLLEXPORT bool DLLAPI GetObjInfo(Handle h, ObjectInfoType type, char pBuffer[ 64 ]);
/*
	Get_CFG, // Returns the GameObjectClass's cfg string
	Get_ODF, // Returns the ODF of the object
	Get_GOClass_gCfg, // Returns the GameObjectClass's gCfg string (not 100% sure how it differs from the CFG) (It returns the ODF's BaseName. Even reads through ODF inheritence. :) ) -GBD
	Get_EntityType,
	Get_GOClass, 

	Get_Weapon0Config, 
	Get_Weapon1Config,
	Get_Weapon2Config,
	Get_Weapon3Config,
	Get_Weapon4Config,

	Get_Weapon0ODF, 
	Get_Weapon1ODF,
	Get_Weapon2ODF,
	Get_Weapon3ODF,
	Get_Weapon4ODF,

	Get_Weapon0GOClass, 
	Get_Weapon1GOClass,
	Get_Weapon2GOClass,
	Get_Weapon3GOClass,
	Get_Weapon4GOClass,
*/
//Get_CFG
static int GetObjInfo_CFG(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, Get_CFG, buffer);
	if(h)
		lua_pushstring(L, buffer);
	else
		lua_pushnil(L);
	return 1;
}
	
//Get_ODF //char *GetOdf(Handle h)
static int GetObjInfo_ODF(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, Get_ODF, buffer);
	if(h)
		lua_pushstring(L, buffer);
	else
		lua_pushnil(L);
	return 1;
}

//Get_GOClass_gCfg // Was char *GetBase(Handle h) in BZ1?
static int GetObjInfo_GOClass_gCfg(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, Get_GOClass_gCfg, buffer);
	if(h)
		lua_pushstring(L, buffer);
	else
		lua_pushnil(L);
	return 1;
}

//Get_EntityType // Was char *GetClassSig(Handle h) in BZ1?
static int GetObjInfo_EntityType(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, Get_EntityType, buffer);
	if(h)
		lua_pushstring(L, buffer);
	else
		lua_pushnil(L);
	return 1;
}

//Get_GOClass // Was char *GetClassLabel(Handle h) in BZ1?
static int GetObjInfo_GOClass(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, Get_GOClass, buffer);
	if(h)
		lua_pushstring(L, buffer);
	else
		lua_pushnil(L);
	return 1;
}

//Get_Weapon0Config, 
static int Get_WeaponConfig(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	if (h == 0)
		return 0;

	int slot = luaL_optinteger(L, 2, 0);
	if (slot < 0 || slot > 4)
	return 0;

	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, ObjectInfoType(Get_Weapon0Config + slot), buffer);
	lua_pushstring(L, buffer);

	return 1;
}

//Get_Weapon0ODF, 
static int Get_WeaponODF(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	if (h == 0)
		return 0;

	int slot = luaL_optinteger(L, 2, 0);
	if (slot < 0 || slot > 4)
	return 0;

	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, ObjectInfoType(Get_Weapon0ODF + slot), buffer);
	lua_pushstring(L, buffer);

	return 1;
}

//Get_Weapon0GOClass, 
static int Get_WeaponGOClass(lua_State *L)
{
	Handle h = GetHandle(L, 1);
	if (h == 0)
		return 0;

	int slot = luaL_optinteger(L, 2, 0);
	if (slot < 0 || slot > 4)
	return 0;

	char buffer[MAX_ODF_LENGTH] = {0};
	GetObjInfo(h, ObjectInfoType(Get_Weapon0GOClass + slot), buffer);
	lua_pushstring(L, buffer);

	return 1;
}

//DLLEXPORT bool DLLAPI DoesODFExist(char* odf);
static int DoesODFExist(lua_State *L)
{
	char *odf = const_cast<char *>(luaL_checkstring(L, 1));
	lua_pushboolean(L, DoesODFExist(odf));
	return 1;
}

//DLLEXPORT bool DLLAPI IsAlive2(Handle h);
static int IsAlive2(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsAlive2(h));
	return 1;
}

//DLLEXPORT bool DLLAPI IsFlying2(Handle h);
static int IsFlying2(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsFlying2(h));
	return 1;
}

//DLLEXPORT bool DLLAPI IsAliveAndPilot2(Handle h);
static int IsAliveAndPilot2(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsAliveAndPilot2(h));
	return 1;
}

//DLLEXPORT void DLLAPI TranslateString2(Name Dst, size_t bufsize, Name Src);
static int TranslateString2(lua_State *L)
{
	const char *src = luaL_checkstring(L, 1);
	size_t size = size_t(luaL_checkinteger(L, 2));
	char *dst = static_cast<char *>(alloca(size+1));
	TranslateString2(dst, size, const_cast<char *>(src));
	dst[size] = '\0';
	lua_pushstring(L, dst);
	return 1;
}

//DLLEXPORT int DLLAPI GetScavengerCurScrap(Handle h);
static int GetScavengerCurScrap(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetScavengerCurScrap(h));
	return 1;
}

//DLLEXPORT int DLLAPI GetScavengerMaxScrap(Handle h);
static int GetScavengerMaxScrap(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetScavengerMaxScrap(h));
	return 1;
}

//DLLEXPORT void DLLAPI SetScavengerCurScrap(Handle h, int aNewScrap);
static int SetScavengerCurScrap(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int scrap = luaL_checkinteger(L, 2);
	SetScavengerCurScrap(h, scrap);
	return 0;
}

//DLLEXPORT void DLLAPI SetScavengerMaxScrap(Handle h, int aNewScrap);
static int SetScavengerMaxScrap(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int scrap = luaL_checkinteger(L, 2);
	SetScavengerMaxScrap(h, scrap);
	return 0;
}

//DLLEXPORT void DLLAPI SelfDamage(Handle him, float amt);
static int SelfDamage(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	float amt = float(luaL_checknumber(L, 2));
	SelfDamage(h, amt);
	return 0;
}

//DLLEXPORT void DLLAPI WantBotKillMessages(void);
static int WantBotKillMessages(lua_State *L)
{
	WantBotKillMessages();
	return 0;
}

//DLLEXPORT void DLLAPI EnableHighTPS(int& newRate);
static int EnableHighTPS(lua_State *L)
{
	int tpsrate;
	EnableHighTPS(tpsrate);
	lua_pushinteger(L, tpsrate);
	return 1;
}

//DLLEXPORT Handle DLLAPI GetLocalUserInspectHandle(void);
static int GetLocalUserInspectHandle(lua_State *L)
{
	PushHandle(L, GetLocalUserInspectHandle());
	return 1;
}

//DLLEXPORT Handle DLLAPI GetLocalUserSelectHandle(void);
static int GetLocalUserSelectHandle(lua_State *L)
{
	PushHandle(L, GetLocalUserSelectHandle());
	return 1;
}

//DLLEXPORT void DLLAPI ResetTeamSlot(Handle h);
static int ResetTeamSlot(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	ResetTeamSlot(h);
	return 0;
}

//DLLEXPORT int DLLAPI GetCategoryTypeOverride(Handle h);
static int GetCategoryTypeOverride(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetCategoryTypeOverride(h));
	return 1;
}

//DLLEXPORT int DLLAPI GetCategoryType(Handle h);
static int GetCategoryType(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetCategoryType(h));
	return 1;
}

// GetODF* Read accessors. These versions recursively handle ODF inheritence, and opening/closing the files internally for optimized usage.

//DLLEXPORT int DLLAPI GetODFHexInt(const char* file, const char* block, const char* name, int* value = NULL, int defval = 0);
//extern int GetODFHexInt(Handle h, const char* block, const char* name, int* value = NULL, int defval = 0);
static int GetODFHexInt(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	int value = luaL_optinteger(L, 4, 0);

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFHexInt(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFHexInt(h, section, label, &value, value);
	}

	lua_pushinteger(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFInt(const char* file, const char* block, const char* name, int* value = NULL, int defval = 0);
//extern int GetODFInt(Handle h, const char* block, const char* name, int* value = NULL, int defval = 0);
static int GetODFInt(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	int value = luaL_optinteger(L, 4, 0);

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFInt(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFInt(h, section, label, &value, value);
	}

	lua_pushinteger(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFLong(const char* file, const char* block, const char* name, long* value = NULL, long defval = 0);
//extern int GetODFLong(Handle h, const char* block, const char* name, long* value = NULL, long defval = 0);
static int GetODFLong(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	long value = luaL_optlong(L, 4, 0);

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFLong(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFLong(h, section, label, &value, value);
	}

	lua_pushnumber(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFFloat(const char* file, const char* block, const char* name, float* value = NULL, float defval = 0.0f);
//extern int GetODFFloat(Handle h, const char* block, const char* name, float* value = NULL, float defval = 0.0f);
static int GetODFFloat(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	float value = float(luaL_optnumber(L, 4, 0.0f));

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFFloat(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFFloat(h, section, label, &value, value);
	}

	lua_pushnumber(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFDouble(const char* file, const char* block, const char* name, double* value = NULL, double defval = 0.0);
//extern int GetODFDouble(Handle h, const char* block, const char* name, double* value = NULL, double defval = 0.0);
static int GetODFDouble(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	double value = double(luaL_optnumber(L, 4, 0.0));

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFDouble(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFDouble(h, section, label, &value, value);
	}

	lua_pushnumber(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFChar(const char* file, const char* block, const char* name, char* value = NULL, char defval = '\0');
//extern int GetODFChar(Handle h, const char* block, const char* name, char* value = NULL, char defval = '\0');
static int GetODFChar(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	char value = luaL_optstring(L, 4, '\0')[0];

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFChar(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFChar(h, section, label, &value, value);
	}

	lua_pushlstring(L, &value, 1);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFBool(const char* file, const char* block, const char* name, bool* value = NULL, bool defval = false);
//extern int GetODFBool(Handle h, const char* block, const char* name, bool* value = NULL, bool defval = false);
static int GetODFBool(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	bool value = luaL_optboolean(L, 4, FALSE) != 0;

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFBool(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFBool(h, section, label, &value, value);
	}

	lua_pushboolean(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFString(const char* file, const char* block, const char* name, size_t ValueLen = 0, char* value = NULL, const char* defval = NULL);
//extern int GetODFString(Handle h, const char* block, const char* name, size_t ValueLen = 0, char* value = NULL, const char* defval = NULL);
static int GetODFString(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	const char *defval = luaL_optstring(L, 4, NULL);
	char value[MAX_ODF_LENGTH] = {0};

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFString(file, NULL, section, label, MAX_ODF_LENGTH, value, defval);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFString(h, section, label, MAX_ODF_LENGTH, value, defval);
	}

	lua_pushstring(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFColor(const char* file, const char* block, const char* name, DWORD* value = NULL, DWORD defval = 0);
//extern int GetODFColor(Handle h, const char* block, const char* name, DWORD* value = NULL, DWORD defval = 0);
static int GetODFColor(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	DWORD value = luaL_optinteger(L, 4, 0);

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFColor(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFColor(h, section, label, &value, value);
	}

	lua_pushnumber(L, value);
	lua_pushboolean(L, found);
	return 2;
}

//DLLEXPORT int DLLAPI GetODFVector(const char* file, const char* block, const char* name, Vector* value = NULL, Vector defval = Vector(0.0f, 0.0f, 0.0f));
//extern int GetODFVector(Handle h, const char* block, const char* name, Vector* value = NULL, Vector defval = Vector(0.0f, 0.0f, 0.0f));
static int GetODFVector(lua_State *L)
{
	const char *section = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	bool found = false;
	// Data Type stuff.
	const Vector *v = GetVector(L, 4);
	Vector value = v ? *v : Vector(0, 0, 0);

	if (lua_isstring(L, 1))
	{
		const char *file = lua_tostring(L, 1);
		if(OpenODF2(file))
			found = GetODFVector(file, NULL, section, label, &value, value);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		found = GetODFVector(h, section, label, &value, value);
	}

	//lua_pushboolean(L, value);
	*NewVector(L) = value;
	lua_pushboolean(L, found);
	return 2;
}

/* // Functions removed, the above functions handle it internally.
//DLLEXPORT bool DLLAPI OpenODF(char *name);
static int OpenODF(lua_State *L)
{
	const char *file = luaL_checkstring(L, 1);
	lua_pushboolean(L, OpenODF(file));
	return 1;
}

//DLLEXPORT bool DLLAPI CloseODF(char *name);
static int CloseODF(lua_State *L)
{
	const char *file = luaL_checkstring(L, 1);
	lua_pushboolean(L, CloseODF(file));
	return 1;
}
*/

//DLLEXPORT void DLLAPI NoteGameoverWithCustomMessage(const char* pString);
static int NoteGameoverWithCustomMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	NoteGameoverWithCustomMessage(msg);
	return 0;
}

//DLLEXPORT int DLLAPI SetBestGroup(Handle h);
static int SetBestGroup(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, SetBestGroup(h));
	return 1;
}

//DLLEXPORT void DLLAPI GetGroup(int team, int group, ObjectInfoType type, char pBuffer[ 64 ]);
//DLLEXPORT int DLLAPI GetGroup(Handle h);
static int GetGroup(lua_State *L)
{
	if (lua_isnumber(L, 2))
	{
		int team = luaL_checkinteger(L, 1);
		int group = lua_tointeger(L, 2);
		ObjectInfoType type = ObjectInfoType(luaL_checkinteger(L, 3));

		char pBuffer[MAX_ODF_LENGTH] = {0};
		GetGroup(team, group, type, pBuffer);
		lua_pushstring(L, pBuffer);
	}
	else
	{
		Handle h = RequireHandle(L, 1);
		lua_pushnumber(L, GetGroup(h));
	}
	return 1;
}

//DLLEXPORT int DLLAPI GetGroupCount(int team, int group);
static int GetGroupCount(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	int group = luaL_checkinteger(L, 2);
	lua_pushinteger(L, GetGroupCount(team, group));
	return 1;
}

//DLLEXPORT void DLLAPI SetLifespan(Handle h, float timeout);
static int SetLifespan(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	float timeout = float(luaL_checknumber(L, 2));
	SetLifespan(h, timeout);
	return 0;
}

//DLLEXPORT bool DLLAPI DoesFileExist(const char* filename);
static int DoesFileExist(lua_State *L)
{
	const char *file = luaL_checkstring(L, 1);
	lua_pushboolean(L, DoesFileExist(file));
	return 1;
}

//DLLEXPORT bool DLLAPI LoadFile(const char* filename, void* pData, size_t& bufSize);
static int LoadFile(lua_State *L)
{
	size_t size;
	const char* filename = luaL_checklstring(L, 1, &size);

	size_t bufSize = 0;
	char* FileBuffer = NULL;
	LoadFile(filename, NULL, bufSize);
	FileBuffer = static_cast<char *>(malloc(bufSize+1));
	LoadFile(filename, FileBuffer, bufSize);
	FileBuffer[bufSize] = '\0';

	lua_pushlstring(L, FileBuffer, bufSize);

	free(FileBuffer);

	return 1;
}

//DLLEXPORT DLLAudioHandle DLLAPI StartAudio3D(const char *const name, Handle h, DLLAudioCategory cat = AUDIO_CAT_UNKNOWN, bool loop = false, bool stopLast = false);
//DLLEXPORT DLLAudioHandle DLLAPI StartAudio3D(const char *const name, float x, float y, float z, DLLAudioCategory cat = AUDIO_CAT_UNKNOWN, bool loop = false);
//inline DLLAudioHandle StartAudio3D(const char *const name, Vector Pos, DLLAudioCategory cat = AUDIO_CAT_UNKNOWN, bool loop = false) { return StartAudio3D(name, Pos.x, Pos.y, Pos.z, cat, loop); }
//inline DLLAudioHandle StartAudio3D(const char *const name, Matrix Mat, DLLAudioCategory cat = AUDIO_CAT_UNKNOWN, bool loop = false) { return StartAudio3D(name, Mat.Posit.x, Mat.Posit.y, Mat.Posit.z, cat, loop); }
static int StartAudio3D(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);

	if(lua_isboolean(L, 4))
	{
		Handle h = RequireHandle(L, 2);
		DLLAudioCategory cat = DLLAudioCategory(luaL_checkinteger(L, 3));
		bool loop = luaL_optboolean(L, 4, FALSE);
		bool stoplast = luaL_optboolean(L, 5, FALSE);
		lua_pushnumber(L, StartAudio3D(msg, h, cat, loop, stoplast));
	}
	else if (Matrix *mat = GetMatrix(L, 2))
	{
		DLLAudioCategory cat = DLLAudioCategory(luaL_checkinteger(L, 5));
		bool loop = luaL_optboolean(L, 6, FALSE);
		lua_pushnumber(L, StartAudio3D(msg, mat->posit, cat, loop));
	}
	else if (Vector *pos = GetVector(L, 2))
	{
		DLLAudioCategory cat = DLLAudioCategory(luaL_checkinteger(L, 5));
		bool loop = luaL_optboolean(L, 6, FALSE);
		lua_pushnumber(L, StartAudio3D(msg, *pos, cat, loop));
	}
	else
	{
		float x = float(luaL_checknumber(L, 2));
		float y = float(luaL_checknumber(L, 3));
		float z = float(luaL_checknumber(L, 4));
		DLLAudioCategory cat = DLLAudioCategory(luaL_checkinteger(L, 5));
		bool loop = luaL_optboolean(L, 6, FALSE);
		lua_pushnumber(L, StartAudio3D(msg, x, y, z, cat, loop));
	}
	return 1;
}


//DLLEXPORT DLLAudioHandle DLLAPI StartAudio2D(const char *const name, float volume, float pan, float rate, bool loop = false, bool isMusic = false);
static int StartAudio2D(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	float vol = float(luaL_checknumber(L, 2));
	float pan = float(luaL_checknumber(L, 3));
	float rate = float(luaL_checknumber(L, 4));
	bool loop = luaL_optboolean(L, 5, FALSE);
	bool ismusic = luaL_optboolean(L, 6, FALSE);
	lua_pushnumber(L, StartAudio2D(msg, vol, pan, rate, loop, ismusic));
	return 1;
}

//DLLEXPORT bool DLLAPI IsAudioPlaying(DLLAudioHandle &h);
static int IsAudioPlaying(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	lua_pushboolean(L, IsAudioPlaying(h));
	return 1;
}

//DLLEXPORT void DLLAPI StopAudio(DLLAudioHandle h);
static int StopAudio(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	StopAudio(h);
	return 0;
}

//DLLEXPORT void DLLAPI PauseAudio(DLLAudioHandle h);
static int PauseAudio(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	PauseAudio(h);
	return 0;
}

//DLLEXPORT void DLLAPI ResumeAudio(DLLAudioHandle h);
static int ResumeAudio(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	ResumeAudio(h);
	return 0;
}

//DLLEXPORT void DLLAPI SetVolume(DLLAudioHandle h, float vol, bool adjustByVolumes = true);
static int SetVolume(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	float vol = float(luaL_checknumber(L, 2));
	bool adjust = luaL_optboolean(L, 3, FALSE);
	SetVolume(h, vol, adjust);
	return 0;
}

//DLLEXPORT void DLLAPI SetPan(DLLAudioHandle h, float pan);
static int SetPan(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	float pan = float(luaL_checknumber(L, 2));
	SetPan(h, pan);
	return 0;
}

//DLLEXPORT void DLLAPI SetRate(DLLAudioHandle h, float rate);
static int SetRate(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	float rate = float(luaL_checknumber(L, 2));
	SetRate(h, rate);
	return 0;
}

//DLLEXPORT float DLLAPI GetAudioFileDuration(DLLAudioHandle h);
static int GetAudioFileDuration(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	lua_pushnumber(L, GetAudioFileDuration(h));
	return 1;
}

//DLLEXPORT bool DLLAPI IsPlayingLooped(DLLAudioHandle h);
static int IsPlayingLooped(lua_State *L)
{
	DLLAudioHandle h = DLLAudioHandle(luaL_checknumber(L, 1));
	lua_pushboolean(L, IsPlayingLooped(h));
	return 1;
}

//DLLEXPORT Handle DLLAPI GetNearestPowerup(Handle h, bool ignoreSpawnpoints, float maxDist = 450.0f);
static int GetNearestPowerup(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	bool ignorespawn = lua_toboolean(L, 2);
	float maxdist = float(luaL_optnumber(L, 3, 450.0f));
	PushHandle(L, GetNearestPowerup(h, ignorespawn, maxdist));
	return 1;
}

//DLLEXPORT Handle DLLAPI GetNearestPerson(Handle h, bool skipFriendlies, float maxDist = 450.0f);
static int GetNearestPerson(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	bool skipfriendlies = lua_toboolean(L, 2);
	float maxdist = float(luaL_optnumber(L, 3, 450.0f));
	PushHandle(L, GetNearestPerson(h, skipfriendlies, maxdist));
	return 1;
}

//void SetCommand(Handle me, AiCommand what, int priority, Handle who, const Matrix &where, long param);
//void SetCommand(Handle me, AiCommand what, int priority, Handle who, const Vector &where, long param);
//void SetCommand(Handle me, AiCommand what, int priority, Handle who, Name path, long param);

//DLLEXPORT void DLLAPI SetCommand(Handle me, int cmd, int priority, Handle who, const Vector& where, int param = 0);
//DLLEXPORT void DLLAPI SetCommand(Handle me, int cmd, int priority = 0, Handle who = 0, const Name path = NULL, int param = 0);
static int SetCommand(lua_State *L)
{
	Handle me = RequireHandle(L, 1);

	AiCommand what = AiCommand(luaL_checkinteger(L, 2));
	int priority = luaL_optinteger(L, 3, 1);
	Handle who = GetHandle(L, 4);
	//float when = float(luaL_optnumber(L, 6, 0.0f)); // BZ2 has no when. :( -GBD
	long param = lua_isstring(L, 6) ? CalcCRC(Name(lua_tostring(L, 6))) : luaL_optlong(L, 6, 0);

	// set where
	if (Matrix *mat = GetMatrix(L, 5))
	{
		SetCommand(me, what, priority, who, mat->posit, param);
	}
	else if (Vector *pos = GetVector(L, 5))
	{
		SetCommand(me, what, priority, who, *pos, param);
	}
	else if (lua_isstring(L, 5))
	{
		Name path = Name(lua_tostring(L, 5));
		SetCommand(me, what, priority, who, path, param);
	}
	else
	{
		SetCommand(me, what, priority, who, NULL, param);
	}
	return 0;
}

//DLLEXPORT void DLLAPI SetGravity(float gravity = 12.5f);
static int SetGravity(lua_State *L)
{
	const float gravity = float(luaL_optnumber(L, 1, 12.5f));
	SetGravity(gravity);
	return 0;
}

//DLLEXPORT void DLLAPI SetAutoGroupUnits(bool autoGroup = true);
static int SetAutoGroupUnits(lua_State *L)
{
	bool autogroup = luaL_optboolean(L, 2, TRUE);
	SetAutoGroupUnits(autogroup);
	return 0;
}

//DLLEXPORT void DLLAPI KickPlayer(Handle h, const char* pExplanationStr = "", bool banAlso = false);
static int KickPlayer(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	const char *Explination_THIS_IS_SPARTA = luaL_checkstring(L, 2);
	bool BANHAMMER = luaL_optboolean(L, 3, FALSE);
	KickPlayer(h, Explination_THIS_IS_SPARTA, BANHAMMER);
	return 0;
}

//DLLEXPORT bool DLLAPI TerrainIsWater(const Vector &pos);
//DLLEXPORT bool DLLAPI TerrainIsWater(float x, float z);
//inline bool TerrainIsWater(Matrix mat) { return TerrainIsWater(mat.posit); }
//inline bool TerrainIsWater(Name Path, int Point = 0) { return TerrainIsWater(GetVectorFromPath(Path, Point)); }
//inline bool TerrainIsWater(Handle h) { return TerrainIsWater(GetPosition(h)); }
static int TerrainIsWater(lua_State *L)
{
	if (Matrix *mat = GetMatrix(L, 1))
	{
		lua_pushboolean(L, TerrainIsWater(*mat));
	}
	else if (Vector *pos = GetVector(L, 1))
	{
		lua_pushboolean(L, TerrainIsWater(*pos));
	}
	else if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 3));
		int point = luaL_optinteger(L, 4, 0);
		lua_pushboolean(L, TerrainIsWater(path, point));
	}
	else if (Handle h = GetHandle(L, 1))
	{
		lua_pushboolean(L, TerrainIsWater(h));
	}
	else
	{
		float x = float(luaL_checknumber(L, 1));
		float z = float(luaL_checknumber(L, 2));
		lua_pushboolean(L, TerrainIsWater(x, z));
	}
	return 1;
}

//DLLEXPORT bool DLLAPI TerrainGetHeightAndNormal(const Vector& pos, float& outHeight, Vector& outNormal, bool useWater);
static int GetTerrainHeightAndNormal(lua_State *L)
{
	float height;
	Vector normal;
	bool usewater = false;
	if (lua_isstring(L, 1))
		usewater = luaL_optboolean(L, 3, FALSE);
	else
		usewater = luaL_optboolean(L, 2, FALSE);

	if (Matrix *mat = GetMatrix(L, 1))
	{
		TerrainGetHeightAndNormal(*mat, height, normal, usewater);
	}
	else if (Vector *vec = GetVector(L, 1))
	{
		TerrainGetHeightAndNormal(*vec, height, normal, usewater);
	}
	else if (lua_isstring(L, 1))
	{
		Name path = Name(lua_tostring(L, 1));
		int point = luaL_optinteger(L, 2, 0);
		TerrainGetHeightAndNormal(path, point, height, normal, usewater);	}
	else
	{
		Handle h = RequireHandle(L, 1);
		TerrainGetHeightAndNormal(h, height, normal, usewater);
	}

	lua_pushnumber(L, height);
	*NewVector(L) = normal;
	return 2;
}

// WriteToFile does this internally. No need for users to do it.
//DLLEXPORT bool DLLAPI GetOutputPath(size_t& bufSize, wchar_t* pData);

// A safe way to write a file with Lua, for custom logs or saved scores.
// extern void WriteToFile(lua_string filename, lua_string towrite, bool append = true)
static int WriteToFile(lua_State *L)
{
	size_t fileNameSize = 0;
	const char* filename = luaL_checklstring(L, 1, &fileNameSize);

	size_t writeSize = 0;
	const char* writeStr = luaL_checklstring(L, 2, &writeSize);

	bool append = luaL_optboolean(L, 3, TRUE);

	lua_pushboolean(L, FormatFileMessage(filename, append, writeStr));
	return 1;
}

//DLLEXPORT bool DLLAPI GetPathPoints(Name path, size_t& bufSize, float* pData);
static int GetPathPoints(lua_State *L)
{
	Name path = Name(luaL_checkstring(L, 1));
	size_t bufSize = 0;
	GetPathPoints(path, bufSize, NULL);
	float *pData = static_cast<float *>(_alloca(sizeof(float) * 2 * bufSize));
	if(GetPathPoints(path, bufSize, pData))
	{
		lua_createtable(L, bufSize, 0);
		for (size_t i = 0; i < bufSize; ++i)
		{
			lua_pushnumber(L, pData[i]);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	return 0;
}

//Handle GetOwner(Handle h);
static int GetOwner(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Handle o = GetOwner(h);
	PushHandle(L, o);
	return 1;
}

//void SetTarget(Handle h, Handle target);
static int SetTarget(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Handle t = RequireHandle(L, 2);
	SetTarget(h, t);
	return 0;
}


//void SetOwner(Handle h, Handle target);
static int SetOwner(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Handle o = RequireHandle(L, 2);
	SetOwner(h, o);
	return 0;
}

//void SetPilotClass(Handle h, char *odf)
static int SetPilotClass(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	char *odf = const_cast<char *>(luaL_checkstring(L, 2));
	SetPilotClass(h, odf);
	return 0;
}

//DLLEXPORT void DLLAPI AllowRandomTracks(bool bAllow = true);
static int AllowRandomTracks(lua_State *L)
{
	bool allow = luaL_optboolean(L, 2, TRUE);
	AllowRandomTracks(allow);
	return 0;
}

//DLLEXPORT void DLLAPI SetFFATeamColors(TEAMCOLOR_TYPE type);
static int SetFFATeamColors(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	SetFFATeamColors(type);
	return 0;
}

//DLLEXPORT void DLLAPI SetTeamStratColors(TEAMCOLOR_TYPE type);
static int SetTeamStratColors(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	SetTeamStratColors(type);
	return 0;
}

//DLLEXPORT void DLLAPI GetFFATeamColor(TEAMCOLOR_TYPE type, int team, int& red, int& green, int& blue);
static int GetFFATeamColor(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	int team = luaL_checkinteger(L, 2);
	int r, g, b;
	GetFFATeamColor(type, team, r, g, b);
	lua_pushinteger(L, r);
	lua_pushinteger(L, g);
	lua_pushinteger(L, b);
	return 3;
}
//inline Vector GetFFATeamColor(TEAMCOLOR_TYPE Type, int Team) { int r = 0, g = 0, b = 0; GetFFATeamColor(Type, Team, r, g, b); return Vector(float(r),float(g),float(b)); }
static int GetFFATeamColorVector(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	int team = luaL_checkinteger(L, 2);
	*NewVector(L) = GetFFATeamColor(type, team);
	return 1;
}

//DLLEXPORT void DLLAPI GetTeamStratColor(TEAMCOLOR_TYPE type, int team, int& red, int& green, int& blue);
static int GetTeamStratColor(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	int team = luaL_checkinteger(L, 2);
	int r, g, b;
	GetTeamStratColor(type, team, r, g, b);
	lua_pushinteger(L, r);
	lua_pushinteger(L, g);
	lua_pushinteger(L, b);
	return 3;
}
//inline Vector GetTeamStratColor(TEAMCOLOR_TYPE Type, int Team) { int r = 0, g = 0, b = 0; GetTeamStratColor(Type, Team, r, g, b); return Vector(float(r),float(g),float(b)); }
static int GetTeamStratColorVector(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	int team = luaL_checkinteger(L, 2);
	*NewVector(L) = GetTeamStratColor(type, team);
	return 1;
}

//DLLEXPORT void DLLAPI SwapTeamStratColors(void);
static int SwapTeamStratColors(lua_State *L)
{
	SwapTeamStratColors();
	return 0;
}

//DLLEXPORT bool DLLAPI GetTeamColorsAreFFA(void);
static int GetTeamColorsAreFFA(lua_State *L)
{
	lua_pushboolean(L, GetTeamColorsAreFFA());
	return 1;
}

//DLLEXPORT void DLLAPI SetTeamColors(TEAMCOLOR_TYPE type);
static int SetTeamColors(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	SetTeamColors(type);
	return 0;
}

//DLLEXPORT int DLLAPI AddPower(TeamNum t, int v);
static int AddPower(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	int v = luaL_checkinteger(L, 2);
	lua_pushinteger(L, AddPower(t, v));
	return 1;
}

//DLLEXPORT int DLLAPI SetPower(TeamNum t, int v);
static int SetPower(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	int v = luaL_checkinteger(L, 2);
	lua_pushinteger(L, SetPower(t, v));
	return 1;
}

//DLLEXPORT int DLLAPI GetPower(TeamNum t);
static int GetPower(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	lua_pushinteger(L, GetPower(t));
	return 1;
}

//DLLEXPORT int DLLAPI GetMaxPower(TeamNum t);
static int GetMaxPower(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	lua_pushinteger(L, GetMaxPower(t));
	return 1;
}

//DLLEXPORT void DLLAPI AddMaxPower(TeamNum t, int v);
static int AddMaxPower(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	int v = luaL_checkinteger(L, 2);
	AddMaxPower(t, v);
	lua_pushinteger(L, GetMaxPower(t));
	return 1;
}

//DLLEXPORT void DLLAPI SetMaxPower(TeamNum t, int v);
static int SetMaxPower(lua_State *L)
{
	TeamNum t = TeamNum(luaL_checkinteger(L, 1));
	int v = luaL_checkinteger(L, 2);
	SetMaxPower(t, v);
	lua_pushinteger(L, GetMaxPower(t));
	return 1;
}

//DLLEXPORT void DLLAPI GetTeamStratIndividualColor(TEAMCOLOR_TYPE type, int team, int& red, int& green, int& blue);
static int GetTeamStratIndividualColor(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	int team = luaL_checkinteger(L, 2);
	int r, g, b;
	GetTeamStratIndividualColor(type, team, r, g, b);
	lua_pushinteger(L, r);
	lua_pushinteger(L, g);
	lua_pushinteger(L, b);
	return 3;
}
//inline Vector GetTeamStratIndividualColor(TEAMCOLOR_TYPE Type, int Team) { int r = 0, g = 0, b = 0; GetTeamStratIndividualColor(Type, Team, r, g, b); return Vector(float(r),float(g),float(b)); }
static int GetTeamStratIndividualColorVector(lua_State *L)
{
	TEAMCOLOR_TYPE type = TEAMCOLOR_TYPE(luaL_checkinteger(L, 1));
	int team = luaL_checkinteger(L, 2);
	*NewVector(L) = GetTeamStratIndividualColor(type, team);
	return 1;
}

// GetMapTRNFilename
static int GetMapTRNFilename(lua_State *L)
{
	lua_pushstring(L, GetMapTRNFilename());
	return 1;
}

//DLLEXPORT bool DLLAPI IsNotDeadAndPilot2(Handle h);
static int IsNotDeadAndPilot2(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsNotDeadAndPilot2(h));
	return 1;
}

//DLLEXPORT const char* DLLAPI GetLabel(Handle h);
static int GetLabel(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	if ( const char *label = GetLabel(h) )
	{
	   lua_pushstring(L, label);
	   return 1;
	}
	return 0;
}

//DLLEXPORT void DLLAPI SetLabel(Handle h, const char* pLabel);
static int SetLabel(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	const char *label = luaL_checkstring(L, 2);
	SetLabel(h, label);
	return 0;
}

//DLLEXPORT Handle DLLAPI GetTap(Handle h, int index);
static int GetTap(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int index = luaL_checkinteger(L, 2);
	Handle n = GetTap(h, index);
	PushHandle(L, n);
	return 1;
}

//DLLEXPORT void DLLAPI SetTap(Handle baseObjectHandle, int index, Handle tapObjectHandle);
static int SetTap(lua_State *L)
{
	Handle h1 = RequireHandle(L, 1);
	int index = luaL_checkinteger(L, 2);
	Handle h2 = GetHandle(L, 3);
	SetTap(h1, index, h2);
	return 0;
}

//DLLEXPORT float DLLAPI GetCurLocalAmmo(Handle h, int weaponIndex);
static int GetCurLocalAmmo(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int index = luaL_checkinteger(L, 2);
	lua_pushnumber(L, GetCurLocalAmmo(h, index));
	return 1;
}

//DLLEXPORT void DLLAPI AddLocalAmmo(Handle h, float v, int weaponIndex);
static int AddLocalAmmo(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	float v = float(luaL_checknumber(L, 2));
	int index = luaL_checkinteger(L, 3);
	AddLocalAmmo(h, v, index);
	return 0;
}

//DLLEXPORT float DLLAPI GetMaxLocalAmmo(Handle h, int weaponIndex);
static int GetMaxLocalAmmo(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	int index = luaL_checkinteger(L, 2);
	lua_pushnumber(L, GetMaxLocalAmmo(h, index));
	return 1;
}

//DLLEXPORT void DLLAPI SetCurLocalAmmo(Handle h, float v, int weaponIndex);
static int SetCurLocalAmmo(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	float v = float(luaL_checknumber(L, 2));
	int index = luaL_checkinteger(L, 3);
	SetCurLocalAmmo(h, v, index);
	return 0;
}

//DLLEXPORT const char* DLLAPI GetNetworkListItem(NETWORK_LIST_TYPE listType, size_t item);
static int GetNetworkListItem(lua_State *L)
{
	NETWORK_LIST_TYPE type = NETWORK_LIST_TYPE(luaL_checkinteger(L, 1));
	int size = luaL_checkinteger(L, 2);
	lua_pushstring(L, GetNetworkListItem(type, size));
	return 1;
}

//DLLEXPORT size_t DLLAPI GetNetworkListCount(NETWORK_LIST_TYPE listType);
static int GetNetworkListCount(lua_State *L)
{
	NETWORK_LIST_TYPE type = NETWORK_LIST_TYPE(luaL_checkinteger(L, 1));
	lua_pushinteger(L, GetNetworkListCount(type));
	return 1;
}

//DLLEXPORT TEAMRELATIONSHIP DLLAPI GetTeamRelationship(Handle h1, Handle h2);
static int GetTeamRelationship(lua_State *L)
{
	Handle h1 = RequireHandle(L, 1);
	Handle h2 = RequireHandle(L, 2);
	lua_pushnumber(L, GetTeamRelationship(h1, h2));
	return 1;
}

//DLLEXPORT bool DLLAPI HasPilot(Handle h);
static int HasPilot(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, HasPilot(h));
	return 1;
}

//DLLEXPORT const char* DLLAPI GetPilotClass(Handle h);
static int GetPilotClass(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	if ( const char *pilotclass = GetPilotClass(h) )
	{
	   lua_pushstring(L, pilotclass);
	   return 1;
	}
	return 0;
}

//DLLEXPORT int DLLAPI GetBaseScrapCost(Handle h);
static int GetBaseScrapCost(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetBaseScrapCost(h));
	return 1;
}

//DLLEXPORT int DLLAPI GetActualScrapCost(Handle h);
static int GetActualScrapCost(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetActualScrapCost(h));
	return 1;
}

//DLLEXPORT void DLLAPI PetWatchdogThread(void);
static int PetWatchdogThread(lua_State *L)
{
	PetWatchdogThread();
	return 0;
}

//TeamNum GetPerceivedTeam(Handle h);
static int GetPerceivedTeam(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushinteger(L, GetPerceivedTeam(h));
	return 1;
}

//DLLEXPORT void DLLAPI SetLastCurrentPosition(Handle h, const Matrix &lastMatrix, const Matrix &curMatrix);
static int SetLastCurrentPosition(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	Matrix CurMatrix = Identity_Matrix;

	if (Matrix *mat = GetMatrix(L, 2))
	{
		SetPosition(h, *mat);
		SetLastCurrentPosition(h, *mat, CurMatrix);
		*NewMatrix(L) = CurMatrix;
		return 1;
	}
	return 0;
}

//DLLEXPORT float DLLAPI GetRemainingLifespan(Handle h);
static int GetRemainingLifespan(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushnumber(L, GetRemainingLifespan(h));
	return 1;
}

//DLLEXPORT size_t DLLAPI GetAllSpawnpoints(SpawnpointInfo*& pSpawnPointInfo, int baseTeamNumber = 0);
static int GetAllSpawnpoints(lua_State *L)
{
	int team = luaL_optinteger(L, 1, 0);
	SpawnpointInfo* info;
	size_t count = GetAllSpawnpoints(info, team);

	lua_createtable(L, count, 0);

	for(size_t i = 0; i < count; i++)
	{
		lua_createtable(L, 0, 7);
		*NewVector( L )  = info[i].m_Position;
		lua_setfield(L, -2, "Position");
		lua_pushinteger(L, info[i].m_Team);
		lua_setfield(L, -2, "Team");
		PushHandle(L, info[i].m_Handle);
		lua_setfield(L, -2, "Handle");
		lua_pushnumber(L, info[i].m_DistanceToClosestTeamZero);
		lua_setfield(L, -2, "DistanceToClosestTeamZero");
		lua_pushnumber(L, info[i].m_DistanceToClosestSameTeam);
		lua_setfield(L, -2, "DistanceToClosestSameTeam");
		lua_pushnumber(L, info[i].m_DistanceToClosestAlly);
		lua_setfield(L, -2, "DistanceToClosestAlly");
		lua_pushnumber(L, info[i].m_DistanceToClosestEnemy);
		lua_setfield(L, -2, "DistanceToClosestEnemy");
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

//DLLEXPORT const char* DLLAPI GetPlan(int team);
static int GetPlan(lua_State *L)
{
	int team = luaL_checkinteger(L, 1);
	if ( const char *aipname = GetPlan(team) )
	{
	   lua_pushstring(L, aipname);
	   return 1;
	}
	return 0;
}

//DLLEXPORT int DLLAPI GetIndependence(Handle h);
static int GetIndependence(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushnumber(L, GetIndependence(h));
	return 1;
}

//DLLEXPORT int DLLAPI GetSkill(Handle h);
static int GetSkill(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushnumber(L, GetSkill(h));
	return 1;
}

//DLLEXPORT const char* DLLAPI GetObjectiveName(Handle h);
static int GetObjectiveName(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	if ( const char *objectivename = GetObjectiveName(h) )
	{
	   lua_pushstring(L, objectivename);
	   return 1;
	}
	return 0;
}

//DLLEXPORT long DLLAPI GetWeaponMask(Handle h);
static int GetWeaponMask(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushnumber(L, GetWeaponMask(h));
	return 1;
}

//DLLEXPORT void DLLAPI SetInterpolablePosition(Handle h, const Matrix* pLastMat = NULL, const Matrix* pTrueMat = NULL, bool bSetPosition = true);
//DLLEXPORT void DLLAPI SetInterpolablePosition(Handle h, const Vector* pLastPos = NULL, const Vector* pTruePos = NULL);
static int SetInterpolablePosition(lua_State *L)
{
	Handle h = RequireHandle(L, 1);

	if (Matrix *mat1 = GetMatrix(L, 2))
	{
		bool setpos = luaL_optboolean(L, 4, TRUE);

		if (Matrix *mat2 = GetMatrix(L, 3))
			SetInterpolablePosition(h, mat1, mat2, setpos);
		else
			SetInterpolablePosition(h, mat1, NULL, setpos);
		
	}
	else if (Vector *pos1 = GetVector(L, 2))
	{
		if (Vector *pos2 = GetVector(L, 3))
			SetInterpolablePosition(h, pos1, pos2);
		else
			SetInterpolablePosition(h, pos1, NULL);
	}
	/*
	else
	{
		SetInterpolablePosition(h); //error C2668: 'SetInterpolablePosition' : ambiguous call to overloaded function
	}
	*/

	return 0;
}

//DLLEXPORT int DLLAPI SecondsToTurns(float timeSeconds);
static int SecondsToTurns(lua_State *L)
{
	float time = float(luaL_checknumber(L, 1));
	lua_pushnumber(L, SecondsToTurns(time));
	return 1;
}
//DLLEXPORT float DLLAPI TurnsToSeconds(int turns);
static int TurnsToSeconds(lua_State *L)
{
	int turns = luaL_checkinteger(L, 1);
	lua_pushnumber(L, TurnsToSeconds(turns));
	return 1;
}

//float DLLAPI GetLifeSpan(Handle h);
static int GetLifeSpan(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushnumber(L, GetLifeSpan(h));
	return 1;
}

//Vector DLLAPI GetCurrentCommandWhere(Handle h);
static int GetCurrentCommandWhere(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	*NewVector(L) = GetCurrentCommandWhere(h);
	return 1;
}

//DLLEXPORT bool DLLAPI GetAllGameObjectHandles(size_t& bufSize, Handle* pData);
static int GetAllGameObjectHandles(lua_State *L)
{
	size_t bufSize = 0;
	GetAllGameObjectHandles(bufSize, NULL);
	Handle *harray = static_cast<Handle *>(_alloca(sizeof(Handle) * bufSize));
	if (GetAllGameObjectHandles(bufSize, harray))
	{
		lua_createtable(L, bufSize, 0);
		for (size_t i = 0; i < bufSize; ++i)
		{
			lua_pushnumber(L, harray[i]);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	return 0;
}

//DLLEXPORT Vector DLLAPI GetOmega(Handle h);
static int GetOmega(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	*NewVector(L) = GetOmega(h);
	return 1;
}

//DLLEXPORT void DLLAPI SetOmega(Handle h, const Vector& omega);
static int SetOmega(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	if (Vector *v = GetVector(L, 2))
		SetOmega(h, *v);

	return 0;
}

//DLLEXPORT bool DLLAPI IsCraftButNotPerson(Handle me);
static int IsCraftButNotPerson(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsCraftButNotPerson(h));
	return 1;
}

//DLLEXPORT bool DLLAPI IsCraftOrPerson(Handle me);
static int IsCraftOrPerson(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsCraftOrPerson(h));
	return 1;
}

//DLLEXPORT bool DLLAPI IsBuilding(Handle me);
static int IsBuilding(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsBuilding(h));
	return 1;
}

//DLLEXPORT bool DLLAPI IsPowerup(Handle me);
static int IsPowerup(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, IsPowerup(h));
	return 1;
}

//DLLEXPORT void DLLAPI SetCanSnipe(Handle me, int canSnipe = -1);
static int SetCanSnipe(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	int cansnipe = luaL_checkinteger(L, 2);
	SetCanSnipe(me, cansnipe);
	return 0;
}

//DLLEXPORT bool DLLAPI GetCanSnipe(Handle me);
static int GetCanSnipe(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	lua_pushboolean(L, GetCanSnipe(h));
	return 1;
}

//DLLEXPORT bool DLLAPI WhoIsTargeting(size_t& bufSize, Handle* pData, Handle me);
static int WhoIsTargeting(lua_State *L)
{
	Handle me = RequireHandle(L, 1);
	size_t bufSize = 0;
	WhoIsTargeting(bufSize, NULL, me);
	Handle *harray = static_cast<Handle *>(_alloca(sizeof(Handle) * bufSize));
	if (WhoIsTargeting(bufSize, harray, me))
	{
		lua_createtable(L, bufSize, 0);
		for (size_t i = 0; i < bufSize; ++i)
		{
			lua_pushnumber(L, harray[i]);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	return 0;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------
// BZScriptor Backwards Compatability Functions:
//---------------------------------------------------------------------------------------------------------------------------------------------------------------

//extern void SetAngle(Handle h, float Degrees);
static int SetAngle(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	float degrees = float(luaL_checknumber(L, 2));
	SetAngle(h, degrees);
	return 0;
}

//extern bool CameraPos(Handle me, Handle him, Vector &PosA, Vector &PosB, float Increment);
static int CameraPos(lua_State *L)
{
	Handle h1 = RequireHandle(L, 1);
	Handle h2 = RequireHandle(L, 2);
	Vector *v1 = RequireVector(L, 3);
	Vector *v2 = RequireVector(L, 4);
	float inc = float(luaL_checknumber(L, 5));
	lua_pushboolean(L, CameraPos(h1, h2, *v1, *v2, inc));
	return 1;
}

//Handle ReplaceObject(Handle h, char* ODF = NULL, int Team = -1, float HeightOffset = 0.0f, int Empty = -1, bool RestoreWeapons = true, int Group = -1, bool KeepCommand = true, int NewCommand = -1, Handle NewWho = -1);
static int ReplaceObject(lua_State *L)
{
	Handle h = RequireHandle(L, 1);
	const char *replace = luaL_optstring(L, 2, NULL);
	int team = luaL_optinteger(L, 3, -1);
	float height = float(luaL_optnumber(L, 4, 0.0f));
	int empty = luaL_optinteger(L, 5, -1);
	bool restoreweapons = luaL_optboolean(L, 6, TRUE);
	int group = luaL_optinteger(L, 7, -1);
	bool keepcmd = luaL_optboolean(L, 8, TRUE);
	int newcmd = luaL_optinteger(L, 9, -1);
	Handle newwho = GetHandle(L, 10);
	PushHandle(L, ReplaceObject(h, const_cast<char *>(replace), team, height, empty, restoreweapons, group, keepcmd, newcmd, newwho));
	return 1;
}

// Lua script utils functions
static const luaL_Reg sLuaScriptUtils [] = {
	{ "GetHandle", GetHandle },
	{ "Make_RGB", Make_RGB },
	{ "Make_RGBA", Make_RGBA },
// BZ2 Script Utils Inlines.
	{ "FailMission", FailMission },
	{ "SucceedMission", SucceedMission },
	{ "ChangeSide", ChangeSide },
	{ "AddScrap", AddScrap },
	{ "SetScrap", SetScrap },
	{ "GetScrap", GetScrap },
	{ "GetMaxScrap", GetMaxScrap },
	{ "GetTime", GetTime },
	{ "GetTug", GetTug },
	{ "HasCargo", HasCargo },
	{ "GetDistance", GetDistance },
	{ "GetNearestObject", GetNearestObject },
	{ "GetNearestVehicle", GetNearestVehicle },
	{ "GetNearestBuilding", GetNearestBuilding },
	{ "GetNearestEnemy", GetNearestEnemy },
// BZ2 Script Utils Functions.
	{ "BuildObject", BuildObject },
	{ "RemoveObject", RemoveObject },
	{ "GetFirstEmptyGroup", GetFirstEmptyGroup },
	{ "SetGroup", SetGroup },
	{ "Attack", Attack },
	{ "Service", Service },
	{ "Goto", Goto },
	{ "Mine", Mine },
	{ "Follow", Follow },
	{ "Defend", Defend },
	{ "Defend2", Defend2 },
	{ "Stop", Stop },
	{ "Patrol", Patrol },
	{ "Retreat", Retreat },
	{ "GetIn", GetIn },
	{ "Pickup", Pickup },
	{ "Dropoff", Dropoff },
	{ "Build", Build },
	{ "LookAt", LookAt },
	{ "AllLookAt", AllLookAt },
	{ "IsOdf", IsOdf },
	{ "GetRace", GetRace },
	{ "GetPlayerHandle", GetPlayerHandle },
	{ "IsAlive", IsAlive },
	{ "IsFlying", IsFlying },
	{ "IsAliveAndPilot", IsAliveAndPilot },
	{ "IsAround", IsAround },
	{ "InBuilding", InBuilding },
	{ "AtTerminal", AtTerminal },
	{ "GetPosition", GetPosition },
	{ "GetPosition2", GetPosition2 },
	{ "GetFront", GetFront },
	{ "SetPosition", SetPosition },
	{ "Damage", Damage },
	{ "GetHealth", GetHealth },
	{ "GetCurHealth", GetCurHealth },
	{ "GetMaxHealth", GetMaxHealth },
	{ "SetCurHealth", SetCurHealth },
	{ "SetMaxHealth", SetMaxHealth },
	{ "AddHealth", AddHealth },
	{ "GetAmmo", GetAmmo },
	{ "GetCurAmmo", GetCurAmmo },
	{ "GetMaxAmmo", GetMaxAmmo },
	{ "SetCurAmmo", SetCurAmmo },
	{ "SetMaxAmmo", SetMaxAmmo },
	{ "AddAmmo", AddAmmo },
	{ "GetTeamNum", GetTeamNum },
	{ "SetTeamNum", SetTeamNum },
	{ "SetVelocity", SetVelocity },
	{ "SetControls", SetControls },
	{ "GetWhoShotMe", GetWhoShotMe },
	{ "GetLastEnemyShot", GetLastEnemyShot },
	{ "GetLastFriendShot", GetLastFriendShot },
	{ "DefaultAllies", DefaultAllies },
	{ "TeamplayAllies", TeamplayAllies },
	{ "Ally", Ally },
	{ "UnAlly", UnAlly },
	{ "IsTeamAllied", IsTeamAllied },
	{ "IsAlly", IsAlly },
	{ "AudioMessage", AudioMessage },
	{ "IsAudioMessageDone", IsAudioMessageDone },
	{ "StopAudioMessage", StopAudioMessage },
	{ "PreloadAudioMessage", PreloadAudioMessage },
	{ "PurgeAudioMessage", PurgeAudioMessage },
	{ "PreloadMusicMessage", PreloadMusicMessage },
	{ "PurgeMusicMessage", PurgeMusicMessage },
	{ "LoadJukeFile", LoadJukeFile },
	{ "SetMusicIntensity", SetMusicIntensity },
	//{ "FindAiPath", FindAiPath },
	//{ "FreeAiPath", FreeAiPath },
	{ "GetAiPaths", GetAiPaths },
	{ "SetPathType", SetPathType },
	{ "SetIndependence", SetIndependence },
	{ "IsInfo", IsInfo },
	{ "StartCockpitTimer", StartCockpitTimer },
	{ "StartCockpitTimerUp", StartCockpitTimerUp },
	{ "StopCockpitTimer", StopCockpitTimer },
	{ "HideCockpitTimer", HideCockpitTimer },
	{ "GetCockpitTimer", GetCockpitTimer },
	{ "StartEarthQuake", StartEarthQuake },
	{ "UpdateEarthQuake", UpdateEarthQuake },
	{ "StopEarthQuake", StopEarthQuake },
	{ "IsPerson", IsPerson },
	{ "GetCurWorld", GetCurWorld },
	{ "GetVarItemStr", GetVarItemStr },
	{ "GetVarItemInt", GetVarItemInt },
	{ "GetCVarItemInt", GetCVarItemInt },
	{ "GetCVarItemStr", GetCVarItemStr },
	{ "PreloadODF", PreloadODF },
	{ "TerrainFindFloor", TerrainFindFloor },
	{ "AddPilotByHandle", AddPilotByHandle },
	{ "PrintConsoleMessage", PrintConsoleMessage },
	{ "GetRandomFloat", GetRandomFloat },
	{ "IsDeployed", IsDeployed },
	{ "Deploy", Deploy },
	{ "IsSelected", IsSelected },
	{ "SetWeaponMask", SetWeaponMask },
	{ "GiveWeapon", GiveWeapon },
	{ "FireAt", FireAt },
	{ "IsFollowing", IsFollowing },
	{ "WhoFollowing", WhoFollowing },
	{ "SetUserTarget", SetUserTarget },
	{ "GetUserTarget", GetUserTarget },
	{ "SetPerceivedTeam", SetPerceivedTeam },
	{ "GetCurrentCommand", GetCurrentCommand },
	{ "GetCurrentWho", GetCurrentWho },
	{ "EjectPilot", EjectPilot },
	{ "HopOut", HopOut },
	{ "KillPilot", KillPilot },
	{ "RemovePilot", RemovePilot },
	{ "HoppedOutOf", HoppedOutOf },
	{ "GetCameraPosition", GetCameraPosition },
	{ "SetCameraPosition", SetCameraPosition },
	{ "ResetCameraPosition", ResetCameraPosition },
	{ "IFace_Exec", IFace_Exec },
	{ "IFace_Activate", IFace_Activate },
	{ "IFace_Deactivate", IFace_Deactivate },
	{ "IFace_CreateCommand", IFace_CreateCommand },
	{ "IFace_CreateString", IFace_CreateString },
	{ "IFace_SetString", IFace_SetString },
	{ "IFace_GetString", IFace_GetString },
	{ "IFace_CreateInteger", IFace_CreateInteger },
	{ "IFace_SetInteger", IFace_SetInteger },
	{ "IFace_GetInteger", IFace_GetInteger },
	{ "IFace_SetIntegerRange", IFace_SetIntegerRange },
	{ "IFace_CreateFloat", IFace_CreateFloat },
	{ "IFace_SetFloat", IFace_SetFloat },
	{ "IFace_GetFloat", IFace_GetFloat },
	{ "IFace_ClearListBox", IFace_ClearListBox },
	{ "IFace_AddTextItem", IFace_AddTextItem },
	{ "IFace_GetSelectedItem", IFace_GetSelectedItem },
	{ "SetSkill", SetSkill },
	{ "SetAIP", SetAIP },
	{ "LogFloat", LogFloat },
	{ "GetInstantMyForce", GetInstantMyForce },
	{ "GetInstantCompForce", GetInstantCompForce },
	{ "GetInstantDifficulty", GetInstantDifficulty },
	{ "GetInstantGoal", GetInstantGoal },
	{ "GetInstantType", GetInstantType },
	{ "GetInstantFlag", GetInstantFlag },
	{ "GetInstantMySide", GetInstantMySide },
	//{ "StoppedPlayback", StoppedPlayback }, // No such function.
	{ "CameraReady", CameraReady },
	{ "CameraPath", CameraPath },
	{ "CameraPathDir", CameraPathDir },
	{ "PanDone", PanDone },
	{ "CameraObject", CameraObject },
	{ "CameraFinish", CameraFinish },
	{ "CameraCancelled", CameraCancelled },
	{ "PlayMovie", PlayMovie },
	{ "StopMovie", StopMovie },
	{ "PlayMove", PlayMove },
	{ "PlayRecording", PlayRecording },
	{ "PlaybackVehicle", PlaybackVehicle },
	{ "SetAnimation", SetAnimation },
	{ "GetAnimationFrame", GetAnimationFrame },
	{ "StartAnimation", StartAnimation },
	{ "MaskEmitter", MaskEmitter },
	{ "StartEmitter", StartEmitter },
	{ "StopEmitter", StopEmitter },
	//{ "SaveObjects", SaveObjects },
	//{ "LoadObjects", LoadObjects },
	//{ "IgnoreSync", IgnoreSync },
	//{ "IsRecording", IsRecording },
	{ "SetObjectiveOn", SetObjectiveOn },
	{ "SetObjectiveOff", SetObjectiveOff },
	{ "SetObjectiveName", SetObjectiveName },
	{ "ClearObjectives", ClearObjectives },
	{ "AddObjective", AddObjective },
	{ "IsWithin", IsWithin },
	{ "CountUnitsNearObject", CountUnitsNearObject },
	{ "SpawnBirds", SpawnBirds },
	{ "RemoveBirds", RemoveBirds },
	{ "SetColorFade", SetColorFade },
	{ "StopCheats", StopCheats },
	{ "CalcCliffs", CalcCliffs },
	{ "StartSoundEffect", StartSoundEffect },
	{ "FindSoundEffect", FindSoundEffect },
	{ "StopSoundEffect", StopSoundEffect },
	{ "GetObjectByTeamSlot", GetObjectByTeamSlot },
	{ "IsNetGame", IsNetworkOn },
	{ "IsHosting", ImServer },
	{ "IsNetworkOn", IsNetworkOn },
	{ "ImServer", ImServer },
	{ "ImDedicatedServer", ImDedicatedServer },
	{ "IsTeamplayOn", IsTeamplayOn },
	{ "CountPlayers", CountPlayers },
	{ "GetRaceOfTeam", GetRaceOfTeam },
	{ "IsPlayer", IsPlayer },
	{ "GetPlayerName", GetPlayerName },
	{ "WhichTeamGroup", WhichTeamGroup },
	{ "CountAllies", CountAllies },
	{ "GetCommanderTeam", GetCommanderTeam },
	{ "GetFirstAlliedTeam", GetFirstAlliedTeam },
	{ "GetLastAlliedTeam", GetLastAlliedTeam },
	{ "GetTeamplayRanges", GetTeamplayRanges },
	{ "SetRandomHeadingAngle", SetRandomHeadingAngle },
	{ "ClearTeamColors", ClearTeamColors },
	{ "DefaultTeamColors", DefaultTeamColors },
	{ "TeamplayTeamColors", TeamplayTeamColors },
	{ "SetTeamColor", SetTeamColor },
	{ "ClearTeamColor", ClearTeamColor },
	{ "MakeInert", MakeInert },
	{ "GetPositionNear", GetPositionNear },
	{ "GetPlayerODF", GetPlayerODF },
	{ "BuildEmptyCraftNear", BuildEmptyCraftNear },
	{ "GetCircularPos", GetCircularPos },
	{ "GetSafestSpawnpoint", GetSafestSpawnpoint },
	{ "GetSpawnpoint", GetSpawnpoint },
	{ "GetSpawnpointHandle", GetSpawnpointHandle },
	{ "GetRandomSpawnpoint", GetRandomSpawnpoint },
	{ "SetTimerBox", SetTimerBox },
	{ "AddToMessagesBox", AddToMessagesBox },
	{ "GetDeaths", GetDeaths },
	{ "GetKills", GetKills },
	{ "GetScore", GetScore },
	{ "SetDeaths", SetDeaths },
	{ "SetKills", SetKills },
	{ "SetScore", SetScore },
	{ "AddDeaths", AddDeaths },
	{ "AddKills", AddKills },
	{ "AddScore", AddScore },
	{ "SetAsUser", SetAsUser },
	{ "GetTarget", GetTarget },
	{ "IFace_ConsoleCmd", IFace_ConsoleCmd },
	{ "GetVelocity", GetVelocity },
	{ "GetCfg", GetObjInfo_CFG },
	{ "GetOdf", GetObjInfo_ODF },
	{ "GetBase", GetObjInfo_GOClass_gCfg, },
	{ "GetClassSig", GetObjInfo_EntityType },
	{ "GetClassLabel", GetObjInfo_GOClass },
	{ "GetWeaponConfig ", Get_WeaponConfig  },
	{ "GetWeaponODF", Get_WeaponODF },
	{ "GetWeaponGOClass", Get_WeaponGOClass },
	{ "DoesODFExist", DoesODFExist },
	{ "IsAlive2", IsAlive2 },
	{ "IsFlying2", IsFlying2 },
	{ "IsAliveAndPilot2", IsAliveAndPilot2 },
	{ "TranslateString", TranslateString2 },
	{ "GetScavengerCurScrap", GetScavengerCurScrap },
	{ "GetScavengerMaxScrap", GetScavengerMaxScrap },
	{ "SetScavengerCurScrap", SetScavengerCurScrap },
	{ "SetScavengerMaxScrap", SetScavengerMaxScrap },
	//{ "DamageF", DamageF },
	{ "SelfDamage", SelfDamage },
	{ "WantBotKillMessages", WantBotKillMessages },
	{ "EnableHighTPS", EnableHighTPS },
	{ "GetLocalUserInspectHandle", GetLocalUserInspectHandle },
	{ "GetLocalUserSelectHandle", GetLocalUserSelectHandle },
	{ "ResetTeamSlot", ResetTeamSlot },
	{ "GetCategoryTypeOverride", GetCategoryTypeOverride },
	{ "GetCategoryType", GetCategoryType },
	{ "GetODFHexInt", GetODFBool },
	{ "GetODFInt", GetODFInt },
	{ "GetODFLong", GetODFLong },
	{ "GetODFFloat", GetODFFloat },
	{ "GetODFDouble", GetODFDouble },
	{ "GetODFBool", GetODFBool },
	{ "GetODFString", GetODFString },
	{ "GetODFColor", GetODFColor },
	{ "GetODFVector", GetODFVector },
	//{ "OpenODF", OpenODF },
	//{ "CloseODF", CloseODF },
	{ "NoteGameoverWithCustomMessage", NoteGameoverWithCustomMessage },
	{ "SetBestGroup", SetBestGroup },
	{ "GetGroup", GetGroup },
	{ "GetGroupCount", GetGroupCount },
	{ "SetLifespan", SetLifespan },
	{ "DoesFileExist", DoesFileExist },
	{ "LoadFile", LoadFile },
	{ "StartAudio3D", StartAudio3D },
	{ "StartAudio2D", StartAudio2D },
	{ "IsAudioPlaying", IsAudioPlaying },
	{ "StopAudio", StopAudio },
	{ "PauseAudio", PauseAudio },
	{ "ResumeAudio", ResumeAudio },
	{ "SetVolume", SetVolume },
	{ "SetPan", SetPan },
	{ "SetRate", SetRate },
	{ "GetAudioFileDuration", GetAudioFileDuration },
	{ "IsPlayingLooped", IsPlayingLooped },
	{ "GetNearestPowerup", GetNearestPowerup },
	{ "GetNearestPerson", GetNearestPerson },
	{ "SetCommand", SetCommand },
	{ "SetGravity", SetGravity },
	{ "SetAutoGroupUnits", SetAutoGroupUnits },
	{ "KickPlayer", KickPlayer },
	{ "TerrainIsWater", TerrainIsWater },
	{ "GetTerrainHeightAndNormal", GetTerrainHeightAndNormal },
	{ "WriteToFile", WriteToFile }, //{ "GetOutputPath", GetOutputPath },
	{ "GetPathPoints", GetPathPoints },
	{ "GetOwner", GetOwner },
	{ "SetTarget", SetTarget },
	{ "SetOwner", SetOwner },
	{ "SetPilotClass", SetPilotClass },
	{ "AllowRandomTracks", AllowRandomTracks },
	{ "SetFFATeamColors", SetFFATeamColors },
	{ "SetTeamStratColors", SetTeamStratColors },
	{ "GetFFATeamColor", GetFFATeamColor },
	{ "GetFFATeamColorVector", GetFFATeamColorVector },
	{ "GetTeamStratColor", GetTeamStratColor },
	{ "GetTeamStratColorVector", GetTeamStratColorVector },
	{ "SwapTeamStratColors", SwapTeamStratColors },
	{ "GetTeamColorsAreFFA", GetTeamColorsAreFFA },
	{ "SetTeamColors", SetTeamColors },
	{ "AddPower", AddPower },
	{ "SetPower", SetPower },
	{ "GetPower", GetPower },
	{ "GetMaxPower", GetMaxPower },
	{ "AddMaxPower", AddMaxPower },
	{ "AddMaxScrap", AddMaxScrap },
	{ "SetMaxScrap", SetMaxScrap },
	{ "SetMaxPower", SetMaxPower },
	{ "GetTeamStratIndividualColor", GetTeamStratIndividualColor },
	{ "GetTeamStratIndividualColorVector", GetTeamStratIndividualColorVector },
	{ "GetMapTRNFilename", GetMapTRNFilename },
	{ "IsNotDeadAndPilot2", IsNotDeadAndPilot2 },
	{ "GetLabel", GetLabel },
	{ "SetLabel", SetLabel },
	{ "GetTap", GetTap },
	{ "SetTap", SetTap },
	{ "GetCurLocalAmmo", GetCurLocalAmmo },
	{ "AddLocalAmmo", AddLocalAmmo },
	{ "GetMaxLocalAmmo", GetMaxLocalAmmo },
	{ "SetCurLocalAmmo", SetCurLocalAmmo },
	{ "GetNetworkListItem", GetNetworkListItem },
	{ "GetNetworkListCount", GetNetworkListCount },
	{ "GetTeamRelationship", GetTeamRelationship },
	// 1.3.6.4+
	{ "HasPilot", HasPilot },
	{ "GetPilotClass", GetPilotClass },
	{ "GetBaseScrapCost", GetBaseScrapCost },
	{ "GetActualScrapCost", GetActualScrapCost },
	{ "PetWatchdogThread", PetWatchdogThread },
	{ "GetPerceivedTeam", GetPerceivedTeam },
	{ "SetLastCurrentPosition", SetLastCurrentPosition },
	{ "GetRemainingLifespan", GetRemainingLifespan },
	{ "GetAllSpawnpoints", GetAllSpawnpoints },
	// 1.3.7.0+
	{ "GetPlan", GetPlan },
	{ "GetIndependence", GetIndependence },
	{ "GetSkill", GetSkill },
	{ "GetWeaponMask", GetWeaponMask },
	{ "SetInterpolablePosition", SetInterpolablePosition },
	{ "SecondsToTurns", SecondsToTurns },
	{ "TurnsToSeconds", TurnsToSeconds },
	{ "GetLifeSpan", GetLifeSpan },
	// 1.3.7.2+
	{ "GetCurrentCommandWhere", GetCurrentCommandWhere },
	{ "GetAllGameObjectHandles", GetAllGameObjectHandles },
	{ "GetOmega", GetOmega },
	{ "SetOmega", SetOmega },
	{ "IsCraftButNotPerson", IsCraftButNotPerson },
	{ "IsCraftOrPerson", IsCraftOrPerson },
	{ "IsBuilding", IsBuilding },
	{ "IsPowerup", IsPowerup },
	{ "SetCanSnipe", SetCanSnipe },
	{ "GetCanSnipe", GetCanSnipe },
	{ "WhoIsTargeting", WhoIsTargeting },
	// BZScriptor Backwards Compatability.
	{ "SetAngle", SetAngle },
	{ "CameraPos", CameraPos },
	{ "ReplaceObject", ReplaceObject },

	{ NULL, NULL }
};

// read a lua value from the file
static bool LoadValue(lua_State *L, bool push)
{
	bool ret = true;
	unsigned char type = '0';
	//fread(&type, sizeof(type), 1, fp);
	ret = ret && Read(&type, 1);
	type -= '0';

	switch (type)
	{
	default:
	case LUA_TNIL:
		{
			if (push)
				lua_pushnil(L);
		}
		break;
	case LUA_TBOOLEAN:
		{
			bool value = false;
			//in(fp, &value, sizeof(value));
			ret = ret && Read(&value, 1);
			if (push)
				lua_pushboolean(L, value);
		}
		break;
	case LUA_TLIGHTUSERDATA:
		{
			Handle value = 0;
			//in(fp, &value, sizeof(value));
			ret = ret && Read(&value, 1);
			ConvertHandles(&value, 1);
			if (push)
				lua_pushlightuserdata(L, (void *)value);
		}
		break;
	case LUA_TNUMBER:
		{
			double value = 0.0;
			//in(fp, &value, sizeof(value));
			ret = ret && Read(&value, sizeof(value));
			if (push)
				lua_pushnumber(L, value);
		}
		break;
	case LUA_TSTRING:
		{
			size_t len = 0;
			//in(fp, &len, sizeof(len));
			ret = ret && Read(&len, sizeof(len));
			char *buf = static_cast<char *>(_alloca(len));
			//in(fp, buf, len);
			ret = ret && Read(buf, len);
			if (push)
				lua_pushlstring(L, buf, len);
		}
		break;
	case LUA_TTABLE:
		{
			int count = 0;
			//in(fp, &count, sizeof(count));
			ret = ret && Read(&count, 1);
			lua_newtable(L);
			for (int i = 0; i < count; ++i)
			{
				ret = ret && LoadValue(L, push);	// key
				ret = ret && LoadValue(L, push);	// value
				if (push)
					lua_settable(L, -3);
			}
		}
		break;
	case LUA_TUSERDATA:
		{
			unsigned long type = 0;
			//in(fp, &type, sizeof(type));
			ret = ret && Read(&type, sizeof(type));
			switch (type)
			{
			case 0x8f89e802 /* "Vector" */:
				{
					Vector v;
					//in(fp, &v, sizeof(v));
					ret = ret && Read(&v, sizeof(v));
					if (push)
						*NewVector(L) = v;
				}
				break;
			case 0x15c2f8ec /* "Matrix" */:
				{
					Matrix m;
					//in(fp, &m, sizeof(m));
					ret = ret && Read(&m, sizeof(m));
					if (push)
						*NewMatrix(L) = m;
				}
				break;
		//	case 0x79fa9618 /* "AiPath" */:
		//		{
		//			AiPath *p;
		//			ret = ret && Read(&p, sizeof(p));
		//			if (push)
		//				*NewAiPath(L) = p;
		//		}
		//		break;
			default:
				break;
			}
		}
		break;
	}

	return ret;
}

// save a Lua value to the file
static bool SaveValue(lua_State *L, int i)
{
	bool ret = true;
	if (i < 0)
	{
		i += lua_gettop(L) + 1;
	}

	unsigned char type = unsigned char(lua_type(L, i));
	type += '0';
	ret = ret && Write(&type, 1);
	type -= '0';

	switch (type)
	{
	case LUA_TBOOLEAN:
		{
			bool value = lua_toboolean(L, i) != 0;
			//out(fp, &value, sizeof(value), "b");
			ret = ret && Write(&value, 1);
		}
		break;
	case LUA_TLIGHTUSERDATA:
		{
			Handle value = Handle(luaL_testudata(L, i, "Handle"));
			//out(fp, &value, sizeof(value), "h");
			ret = ret && Write(&value, 1);
		}
		break;
	case LUA_TNUMBER:
		{
			double value = lua_tonumber(L, i);
			//out(fp, &value, sizeof(value), "f");
			ret = ret && Write(&value, sizeof(value));
		}
		break;
	case LUA_TSTRING:
		{
			size_t len;
			const char *value = lua_tolstring(L, i, &len);
			//out(fp, reinterpret_cast<int *>(&len), sizeof(len), "l");
			//out(fp, const_cast<char *>(value), len, "s");
			ret = ret && Write(&len, sizeof(len));
			ret = ret && Write(const_cast<char *>(value), len);
		}
		break;
	case LUA_TTABLE:
		{
			int count = 0;
			lua_pushnil(L);
			while (lua_next(L, i))
			{
				++count;
				lua_pop(L, 1);
			}
			//out(fp, &count, sizeof(count), "count");
			ret = ret && Write(&count, 1);

			lua_pushnil(L);
			while (lua_next(L, i))
			{
				ret = ret && SaveValue(L, -2);	// key
				ret = ret && SaveValue(L, -1);	// value
				lua_pop(L, 1);
			}
		}
		break;
	case LUA_TUSERDATA:
		{
			if (lua_getmetatable(L, i))
			{
				lua_getfield(L, -1, "__type");
				unsigned long type = Hash(luaL_checkstring(L, -1));
				lua_pop(L, 2);

				//out(fp, &type, sizeof(type));
				ret = ret && Write(&type, sizeof(type));
				switch (type)
				{
				case 0x8f89e802 /* "Vector" */:
					//out(fp, GetVector(L, i), sizeof(Vector));
					ret = ret && Write(GetVector(L, i), sizeof(Vector));
					break;
				case 0x15c2f8ec /* "Matrix" */:
					//out(fp, GetMatrix(L, i), sizeof(Matrix));
					ret = ret && Write(GetMatrix(L, i), sizeof(Matrix));
					break;
			//	case 0x79fa9618 /* "AiPath" */:
			//		ret = ret && Write(GetAiPath(L, i), sizeof(AiPath));
			//		break;
				default:
					break;
				}
			}
		}
		break;

	default:
		break;
	}

	return ret;
}

DLLBase * BuildMission(void)
{
	return new LuaMission();
}

void LuaMission::Setup(void)
{
	DidSetup = true;

	if (!L)
		return;

	// if the script has a Start function...
	lua_getglobal(L, "Start");
	if (lua_isfunction(L, -1))
	{
		// call the Start function
		LuaCheckStatus(lua_pcall(L, 0, 0, 0), L, "Lua script Start error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}

}

void LuaMission::InitialSetup(void)
{
	// Setup some initial stuff.
	EnableHighTPS(m_GameTPS);

	FormatConsoleMessage("Battlezone 2 LuaMission DLL created by %s", DLLAuthors);

	SetAutoGroupUnits(false);

	Difficulty = IFace_GetInteger("options.play.difficulty");
	StopScript = GetVarItemInt("network.session.ivar119");

/*
	Here's where you set the values at the start.  
*/

//  bools

//  floats

//  handles

	if(StopScript)
	{
		for(int i = 0; i < MAX_TEAMS; i++)
			for(int x = 0; x < MAX_TEAMS; x++)
				if(i != x)
					Ally(i,x);
	}

	// Load Lua Stuff here?

	// Lua file initial loading. Look for TRN value first, then map name.
	char script_filename[2 * MAX_ODF_LENGTH] = {0};

	if(IsNetworkOn())
	{
		const char *msn_filename = GetVarItemStr("network.session.svar0");
		strcpy_s(script_filename, msn_filename);
	}
	else
	{
		// Look for a script named after the mission file
		const char *TrnFileName = GetMapTRNFilename();
		strcpy_s(script_filename, TrnFileName);
	}

	if (char *dot = strchr(script_filename, '.'))
		*dot = '\0';
	strcat_s(script_filename, ".lua");

	// Skip setup if there isn't a script
	if (!DoesFileExist(script_filename))
	{
		FormatConsoleMessage("No Lua script: '%s'", script_filename);
		return;
	}

	// Create a new Lua state with default memory allocator.
	L = luaL_newstate();
	if (!L)
		return;

	// Load standard libraries
	// (excluding io and os for security; see linit.c)
	luaL_openlibs(L);

	// Register a panic handler
	lua_atpanic(L, LuaPanic);

	// If running in a debugger...
//	if (IsDebuggerPresent())
//	{
		// Hijack the print function
		lua_register(L, "print", LuaPrint);
//	}

	// Register our functions
	// (into the global table)
	lua_pushglobaltable(L);
	luaL_setfuncs(L, sLuaScriptUtils, 0);

	// Create a metatable for handles
	lua_pushlightuserdata(L, NULL);
    luaL_newmetatable(L, "Handle");
    lua_pushstring(L, "Handle");
    lua_setfield(L, -2, "__type");
    lua_pushcfunction(L, Handle_ToString);
    lua_setfield(L, -2, "__tostring");
    lua_setmetatable(L, -2);
	
	/*
	// Create a metatable for ParameterDB
	luaL_newmetatable(L, "ParameterDB");
	lua_pushcfunction(L, CloseODF);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);
	*/

	// Create a metatable for Vector
	luaL_newmetatable(L, "Vector");
	lua_pushstring(L, "Vector");
	lua_setfield(L, -2, "__type");
	lua_pushcfunction(L, Vector_Index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, Vector_NewIndex);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, Vector_Neg);
	lua_setfield(L, -2, "__unm");
	lua_pushcfunction(L, Vector_Add);
	lua_setfield(L, -2, "__add");
	lua_pushcfunction(L, Vector_Sub);
	lua_setfield(L, -2, "__sub");
	lua_pushcfunction(L, Vector_Mul);
	lua_setfield(L, -2, "__mul");
	lua_pushcfunction(L, Vector_Div);
	lua_setfield(L, -2, "__div");
	lua_pushcfunction(L, Vector_Eq);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, Vector_ToString);
	lua_setfield(L, -2, "__tostring");
	// Vector is a struct so it doesn't need __gc
	lua_pop(L, 1);

	// Create a metatable for Matrix
	luaL_newmetatable(L, "Matrix");
	lua_pushstring(L, "Matrix");
	lua_setfield(L, -2, "__type");
	lua_pushcfunction(L, Matrix_Index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, Matrix_NewIndex);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, Matrix_Mul);
	lua_setfield(L, -2, "__mul");
	lua_pushcfunction(L, Matrix_ToString);
	lua_setfield(L, -2, "__tostring");
	// Matrix is a struct so it doesn't need __gc
	lua_pop(L, 1);

	/*
	// Create a metatable for AiPaths
	luaL_newmetatable(L, "AiPath");
	lua_pushstring(L, "AiPath");
	lua_setfield(L, -2, "__type");
	lua_pushcfunction(L, FreeAiPath);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);
	*/

	// identity matrix
	*NewMatrix(L) = Identity_Matrix;
	lua_setglobal(L, "IdentityMatrix");

	// ai commands
	lua_createtable(L, NUM_CMD, NUM_CMD);
	for (int cmd = 0; cmd < NUM_CMD; ++cmd)
	{
		lua_pushnumber(L, cmd);
		lua_pushstring(L, CommandList[cmd]);
		lua_pushvalue(L, -1);
		lua_pushvalue(L, -3);
		lua_rawset(L, -5);
		lua_rawset(L, -3);
	}
	lua_setglobal(L, "AiCommand");

	/*
	// get file data
	const char *buf = static_cast<const char *>(UseItem(script_filename));
	long len = GetItemSize(script_filename);
	*/

	char* FileBuffer;
	size_t bufSize = 0;
	LoadFile(script_filename, NULL, bufSize);
	FileBuffer = static_cast<char *>(malloc(bufSize+1));
	LoadFile(script_filename, FileBuffer, bufSize);
	FileBuffer[bufSize] = '\0';

	// load and run the script
	LuaCheckStatus(luaL_loadbuffer(L, FileBuffer, bufSize, script_filename), L, "Lua script load error: %s") &&
	LuaCheckStatus(lua_pcall(L, 0, LUA_MULTRET, 0), L, "Lua script run error: %s");

	// release file data
	free(FileBuffer);

	if (!L)
		return;

	FormatConsoleMessage("Lua script: '%s' loaded", script_filename);

	// if the script has an Initial Setup function
	lua_getglobal(L, "InitialSetup");
	if (lua_isfunction(L, -1))
	{
		// call the InitialSetup function.
		LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script InitialSetup error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

bool LuaMission::Load(bool missionSave)
{
	bool ret = true;

	if (missionSave) {
		int i = 0;

		// init bools
		for (i = 0; i < b_count; i++)
			b_array[i] = false;

		/*
		// init floats
		for (i = 0; i < f_count; i++)
			f_array[i] = 99999.0f; // Speical for SP Mission.

		// init handles
		for (i = 0; i < h_count; i++)
			h_array[i] = 0;
		*/

		// init ints
		for (i = 0; i < i_count; i++)
			i_array[i] = 0;

//		Setup();
		return true;
	}

	// bools
	if(b_count)
		ret = ret && Read(b_array, b_count);

	/*
	// floats
	if(f_count)
		ret = ret && Read(f_array, f_count);

	// Handles
	if(h_count)
		ret = ret && Read(h_array, h_count);
	*/

	// ints
	if(i_count)
		ret = ret && Read(i_array, i_count);

	if (L)
	{
		// if the script has a Load function...
		lua_getglobal(L, "Load");
		if (lua_isfunction(L, -1))
		{
			// read argument values from the file
			int count = 0;
			ret = ret && Read(&count, 1);
			for (int i = 0; i < count; ++i)
			{
				ret = ret && LoadValue(L, true);
			}

			// call the Load function
			LuaCheckStatus(lua_pcall(L, count, 0, 0), L, "Lua script Load error: %s");
		}
		else
		{
			// skip argument values from the file
			// to prevent a missing Load function
			// from derailing the entire load process
			int count = 0;
			ret = ret && Read(&count, 1);
			for (int i = 0; i < count; ++i)
			{
				ret = ret && LoadValue(L, false);
			}

			lua_pop(L, 1);
		}
	}

	return ret;
}

bool LuaMission::PostLoad(bool missionSave)
{
	bool ret = true;

	if (missionSave)
		return true;

	ConvertHandles(h_array, h_count);

	return ret;
}

bool LuaMission::Save(bool missionSave)
{
	bool ret = true;

	if (missionSave)
		return true;
	
	// bools
	if(b_count)
		ret = ret && Write(b_array, b_count);

	/*
	// floats
	if(f_count)
		ret = ret && Write(f_array, f_count);

	// Handles
	if(h_count)
		ret = ret && Write(h_array, h_count);
	*/

	// ints
	if(i_count)
		ret = ret && Write(i_array, i_count);

	if (L)
	{
		// if the script has a Save function...
		lua_getglobal(L, "Save");
		if (lua_isfunction(L, -1))
		{
			// call the Save function
			int level = lua_gettop(L);
			if (LuaCheckStatus(lua_pcall(L, 0, LUA_MULTRET, 0), L, "Lua script Save error: %s"))
			{
				// write return values to the file
				int count = lua_gettop(L) - level + 1;
				ret = ret && Write(&count, 1);
				for (int i = level; i < level + count; ++i)
				{
					ret = ret && SaveValue(L, i);
				}
			}
		}
		else
		{
			// write zero return values
			int count = 0;
			ret = ret && Write(&count, 1);

			lua_pop(L, 1);
		}
	}

	return ret;
}

void LuaMission::PreOrdnanceHit(Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF)
{
	if(!L)
		return;

	// if the script has a PreOrdnanceHit function...
	lua_getglobal(L, "PreOrdnanceHit");
	if (lua_isfunction(L, -1))
	{
		// call the PreOrdnanceHit function
		PushHandle(L, shooterHandle);
		PushHandle(L, victimHandle);
		lua_pushinteger(L, ordnanceTeam);
		lua_pushstring(L, pOrdnanceODF);
		LuaCheckStatus(lua_pcall(L, 4, 0, 0), L, "Lua script PreOrdnanceHit error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

PreGetInReturnCodes LuaMission::PreGetIn(const int curWorld, Handle pilotHandle, Handle emptyCraftHandle)
{
	// Default value;
	PreGetInReturnCodes returnvalue = PREGETIN_ALLOW;

	// Use Default.
	if(!L)
		return returnvalue;

	// if the script has a PreGetIn function...
	lua_getglobal(L, "PreGetIn");
	if (lua_isfunction(L, -1))
	{
		// call the PreGetIn function
		lua_pushnumber(L, curWorld);
		PushHandle(L, pilotHandle);
		PushHandle(L, emptyCraftHandle);
		LuaCheckStatus(lua_pcall(L, 3, 1, 0), L, "Lua script PreGetIn error: %s");

		PreGetInReturnCodes returnvalue2 = PreGetInReturnCodes(luaL_optinteger(L, 1, returnvalue));

		if(returnvalue2 != PREGETIN_ALLOW) // If the Lua return was different then the Default behavior, let Lua Override.
			returnvalue = returnvalue2;

		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}
PreSnipeReturnCodes LuaMission::PreSnipe(const int curWorld, Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF)
{
	// Default value.
	PreSnipeReturnCodes returnvalue = PRESNIPE_KILLPILOT;

	if(!L)
		return returnvalue;

	// if the script has a PreSnipe function...
	lua_getglobal(L, "PreSnipe");
	if (lua_isfunction(L, -1))
	{
		// call the PreSnipe function
		lua_pushinteger(L, curWorld);
		PushHandle(L, shooterHandle);
		PushHandle(L, victimHandle);
		lua_pushinteger(L, ordnanceTeam);
		lua_pushstring(L, pOrdnanceODF);
		LuaCheckStatus(lua_pcall(L, 5, 1, 0), L, "Lua script PreSnipe error: %s");
		PreSnipeReturnCodes returnvalue2 = PreSnipeReturnCodes(luaL_optinteger(L, 1, returnvalue));

		if(returnvalue2 != PRESNIPE_KILLPILOT) // If the Lua return was different then the Default behavior, let Lua Override.
			returnvalue = returnvalue2;

		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}

PrePickupPowerupReturnCodes LuaMission::PrePickupPowerup(const int curWorld, Handle me, Handle powerupHandle)
{
	// Default.
	PrePickupPowerupReturnCodes returnvalue = PREPICKUPPOWERUP_ALLOW;

	if (!L)
		return returnvalue;

	// if the script has a PrePickupPowerup function...
	lua_getglobal(L, "PrePickupPowerup");
	if (lua_isfunction(L, -1))
	{
		// call the PrePickupPowerup function
		lua_pushinteger(L, curWorld);
		PushHandle(L, me);
		PushHandle(L, powerupHandle);
		LuaCheckStatus(lua_pcall(L, 3, 1, 0), L, "Lua script PrePickupPowerup error: %s");
		PrePickupPowerupReturnCodes returnvalue2 = PrePickupPowerupReturnCodes(luaL_optinteger(L, 1, returnvalue));

		if(returnvalue2 != PREPICKUPPOWERUP_ALLOW) // If the Lua return was different then the Default behavior, let Lua Override.
			returnvalue = returnvalue2;

		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}

// Notification to the DLL: called when a pilot/craft has changed targets
void LuaMission::PostTargetChangedCallback(Handle craft, Handle previousTarget, Handle currentTarget)
{
	// Default handler for this function: no-operation.  If a derived
	// class overrides this function, it can implement custom logic.
	// if the script has an Update function...
	if (!L)
		return;

	lua_getglobal(L, "PostTargetChangeCallback");
	if (lua_isfunction(L, -1))
	{
		// call the PostTargetChangeCallback function
		PushHandle(L, craft);
		PushHandle(L, previousTarget);
		PushHandle(L, currentTarget);
		LuaCheckStatus(lua_pcall(L, 3, 0, 0), L, "Lua script PostTargetChangeCallback error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

void LuaMission::AddObject(Handle h)
{
	//and this -GBD
	if(StopScript)
		return;

	if (!L)
		return;

	// Add CreateObject in ontop of AddObject, since BZ2 doesn't have CreateObject.
	lua_getglobal(L, "CreateObject");
	if (lua_isfunction(L, -1))
	{
		// call the CreateObject function with the game object handle
		PushHandle(L, h);
		LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script CreateObject error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}

	// if the script has an AddObject function
	lua_getglobal(L, "AddObject");
	if (lua_isfunction(L, -1))
	{
		// call the AddObject function with the game object handle
		PushHandle(L, h);
		LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script AddObject error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

void LuaMission::DeleteObject(Handle h)
{
	//and this -GBD
	if(StopScript)
		return;

	if (!L)
		return;

	// if the script has a DeleteObject function
	lua_getglobal(L, "DeleteObject");
	if (lua_isfunction(L, -1))
	{
		// call the DeleteObject function with the game object handle
		PushHandle(L, h);
		LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script DeleteObject error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

bool LuaMission::AddPlayer(DPID id, int Team, bool IsNewPlayer)
{
	if(!DidSetup)
		Setup();

	if (!L)
		return true;

	bool returnvalue = true;

	// if the script has an AddPlayer function...
	lua_getglobal(L, "AddPlayer");
	if (lua_isfunction(L, -1))
	{
		// call the AddPlayer function
		lua_pushinteger(L, id);
		lua_pushinteger(L, Team);
		lua_pushboolean(L, IsNewPlayer);
		LuaCheckStatus(lua_pcall(L, 3, 1, 0), L, "Lua script AddPlayer error: %s");
		returnvalue = luaL_optboolean(L, 1, returnvalue);
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return true;
}

void LuaMission::DeletePlayer(DPID id)
{
	if (!L)
		return;

	// if the script has an Update function...
	lua_getglobal(L, "DeletePlayer");
	if (lua_isfunction(L, -1))
	{
		// call the Update function
		lua_pushinteger(L, id);
		LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script DeletePlayer error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

EjectKillRetCodes LuaMission::PlayerEjected(Handle DeadObjectHandle)
{
	if (!L)
		return DoEjectPilot;

	EjectKillRetCodes returnvalue = DoEjectPilot;

	// if the script has a PlayerEjected function...
	lua_getglobal(L, "PlayerEjected");
	if (lua_isfunction(L, -1))
	{
		// call the PlayerEjected function
		PushHandle(L, DeadObjectHandle);
		LuaCheckStatus(lua_pcall(L, 1, 1, 0), L, "Lua script PlayerEjected error: %s");
		returnvalue = EjectKillRetCodes(luaL_optinteger(L, 1, returnvalue));
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}

EjectKillRetCodes LuaMission::ObjectKilled(int DeadObjectHandle, int KillersHandle)
{
	if (!L)
	{
		if(!IsPlayer(DeadObjectHandle)) // J.Random AI-controlled Object is toast
			return DLLHandled;
		else // Player dead
			return DoEjectPilot;
	}

	EjectKillRetCodes returnvalue = IsPlayer(DeadObjectHandle) ? DoEjectPilot : DLLHandled;

	// if the script has an ObjectKilled function...
	lua_getglobal(L, "ObjectKilled");
	if (lua_isfunction(L, -1))
	{
		// call the ObjectKilled function
		PushHandle(L, DeadObjectHandle);
		PushHandle(L, KillersHandle);
		LuaCheckStatus(lua_pcall(L, 2, 1, 0), L, "Lua script ObjectKilled error: %s");
		returnvalue = EjectKillRetCodes(luaL_optinteger(L, 1, returnvalue));
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}
	
	return returnvalue;
}

EjectKillRetCodes LuaMission::ObjectSniped(int DeadObjectHandle, int KillersHandle)
{
	if (!L)
	{	
		if(!IsPlayer(DeadObjectHandle)) // J.Random AI-controlled Object is toast
			return DLLHandled;
		else // Player dead
			return DoGameOver; //DoRespawnSafest;
	}

	EjectKillRetCodes returnvalue = IsPlayer(DeadObjectHandle) ? DoGameOver : DLLHandled;

	// if the script has an ObjectSniped function...
	lua_getglobal(L, "ObjectSniped");
	if (lua_isfunction(L, -1))
	{
		// call the ObjectSniped function
		PushHandle(L, DeadObjectHandle);
		PushHandle(L, KillersHandle);
		LuaCheckStatus(lua_pcall(L, 2, 1, 0), L, "Lua script ObjectSniped error: %s");
		returnvalue = EjectKillRetCodes(luaL_optinteger(L, 1, returnvalue));
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}


void LuaMission::Execute(void)
{
	// Do init first! -GBD
	if (!DidSetup)
		Setup();

	// Count the turns each tick. Multiply or Divide by m_GameTPS to translate into seconds. (divide to turn m_ElapsedGameTime into seconds,
	// Multiply # of seconds by m_GameTPS to turn it into ticks per second.
	m_ElapsedGameTime++;

	if(!StopScript)
	{

/*
	Here is where you put what happens every frame.  
*/

		if (!L)
			return;

		// if the script has an Update function...
		lua_getglobal(L, "Update");
		if (lua_isfunction(L, -1))
		{
			// call the Update function
			LuaCheckStatus(lua_pcall(L, 0, 0, 0), L, "Lua script Update error: %s");
		}
		else
		{
			lua_pop(L, 1);
		}

	}
}

void LuaMission::PostRun(void)
{
	if(!StopScript)
	{
		if (!L)
			return;

		// if the script has a PostRun function...
		lua_getglobal(L, "PostRun");
		if (lua_isfunction(L, -1))
		{
			// call the PostRun function
			LuaCheckStatus(lua_pcall(L, 0, 0, 0), L, "Lua script PostRun error: '%s'");
		}
		else
		{
			lua_pop(L, 1);
		}

	}

}