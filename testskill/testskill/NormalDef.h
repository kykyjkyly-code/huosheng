#if 0
#ifndef NORMALDEF_H
#define NORMALDEF_H
#include "src\utils\PlayerTask.h"
class NormalDef
{
public:
	NormalDef();
	~NormalDef();
	PlayerTask plan(int id);
private:
	
};
typedef Singleton<NormalDef> normalDef;

#endif
#endif