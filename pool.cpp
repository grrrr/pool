/* 

pool - hierarchical storage object for PD and Max/MSP

Copyright (c) 2002-2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "pool.h"

#include <string.h>
#include <fstream.h>
#include <ctype.h>
#include <stdlib.h>

#define VBITS 6
#define DBITS 5

inline I compare(I a,I b) { return a == b?0:(a < b?-1:1); }
inline I compare(F a,F b) { return a == b?0:(a < b?-1:1); }

static I compare(const S *a,const S *b) 
{
	if(a == b)
		return 0;
	else
		return strcmp(flext::GetString(a),flext::GetString(b));
}

static I compare(const A &a,const A &b) 
{
	if(flext::GetType(a) == flext::GetType(b)) {
		switch(flext::GetType(a)) {
		case A_FLOAT:
			return compare(flext::GetFloat(a),flext::GetFloat(b));
#if FLEXT_SYS == FLEXT_SYS_MAX
		case A_LONG:
			return compare(flext::GetInt(a),flext::GetInt(b));
#endif
		case A_SYMBOL:
			return compare(flext::GetSymbol(a),flext::GetSymbol(b));
#if FLEXT_SYS == FLEXT_SYS_PD
		case A_POINTER:
			return flext::GetPointer(a) == flext::GetPointer(b)?0:(flext::GetPointer(a) < flext::GetPointer(b)?-1:1);
#endif
		default:
			LOG("pool - atom comparison: type not handled");
			return -1;
		}
	}
	else
		return flext::GetType(a) < flext::GetType(b)?-1:1;
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

poolval *poolval::Dup() const
{
	return new poolval(key,data?new AtomList(*data):NULL); 
}


pooldir::pooldir(const A &d,pooldir *p,I vcnt,I dcnt):
	parent(p),nxt(NULL),
	vbits(vcnt?Int2Bits(vcnt):VBITS),dbits(dcnt?Int2Bits(dcnt):DBITS),
	vsize(1<<vbits),dsize(1<<dbits)
{
	dirs = new direntry[dsize];
	ZeroMem(dirs,dsize*sizeof *dirs);
	vals = new valentry[vsize];
	ZeroMem(vals,vsize*sizeof *vals);

	CopyAtom(&dir,&d);
}

pooldir::~pooldir()
{
	Clear(true,false);
		
	if(nxt) delete nxt;
}

V pooldir::Clear(BL rec,BL dironly)
{
	if(rec && dirs) { 
		for(I i = 0; i < dsize; ++i) if(dirs[i].d) delete dirs[i].d;
		delete[] dirs; dirs = NULL;
	}
	if(!dironly && vals) { 
		for(I i = 0; i < vsize; ++i) if(vals[i].v) delete vals[i].v;
		delete[] vals; vals = NULL;
	}
}

pooldir *pooldir::AddDir(I argc,const A *argv,I vcnt,I dcnt)
{
	if(!argc) return this;

	I c = 1,dix = DIdx(argv[0]);
	pooldir *prv = NULL,*ix = dirs[dix].d;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(argv[0],ix->dir);
		if(c <= 0) break;
	}

	if(c || !ix) {
		pooldir *nd = new pooldir(argv[0],this,vcnt,dcnt);
		nd->nxt = ix;

		if(prv) prv->nxt = nd;
		else dirs[dix].d = nd;
		dirs[dix].cnt++;
		ix = nd;
	}

	return ix->AddDir(argc-1,argv+1);
}

pooldir *pooldir::GetDir(I argc,const A *argv,BL rmv)
{
	if(!argc) return this;

	I c = 1,dix = DIdx(argv[0]);
	pooldir *prv = NULL,*ix = dirs[dix].d;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(argv[0],ix->dir);
		if(c <= 0) break;
	}

	if(c || !ix) 
		return NULL;
	else {
		if(argc > 1)
			return ix->GetDir(argc-1,argv+1,rmv);
		else if(rmv) {
			pooldir *nd = ix->nxt;
			if(prv) prv->nxt = nd;
			else dirs[dix].d = nd;
			dirs[dix].cnt--;
			ix->nxt = NULL;
			return ix;
		}
		else 
			return ix;
	}
}

BL pooldir::DelDir(I argc,const A *argv)
{
	pooldir *pd = GetDir(argc,argv,true);
	if(pd && pd != this) {
		delete pd;
		return true;
	}
	else 
		return false;
}

V pooldir::SetVal(const A &key,AtomList *data,BL over)
{
	I c = 1,vix = VIdx(key);
	poolval *prv = NULL,*ix = vals[vix].v;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c <= 0) break;
	}

	if(c || !ix) {
		// no existing data found
	
		if(data) {
			poolval *nv = new poolval(key,data);
			nv->nxt = ix;

			if(prv) prv->nxt = nv;
			else vals[vix].v = nv;
			vals[vix].cnt++;
		}
	}
	else if(over) { 
		// data exists... only set if overwriting enabled
		
		if(data)
			ix->Set(data);
		else {
			// delete key
		
			poolval *nv = ix->nxt;
			if(prv) prv->nxt = nv;
			else vals[vix].v = nv;
			vals[vix].cnt--;
			ix->nxt = NULL;
			delete ix;
		}
	}
}

poolval *pooldir::RefVal(const A &key)
{
	I c = 1,vix = VIdx(key);
	poolval *ix = vals[vix].v;
	for(; ix; ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c <= 0) break;
	}

	return c || !ix?NULL:ix;
}


poolval *pooldir::RefVali(I rix)
{
	for(I vix = 0; vix < vsize; ++vix) 
		if(rix > vals[vix].cnt) rix -= vals[vix].cnt;
		else {
			poolval *ix = vals[vix].v;
			for(; ix && rix; ix = ix->nxt) --rix;
			if(ix && !rix) return ix;
		}
	return NULL;
}

flext::AtomList *pooldir::PeekVal(const A &key)
{
	poolval *ix = RefVal(key);
	return ix?ix->data:NULL;
}

flext::AtomList *pooldir::GetVal(const A &key,BL cut)
{
	I c = 1,vix = VIdx(key);
	poolval *prv = NULL,*ix = vals[vix].v;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c <= 0) break;
	}

	if(c || !ix) 
		return NULL;
	else {
		AtomList *ret;
		if(cut) {
			poolval *nv = ix->nxt;
			if(prv) prv->nxt = nv;
			else vals[vix].v = nv;
			vals[vix].cnt--;
			ix->nxt = NULL;
			ret = ix->data; ix->data = NULL;
			delete ix;
		}
		else
			ret = new AtomList(*ix->data);
		return ret;
	}
}

I pooldir::CntAll() const
{
	I cnt = 0;
	for(I vix = 0; vix < vsize; ++vix) cnt += vals[vix].cnt;
	return cnt;
}

I pooldir::GetKeys(AtomList &keys)
{
	I cnt = CntAll();
	keys(cnt);

	for(I vix = 0; vix < vsize; ++vix) {
		poolval *ix = vals[vix].v;
		for(I i = 0; ix; ++i,ix = ix->nxt) 
			SetAtom(keys[i],ix->key);
	}
	return cnt;
}

I pooldir::GetAll(A *&keys,AtomList *&lst,BL cut)
{
	I cnt = CntAll();
	keys = new A[cnt];
	lst = new AtomList[cnt];

	for(I i = 0,vix = 0; vix < vsize; ++vix) {
		poolval *ix = vals[vix].v;
		for(; ix; ++i) {
			SetAtom(keys[i],ix->key);
			lst[i] = *ix->data;

			if(cut) {
				poolval *t = ix;
				vals[vix].v = ix = ix->nxt;
				vals[vix].cnt--;				
				t->nxt = NULL; delete t;
			}
			else
				ix = ix->nxt;
		}
	}
	return cnt;
}


I pooldir::CntSub() const
{
	I cnt = 0;
	for(I dix = 0; dix < dsize; ++dix) cnt += dirs[dix].cnt;
	return cnt;
}


I pooldir::GetSub(const A **&lst)
{
	const I cnt = CntSub();
	lst = new const A *[cnt];
	for(I i = 0,dix = 0; dix < dsize; ++dix) {
		pooldir *ix = dirs[dix].d;
		for(; ix; ix = ix->nxt) lst[i++] = &ix->dir;
	}
	return cnt;
}


BL pooldir::Paste(const pooldir *p,I depth,BL repl,BL mkdir)
{
	BL ok = true;

	for(I di = 0; di < dsize; ++di) {
		for(poolval *ix = p->vals[di].v; ix; ix = ix->nxt) {
			SetVal(ix->key,new AtomList(*ix->data),repl);
		}
	}

	if(ok && depth) {
		for(I di = 0; di < dsize; ++di) {
			for(pooldir *dix = p->dirs[di].d; ok && dix; dix = dix->nxt) {
				pooldir *ndir = mkdir?AddDir(1,&dix->dir):GetDir(1,&dix->dir);
				if(ndir) { 
					ok = ndir->Paste(dix,depth > 0?depth-1:depth,repl,mkdir);
				}
			}
		}
	}

	return ok;
}

BL pooldir::Copy(pooldir *p,I depth,BL cut)
{
	BL ok = true;

	if(cut) {
		for(I vi = 0; vi < vsize; ++vi) {
			for(poolval *ix = vals[vi].v; ix; ix = ix->nxt)
				p->SetVal(ix->key,ix->data);
			vals[vi].cnt = 0;
			vals[vi].v = NULL;
		}
	}
	else {
		for(I vi = 0; vi < vsize; ++vi) {
			for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
				p->SetVal(ix->key,new AtomList(*ix->data));
			}
		}
	}

	if(ok && depth) {
		for(I di = 0; di < dsize; ++di) {
			for(pooldir *dix = dirs[di].d; ok && dix; dix = dix->nxt) {
				pooldir *ndir = p->AddDir(1,&dix->dir);
				if(ndir)
					ok = ndir->Copy(dix,depth > 0?depth-1:depth,cut);
				else
					ok = false;
			}
		}
	}

	return ok;
}


static C *ReadAtom(C *c,A *a)
{
	// skip whitespace
	while(*c && isspace(*c)) ++c;
	if(!*c) return NULL;

	const C *m = c; // remember position

	// check for word type (s = 0,1,2 ... int,float,symbol)
	I s = 0;
	for(; *c && !isspace(*c); ++c) {
		if(!isdigit(*c)) 
			s = (*c != '.' || s == 1)?2:1;
	}

	if(a) {
		switch(s) {
		case 0: // integer
#if FLEXT_SYS == FLEXT_SYS_MAX
			flext::SetInt(*a,atoi(m));
			break;
#endif
		case 1: // float
			flext::SetFloat(*a,(F)atof(m));
			break;
		default: { // anything else is a symbol
			C t = *c; *c = 0;
			flext::SetString(*a,m);
			*c = t;
			break;
		}
		}
	}

	return c;
}

static BL ReadAtoms(istream &is,flext::AtomList &l,C del)
{
	C tmp[1024];
	is.getline(tmp,sizeof tmp,del); 
	if(is.eof() || !is.good()) return false;

	I i,cnt;
	C *t = tmp;
	for(cnt = 0; ; ++cnt) {
		t = ReadAtom(t,NULL);
		if(!t) break;
	}

	l(cnt);
	if(cnt) {
		for(i = 0,t = tmp; i < cnt; ++i)
			t = ReadAtom(t,&l[i]);
	}
	return true;
}

static V WriteAtom(ostream &os,const A &a)
{
	switch(a.a_type) {
	case A_FLOAT:
		os << a.a_w.w_float;
		break;
#if FLEXT_SYS == FLEXT_SYS_MAX
	case A_LONG:
		os << a.a_w.w_long;
		break;
#endif
	case A_SYMBOL:
		os << flext::GetString(flext::GetSymbol(a));
		break;
	}
}

static V WriteAtoms(ostream &os,const flext::AtomList &l)
{
	for(I i = 0; i < l.Count(); ++i) {
		WriteAtom(os,l[i]);
		os << ' ';
	}
}

BL pooldir::LdDir(istream &is,I depth,BL mkdir)
{
	BL r;
	for(I i = 1; !is.eof(); ++i) {
		AtomList d,k,*v = new AtomList;
		r = ReadAtoms(is,d,',');
		r = r && ReadAtoms(is,k,',') && k.Count() == 1;
		r = r && ReadAtoms(is,*v,'\n') && v->Count();

		if(r) {
			if(depth < 0 || d.Count() <= depth) {
				pooldir *nd = mkdir?AddDir(d):GetDir(d);
				if(nd) {
					nd->SetVal(k[0],v); v = NULL;
				}
	#ifdef FLEXT_DEBUG
				else
					post("pool - directory was not found",i);
	#endif
			}
		}
		else if(!is.eof())
			post("pool - format mismatch encountered, skipped line %i",i);

		if(v) delete v;
	}
	return true;
}

BL pooldir::SvDir(ostream &os,I depth,const AtomList &dir)
{
	for(I vi = 0; vi < vsize; ++vi) {
		for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
			WriteAtoms(os,dir);
			os << ", ";
			WriteAtom(os,ix->key);
			os << " , ";
			WriteAtoms(os,*ix->data);
			os << endl;
		}
	}
	if(depth) {
		I nd = depth > 0?depth-1:-1;
		for(I di = 0; di < dsize; ++di) {
			for(pooldir *ix = dirs[di].d; ix; ix = ix->nxt) {
				ix->SvDir(os,nd,AtomList(dir).Append(ix->dir));
			}
		}
	}
	return true;
}




