/* 

pool - hierarchic storage object for PD and Max/MSP

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "pool.h"

pooldata::pooldata(const S *s):
	sym(s),nxt(NULL),refs(0)
{
	LOG1("new pool %s",sym?flext_base::GetString(sym):"<private>");
}

pooldata::~pooldata()
{
	LOG1("free pool %s",sym?flext_base::GetString(sym):"<private>");
}

V pooldata::Reset()
{
}

bool pooldata::MkDir(const AtomList &d)
{
	return false;
}

bool pooldata::ChkDir(const AtomList &d)
{
	return false;
}

bool pooldata::RmDir(const AtomList &d)
{
	return false;
}

bool pooldata::SvDir(const AtomList &d,const C *flnm)
{
	return false;
}

bool pooldata::Set(const AtomList &d,const S *key,AtomList *data)
{
	return false;
}

bool pooldata::Clr(const AtomList &d,const S *key)
{
	return false;
}

bool pooldata::ClrAll(const AtomList &d)
{
	return false;
}

AtomList *pooldata::Get(const AtomList &d,const S *key)
{
	return NULL;
}

I pooldata::GetAll(const AtomList &d,AtomList *&lst)
{
	lst = NULL;
	return 0;
}

bool pooldata::Load(const C *flnm)
{
	return false;
}

bool pooldata::Save(const C *flnm)
{
	return false;
}



