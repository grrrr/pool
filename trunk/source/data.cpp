/* 

pool - hierarchical storage object for PD and Max/MSP

Copyright (c) 2002-2004 Thomas Grill
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "pool.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fstream>

using namespace std;

pooldata::pooldata(const S *s,I vcnt,I dcnt):
	sym(s),nxt(NULL),refs(0),
    root(pooldir::New(nullatom,NULL,vcnt,dcnt))
{
	FLEXT_LOG1("new pool %s",sym?flext::GetString(sym):"<private>");
}

pooldata::~pooldata()
{
	FLEXT_LOG1("free pool %s",sym?flext::GetString(sym):"<private>");
    pooldir::Free(root);
}

const A pooldata::nullatom = { A_NULL };


pooldir *pooldata::GetDir(dirref &d) 
{ 
    pooldir *ret = d.Ptr();
    if(!ret) {
        ret = root->GetDir(d.Dir());
        d.Assoc(ret);
    }
    return ret;
}

I pooldata::GetAll(dirref &d,A *&keys,AtomList *&lst)
{
	pooldir *pd = GetDir(d);
	if(pd)
		return pd->GetAll(keys,lst);
	else {
		keys = NULL; lst = NULL;
		return 0;
	}
}

I pooldata::PrintAll(dirref &d)
{
    char tmp[1024];
    d.Dir().Print(tmp,sizeof tmp);
    pooldir *pd = GetDir(d);
    strcat(tmp," , ");
	return pd?pd->PrintAll(tmp,sizeof tmp):0;
}

I pooldata::GetSub(dirref &d,const t_atom **&dirs)
{
	pooldir *pd = GetDir(d);
	if(pd)
		return pd->GetSub(dirs);
	else {
		dirs = NULL;
		return 0;
	}
}


BL pooldata::Paste(dirref &d,const pooldir *clip,I depth,BL repl,BL mkdir)
{
	pooldir *pd = GetDir(d);
	return pd && pd->Paste(clip,depth,repl,mkdir);
}

pooldir *pooldata::Copy(dirref &d,const A &key,BL cut)
{
	pooldir *pd = GetDir(d);
	if(pd) {
		AtomList *val = pd->GetVal(key,cut);
		if(val) {
            pooldir *ret = pooldir::New(nullatom,NULL,pd->VSize(),pd->DSize());
			ret->SetVal(key,val);
			return ret;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

pooldir *pooldata::CopyAll(dirref &d,I depth,BL cut)
{
	pooldir *pd = GetDir(d);
	if(pd) {
		// What sizes should we choose here?
        pooldir *ret = pooldir::New(nullatom,NULL,pd->VSize(),pd->DSize());
		if(pd->Copy(ret,depth,cut))
			return ret;
		else {
            pooldir::Free(ret);
			return NULL;
		}
	}
	else
		return NULL;
}


static const C *CnvFlnm(C *dst,const C *src,I sz)
{
#if FLEXT_SYS == FLEXT_SYS_PD && FLEXT_OS == FLEXT_OS_WIN
	I i,cnt = strlen(src);
	if(cnt >= sz-1) return NULL;
	for(i = 0; i < cnt; ++i)
		dst[i] = src[i] != '/'?src[i]:'\\';
	dst[i] = 0;
	return dst;
#else
	return src;
#endif
}

BL pooldata::LdDir(dirref &d,const C *flnm,I depth,BL mkdir)
{
	pooldir *pd = GetDir(d);
	if(pd) {
		C tmp[1024];
		const C *t = CnvFlnm(tmp,flnm,sizeof tmp);
		if(t) {
			ifstream fl(t);
			return fl.good() && pd->LdDir(fl,depth,mkdir);
		}
		else return false;
	}
	else
		return false;
}

BL pooldata::SvDir(dirref &d,const C *flnm,I depth,BL absdir)
{
	pooldir *pd = GetDir(d);
	if(pd) {
		C tmp[1024];
		const C *t = CnvFlnm(tmp,flnm,sizeof tmp);
		if(t) {
			ofstream fl(t);
			AtomList tmp;
			if(absdir) tmp = d.Dir();
			return fl.good() && pd->SvDir(fl,depth,tmp);
		}
		else return false;
	}
	else
		return false;
}

BL pooldata::LdDirXML(dirref &d,const C *flnm,I depth,BL mkdir)
{
	pooldir *pd = GetDir(d);
	if(pd) {
		C tmp[1024];
		const C *t = CnvFlnm(tmp,flnm,sizeof tmp);
		if(t) {
			ifstream fl(t);
            BL ret = fl.good() != 0;
            if(ret) {
                fl.getline(tmp,sizeof tmp);
                ret = !strncmp(tmp,"<?xml",5);
            }
/*
            if(ret) {
                fl.getline(tmp,sizeof tmp);
                // DOCTYPE need not be present / only external DOCTYPE is allowed!
                ret = !strncmp(tmp,"<!DOCTYPE",9);
            }
*/
            if(ret)
                ret = pd->LdDirXML(fl,depth,mkdir);
            return ret;
		}
	}
    return false;
}

BL pooldata::SvDirXML(dirref &d,const C *flnm,I depth,BL absdir)
{
	pooldir *pd = GetDir(d);
	if(pd) {
		C tmp[1024];
		const C *t = CnvFlnm(tmp,flnm,sizeof tmp);
		if(t) {
			ofstream fl(t);
			AtomList tmp;
			if(absdir) tmp = d.Dir();
            if(fl.good()) {
                fl << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>" << endl;
                fl << "<!DOCTYPE pool SYSTEM \"http://grrrr.org/ext/pool/pool-0.2.dtd\">" << endl;
                fl << "<pool>" << endl;
                BL ret = pd->SvDirXML(fl,depth,tmp);
                fl << "</pool>" << endl;
                return ret;
            }
		}
	}
    return false;
}
