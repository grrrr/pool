/* 

pool - hierarchic storage object for PD and Max/MSP

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "pool.h"

inline t_atom &SetAtom(t_atom &dst,const t_atom &src) { flext_base::CopyAtom(&dst,&src); return dst; }

inline I compare(I a,I b) { return a == b?0:(a < b?-1:1); }
inline I compare(F a,F b) { return a == b?0:(a < b?-1:1); }

static I compare(const S *a,const S *b) 
{
	if(a == b)
		return 0;
	else
		return a < b?-1:1;
}

static I compare(const A &a,const A &b) 
{
	if(a.a_type == b.a_type) {
		switch(a.a_type) {
		case A_NULL:
			return 0;
		case A_FLOAT:
			return compare(a.a_w.w_float,b.a_w.w_float);
#ifdef MAXMSP
		case A_LONG:
			return compare(a.a_w.w_long,b.a_w.w_long);
#endif
		case A_SYMBOL:
			return compare(a.a_w.w_symbol,b.a_w.w_symbol);
#ifdef PD
		case A_POINTER:
			return a.a_w.w_gpointer == b.a_w.w_gpointer?0:(a.a_w.w_gpointer < b.a_w.w_gpointer?-1:1);
#endif
#ifdef _DEBUG
		default:
			post("pool - atom comparison: type not handled");
			return -1;
#endif
		}
	}
	else
		return a.a_type < b.a_type?-1:1;
}


poolval::poolval(const A &k,AtomList *d):
	data(d),nxt(NULL)
{
	SetAtom(key,k);
}

poolval::~poolval()
{
	if(data) delete data;
	if(nxt) delete nxt;
}

poolval &poolval::Set(AtomList *d)
{
	if(data) delete data;
	data = d;
	return *this;
}



pooldir::pooldir(const A &d):
	dirs(NULL),vals(NULL),nxt(NULL)
{
	flext_base::CopyAtom(&dir,&d);
}

pooldir::~pooldir()
{
	Clear(true);
	if(nxt) delete nxt;
}

V pooldir::Clear(BL rec)
{
	if(rec && dirs) { delete dirs; dirs = NULL; }
	if(vals) { delete vals; vals = NULL; }
}

V pooldir::AddDir(I argc,const A *argv)
{
	if(!argc) return;

	I c = 1;
	pooldir *prv = NULL,*ix = dirs;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(argv[0],ix->dir);
		if(c >= 0) break;
	}

	if(c || !ix) {
		pooldir *nd = new pooldir(argv[0]);
		nd->nxt = ix;

		if(prv) prv->nxt = nd;
		else dirs = nd;
		ix = nd;
	}

	ix->AddDir(argc-1,argv+1);
}

pooldir *pooldir::GetDir(I argc,const A *argv,BL rmv)
{
	if(!argc) return this;

	I c = 1;
	pooldir *prv = NULL,*ix = dirs;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(argv[0],ix->dir);
		if(c >= 0) break;
	}

	if(c || !ix) 
		return NULL;
	else {
		if(argc > 1)
			return ix->GetDir(argc-1,argv+1,rmv);
		else if(rmv) {
			pooldir *nd = ix->nxt;
			if(prv) prv->nxt = nd;
			else dirs = nd;
			ix->nxt = NULL;
			return ix;
		}
		else 
			return NULL;
	}
}

BL pooldir::DelDir(const AtomList &d)
{
	pooldir *pd = GetDir(d,true);
	if(pd) {
		delete pd;
		return true;
	}
	else 
		return false;
}

V pooldir::SetVal(const A &key,AtomList *data)
{
	I c = 1;
	poolval *prv = NULL,*ix = vals;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c >= 0) break;
	}

	if(c || !ix) {
		if(data) {
			poolval *nv = new poolval(key,data);
			nv->nxt = ix;

			if(prv) prv->nxt = nv;
			else vals = nv;
		}
	}
	else
		if(data)
			ix->Set(data);
		else {
			poolval *nv = ix->nxt;
			if(prv) prv->nxt = nv;
			else vals = nv;
			ix->nxt = NULL;
			delete ix;
		}
}

AtomList *pooldir::GetVal(const A &key)
{
	I c = 1;
	poolval *ix = vals;
	for(; ix; ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c >= 0) break;
	}

	if(c || !ix) 
		return NULL;
	else
		return new AtomList(*ix->data);
}

I pooldir::GetAll(A *&keys,AtomList *&lst)
{
	if(!vals) { keys = NULL; lst = NULL; return 0; }

	I cnt = 0;
	poolval *ix = vals;
	for(; ix; ix = ix->nxt,++cnt);

	keys = new A[cnt];
	lst = new AtomList[cnt];

	ix = vals;
	for(I i = 0; ix; ix = ix->nxt,++i) {
		SetAtom(keys[i],ix->key);
		lst[i] = *ix->data;
	}

	return cnt;
}




pooldata::pooldata(const S *s):
	sym(s),nxt(NULL),refs(0),
	root(nullatom)
{
	LOG1("new pool %s",sym?flext_base::GetString(sym):"<private>");
}

pooldata::~pooldata()
{
	LOG1("free pool %s",sym?flext_base::GetString(sym):"<private>");
}

t_atom pooldata::nullatom = { A_NULL };


V pooldata::Reset()
{
	root.Clear(true);
}

bool pooldata::MkDir(const AtomList &d)
{
	root.AddDir(d);
	return true;
}

bool pooldata::ChkDir(const AtomList &d)
{
	return root.GetDir(d) != NULL;
}

bool pooldata::RmDir(const AtomList &d)
{
	return root.DelDir(d);
}

bool pooldata::SvDir(const AtomList &d,const C *flnm)
{
	return false;
}

bool pooldata::Set(const AtomList &d,const A &key,AtomList *data)
{
	pooldir *pd = root.GetDir(d);
	if(!pd) return false;
	pd->SetVal(key,data);
	return true;
}

bool pooldata::Clr(const AtomList &d,const A &key)
{
	pooldir *pd = root.GetDir(d);
	if(!pd) return false;
	pd->ClrVal(key);
	return true;
}

bool pooldata::ClrAll(const AtomList &d,BL rec)
{
	pooldir *pd = root.GetDir(d);
	if(!pd) return false;
	pd->Clear(rec);
	return true;
}

AtomList *pooldata::Get(const AtomList &d,const A &key)
{
	pooldir *pd = root.GetDir(d);
	return pd?pd->GetVal(key):NULL;
}

I pooldata::GetAll(const AtomList &d,A *&keys,AtomList *&lst)
{
	pooldir *pd = root.GetDir(d);
	return pd?pd->GetAll(keys,lst):0;
}

bool pooldata::Load(const C *flnm)
{
	return false;
}

bool pooldata::Save(const C *flnm)
{
	return false;
}



