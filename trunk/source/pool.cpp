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

using namespace std;

#define VBITS 6
#define DBITS 5

inline int compare(int a,int b) { return a == b?0:(a < b?-1:1); }
inline int compare(float a,float b) { return a == b?0:(a < b?-1:1); }

static int compare(const t_symbol *a,const t_symbol *b) 
{
	if(a == b)
		return 0;
	else
		return strcmp(flext::GetString(a),flext::GetString(b));
}

static int compare(const t_atom &a,const t_atom &b) 
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
			FLEXT_LOG("pool - atom comparison: type not handled");
			return -1;
		}
	}
	else
		return flext::GetType(a) < flext::GetType(b)?-1:1;
}


// --- dirref ------------------------------------------------

pooldir *dirref::Ptr() const 
{ 
    return ptr && ident == ptr->ident?ptr:NULL; 
}

dirref &dirref::operator =(const dirref &r) 
{ 
    if(ptr) pooldir::Unref(ptr);
    if(r.ptr) {
        pooldir::Ref(ptr = r.ptr); 
        ident = r.ident;
    }
    else
        ptr = NULL;
    dir = r.dir;
    return *this;
}

dirref &dirref::Set(int argc,const t_atom *argv)
{
    if(ptr) { pooldir::Unref(ptr); ptr = NULL; }
    dir(argc,argv);
    return *this;
}

dirref &dirref::Assoc(pooldir *p)
{
    if(ptr) pooldir::Unref(ptr);
    pooldir::Ref(ptr = p);
    ident = p->ident;
    return *this;
}

dirref &dirref::Append(int argc,const t_atom *argv)
{
    if(ptr) { pooldir::Unref(ptr); ptr = NULL; }
    dir.Append(argc,argv);
    return *this;
}

dirref &dirref::Part(int ix,int len)
{
    if(ptr) { pooldir::Unref(ptr); ptr = NULL; }
    dir.Part(ix,len);
    return *this;
}

// --- poolval ------------------------------------------------

poolval::poolval(const t_atom &k,AtomList *d):
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

// --- pooldir ------------------------------------------------

static size_t pooldir_cnt = 1;

pooldir *pooldir::New(const t_atom &dir,pooldir *parent,int vcnt,int dcnt)
{
    // \todo we could take it from a pool of already allocated pooldirs
    pooldir *ret = new pooldir(dir,parent,vcnt,dcnt);
    Ref(ret);
    return ret;
}

bool pooldir::Unref(pooldir *d)
{
    if(!d) return false;
    if(!--d->usage) {
        // mark as deleted
        d->ident = 0;
        // \todo we could throw it into a pool of held pooldirs
        delete d; 
        return false; 
    } 
    else return true;
}

pooldir::pooldir(const t_atom &d,pooldir *p,int vcnt,int dcnt):
	parent(p),nxt(NULL),
    ident(pooldir_cnt++),usage(0),
    vals(NULL),dirs(NULL),
	vbits(vcnt?Int2Bits(vcnt):VBITS),dbits(dcnt?Int2Bits(dcnt):DBITS),
	vsize(1<<vbits),dsize(1<<dbits)
{
	Reset();
	CopyAtom(&dir,&d);
}

pooldir::~pooldir()
{
    FLEXT_ASSERT(!usage && !ident && !nxt);
	Reset(false);
}

V pooldir::Clear(bool rec,bool dironly)
{
	if(rec && dirs) { 
		for(int i = 0; i < dsize; ++i) 
            if(dirs[i].d) {
                int cnt = dirs[i].cnt;
                for(pooldir *di = dirs[i].d; cnt--; di = di->nxt) {
                    di->nxt = NULL;
                    Free(di);
                }
                dirs[i].d = NULL;
            }
	}
	if(!dironly && vals) { 
		for(int i = 0; i < vsize; ++i) 
            if(vals[i].v) { delete vals[i].v; vals[i].v = NULL; }
	}
}

V pooldir::Reset(bool realloc)
{
	Clear(true,false);

	if(dirs) delete[] dirs; 
	if(vals) delete[] vals;

	if(realloc) {
		dirs = new direntry[dsize];
		ZeroMem(dirs,dsize*sizeof *dirs);
		vals = new valentry[vsize];
		ZeroMem(vals,vsize*sizeof *vals);
	}
	else 
		dirs = NULL,vals = NULL;
}

pooldir *pooldir::AddDir(int argc,const t_atom *argv,int vcnt,int dcnt)
{
	if(!argc) return this;

	int c = 1,dix = DIdx(argv[0]);
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

pooldir *pooldir::GetDir(int argc,const t_atom *argv,bool rmv)
{
	if(!argc) return this;

	int c = 1,dix = DIdx(argv[0]);
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

bool pooldir::DelDir(int argc,const t_atom *argv)
{
	pooldir *pd = GetDir(argc,argv,true);
	if(pd && pd != this) {
		Free(pd);
		return true;
	}
	else 
		return false;
}

V pooldir::SetVal(const t_atom &key,AtomList *data,bool over)
{
    int c = 1,vix = VIdx(key);
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

bool pooldir::SetVal(int rix,AtomList *data)
{
    poolval *prv = NULL,*ix = NULL;
	for(int vix = 0; vix < vsize; ++vix) 
		if(rix > vals[vix].cnt) rix -= vals[vix].cnt;
		else {
			ix = vals[vix].v;
			for(; ix && rix; prv = ix,ix = ix->nxt) --rix;
			if(ix && !rix) break;
		}  

	if(ix) { 
		// data exists... overwrite it
		
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
        return true;
	}
    else
        return false;
}

poolval *pooldir::RefVal(const t_atom &key)
{
	int c = 1,vix = VIdx(key);
	poolval *ix = vals[vix].v;
	for(; ix; ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c <= 0) break;
	}

	return c || !ix?NULL:ix;
}

poolval *pooldir::RefVal(int rix)
{
	for(int vix = 0; vix < vsize; ++vix) 
		if(rix > vals[vix].cnt) rix -= vals[vix].cnt;
		else {
			poolval *ix = vals[vix].v;
			for(; ix && rix; ix = ix->nxt) --rix;
			if(ix && !rix) return ix;
		}
	return NULL;
}

flext::AtomList *pooldir::GetVal(const t_atom &key,bool cut)
{
	int c = 1,vix = VIdx(key);
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

int pooldir::CntAll() const
{
	int cnt = 0;
	for(int vix = 0; vix < vsize; ++vix) cnt += vals[vix].cnt;
	return cnt;
}

int pooldir::PrintAll(char *buf,int len) const
{
    int offs = strlen(buf);

    int cnt = 0;
    for(int vix = 0; vix < vsize; ++vix) {
		poolval *ix = vals[vix].v;
        for(int i = 0; ix; ++i,ix = ix->nxt) {
			PrintAtom(ix->key,buf+offs,len-offs);
            strcat(buf+offs," , ");
            int l = strlen(buf+offs)+offs;
			ix->data->Print(buf+l,len-l);
            post(buf);
        }
        cnt += vals[vix].cnt;
    }
    
    buf[offs] = 0;

	return cnt;
}

int pooldir::GetKeys(AtomList &keys)
{
	int cnt = CntAll();
	keys(cnt);

	for(int vix = 0; vix < vsize; ++vix) {
		poolval *ix = vals[vix].v;
		for(int i = 0; ix; ++i,ix = ix->nxt) 
			SetAtom(keys[i],ix->key);
	}
	return cnt;
}

int pooldir::GetAll(t_atom *&keys,AtomList *&lst,bool cut)
{
	int cnt = CntAll();
	keys = new t_atom[cnt];
	lst = new AtomList[cnt];

	for(int i = 0,vix = 0; vix < vsize; ++vix) {
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


int pooldir::CntSub() const
{
	int cnt = 0;
	for(int dix = 0; dix < dsize; ++dix) cnt += dirs[dix].cnt;
	return cnt;
}


int pooldir::GetSub(const t_atom **&lst)
{
	const int cnt = CntSub();
	lst = new const t_atom *[cnt];
	for(int i = 0,dix = 0; i < cnt; ++dix) {
		pooldir *ix = dirs[dix].d;
		for(; ix; ix = ix->nxt) lst[i++] = &ix->dir;
	}
	return cnt;
}


bool pooldir::Paste(const pooldir *p,int depth,bool repl,bool mkdir)
{
	bool ok = true;

	for(int vi = 0; vi < p->vsize; ++vi) {
		for(poolval *ix = p->vals[vi].v; ix; ix = ix->nxt) {
			SetVal(ix->key,new AtomList(*ix->data),repl);
		}
	}

	if(ok && depth) {
		for(int di = 0; di < p->dsize; ++di) {
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

bool pooldir::Copy(pooldir *p,int depth,bool cut)
{
	bool ok = true;

	if(cut) {
		for(int vi = 0; vi < vsize; ++vi) {
			for(poolval *ix = vals[vi].v; ix; ix = ix->nxt)
				p->SetVal(ix->key,ix->data);
			vals[vi].cnt = 0;
			vals[vi].v = NULL;
		}
	}
	else {
		for(int vi = 0; vi < vsize; ++vi) {
			for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
				p->SetVal(ix->key,new AtomList(*ix->data));
			}
		}
	}

	if(ok && depth) {
		for(int di = 0; di < dsize; ++di) {
			for(pooldir *dix = dirs[di].d; ok && dix; dix = dix->nxt) {
				pooldir *ndir = p->AddDir(1,&dix->dir);
				if(ndir)
					ok = dix->Copy(ndir,depth > 0?depth-1:depth,cut);
				else
					ok = false;
			}
		}
	}

	return ok;
}


unsigned int pooldir::FoldBits(unsigned long h,int bits)
{
	if(!bits) return 0;
	const unsigned int hmax = (1<<bits)-1;
	unsigned int ret = 0;
	for(unsigned int i = 0; i < sizeof(h)*8; i += bits)
		ret ^= (h>>i)&hmax;
	return ret;
}

int pooldir::Int2Bits(unsigned long n)
{
	int b;
	for(b = 0; n; ++b) n >>= 1;
	return b;
}


// --- iterator ---------------------------------------

bool pooldir::ItValid(const pooliter &it) const
{
    if(it.sl < 0 || it.ix < 0) return false;
    if(it.dir)
        return it.sl < DSize() && it.ix < dirs[it.sl].cnt;
    else
        return it.sl < VSize() && it.ix < vals[it.sl].cnt;
}

bool pooldir::ItInc(pooliter &it) const
{
    if(!ItValid(it)) return false;

    if(it.dir) {
        while(++it.ix >= dirs[it.sl].cnt) {
            it.ix = 0;
            if(++it.sl >= DSize()) return false;
        }
    }
    else {
        while(++it.ix >= vals[it.sl].cnt) {
            it.ix = 0;
            if(++it.sl >= VSize()) return false;
        }
    }
    return true;
}

bool pooldir::ItDec(pooliter &it) const
{
    if(!ItValid(it)) return false;

    if(it.dir) {
        while(--it.ix < 0) {
            it.ix = dirs[it.sl].cnt-1;
            if(--it.sl < 0) return false;
        }
    }
    else {
        while(--it.ix < 0) {
            it.ix = vals[it.sl].cnt-1;
            if(--it.sl < 0) return false;
        }
    }
    return true;
}

