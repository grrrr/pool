/* 

pool - hierarchical storage object for PD and Max/MSP

Copyright (c) 2002-2004 Thomas Grill
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __POOL_H
#define __POOL_H

#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 406)
#error You need at least flext version 0.4.6
#endif

#include <iostream>

using namespace std;

typedef void V;
typedef int I;
typedef unsigned long UL;
typedef float F;
typedef char C;
typedef bool BL;
typedef t_atom A;
typedef t_symbol S;

class pooldir;

class dirref:
    public flext
{
public:
    dirref(): ptr(NULL) {}
    dirref(const dirref &r): ptr(NULL) { operator =(r); }
    dirref(int argc,const t_atom *argv): ptr(NULL) { Set(argc,argv); }
    dirref(const AtomList &l): ptr(NULL) { Set(l); }
    ~dirref() { Reset(); }

    int Count() const { return dir.Count(); }

    dirref &operator =(const dirref &r);
    dirref &operator ()(int argc,const t_atom *argv) { return Set(argc,argv); }
    dirref &operator ()(const AtomList &l) { return Set(l); }

    dirref &Set(int argc,const t_atom *argv);
    dirref &Reset() { return Set(0,NULL); }
    dirref &Set(const AtomList &d) { return Set(d.Count(),d.Atoms()); }
    
    dirref &Assoc(pooldir *p);

    dirref &Append(int argc,const t_atom *argv);
    dirref &Append(const AtomList &l) { return Append(l.Count(),l.Atoms()); }
    dirref &Part(int ix,int len);

    const AtomList &Dir() const { return dir; }
    pooldir *Ptr() const;

protected:
    pooldir *ptr; /* cached directory pointer */
    size_t ident;    /* unique ID of it - only valid if ptr != NULL */
    AtomList dir;   /* full specifier */
};

class pooliter 
{ 
public:
    pooliter(): dir(false),sl(0),ix(0) {}
    void Reset() { sl = ix = 0; }
    void Reset(bool d) { dir = d,sl = ix = 0; }

    dirref curdir;
    bool dir;
    int sl,ix; 
};

class poolval:
	public flext
{
public:
	poolval(const t_atom &key,AtomList *data);
	~poolval();

	poolval &Set(AtomList *data);
	poolval *Dup() const;

	t_atom key;
	AtomList *data;
	poolval *nxt;
};

class pooldir:
	public flext
{
    friend class dirref;

private:
    pooldir(const t_atom &dir,pooldir *parent,int vcnt = 0,int dcnt = 0);
	~pooldir();

    int usage;
    size_t ident;

public:
    /*! Reference an existing pooldir */
    static void Ref(pooldir *d) { ++d->usage; }
    /*! Dereference pooldir - don't use it anymore then! */
    static bool Unref(pooldir *d);
    /*! Allocate a new pooldir */
    static pooldir *New(const t_atom &dir,pooldir *parent,int vcnt = 0,int dcnt = 0);
    /*! Mark pooldir as invalid */
    static bool Free(pooldir *d) { d->ident = 0; return Unref(d); }

	void Clear(bool rec,bool dironly = false);
	void Reset(bool realloc = true);

	bool Empty() const { return !dirs && !vals; }
	bool HasDirs() const { return dirs != NULL; }
	bool HasVals() const { return vals != NULL; }

	pooldir *GetDir(int argc,const t_atom *argv,bool cut = false);
	pooldir *GetDir(const AtomList &d,bool cut = false) { return GetDir(d.Count(),d.Atoms(),cut); }
	bool DelDir(int argc,const t_atom *argv);
	bool DelDir(const AtomList &d) { return DelDir(d.Count(),d.Atoms()); }
	pooldir *AddDir(int argc,const t_atom *argv,int vcnt = 0,int dcnt = 0);
	pooldir *AddDir(const AtomList &d,int vcnt = 0,int dcnt = 0) { return AddDir(d.Count(),d.Atoms(),vcnt,dcnt); }

	void SetVal(const t_atom &key,AtomList *data,bool over = true);
	bool SetVal(int ix,AtomList *data);
	bool SetVal(const pooliter &it,AtomList *data);
	void ClrVal(const t_atom &key) { SetVal(key,NULL); }
    bool ClrVal(int ix) { return SetVal(ix,NULL); }
    bool ClrVal(const pooliter &it) { return SetVal(it,NULL); }
	poolval *RefVal(const t_atom &key);
	poolval *RefVal(int ix);
	poolval *RefVal(const pooliter &it);

	AtomList *PeekVal(const t_atom &key) { poolval *ix = RefVal(key); return ix?ix->data:NULL; }

	AtomList *GetVal(const t_atom &key,bool cut = false);
	int CntAll() const;
	int GetAll(t_atom *&keys,AtomList *&lst,bool cut = false);
	int PrintAll(char *buf,int len) const;
	int GetKeys(AtomList &keys);
	int CntSub() const;
	int GetSub(const t_atom **&dirs);

    /* clipboard operations */
	bool Paste(const pooldir *p,int depth,bool repl,bool mkdir);
	bool Copy(pooldir *p,int depth,bool cur);

    /* file IO operations */
	bool LdDir(istream &is,int depth,bool mkdir);
	bool LdDirXML(istream &is,int depth,bool mkdir);
	bool SvDir(ostream &os,int depth,const AtomList &dir = AtomList());
	bool SvDirXML(ostream &os,int depth,const AtomList &dir = AtomList(),int ind = 0);

    /* iterator operations */
    bool ItValid(const pooliter &it) const;
    bool ItInc(pooliter &it) const;
    bool ItDec(pooliter &it) const;

    /* storage parameters */
	int VSize() const { return vsize; }
	int DSize() const { return dsize; }

protected:
	int VIdx(const t_atom &v) const { return FoldBits(AtomHash(v),vbits); }
	int DIdx(const t_atom &d) const { return FoldBits(AtomHash(d),dbits); }

	t_atom dir;
	pooldir *nxt;

	pooldir *parent;
	const int vbits,dbits,vsize,dsize;
	
	static unsigned int FoldBits(unsigned long h,int bits);
	static int Int2Bits(unsigned long n);

	struct valentry { int cnt; poolval *v; };
	struct direntry { int cnt; pooldir *d; };
	
	valentry *vals;
	direntry *dirs;

private:
  	bool LdDirXMLRec(istream &is,int depth,bool mkdir,AtomList &d);
};


class pooldata:
	public flext
{
public:
	pooldata(const t_symbol *s = NULL,int vcnt = 0,int dcnt = 0);
	~pooldata();

	void Push() { ++refs; }
	bool Pop() { FLEXT_ASSERT(refs >= 0); return --refs > 0; }

    void Reset() { root->Reset(); }

    pooldir *GetDir(dirref &d);

    bool MkDir(dirref &d,int vcnt = 0,int dcnt = 0) 
    { 
        root->AddDir(d.Dir(),vcnt,dcnt); 
        return true; 
    }

    bool ChkDir(dirref &d) { return GetDir(d) != NULL; }

    bool RmDir(dirref &d) { return root->DelDir(d.Dir()); }

    bool Set(dirref &d,const t_atom &key,AtomList *data,bool over = true)
    {
	    pooldir *pd = GetDir(d);
	    if(!pd) return false;
	    pd->SetVal(key,data,over);
	    return true;
    }

    bool Set(dirref &d,int ix,AtomList *data)
    {
	    pooldir *pd = GetDir(d);
	    if(!pd) return false;
	    pd->SetVal(ix,data);
	    return true;
    }

	bool Clr(dirref &d,const t_atom &key)
    {
	    pooldir *pd = GetDir(d);
	    if(!pd) return false;
	    pd->ClrVal(key);
	    return true;
    }

	bool Clr(dirref &d,int ix)
    {
	    pooldir *pd = GetDir(d);
	    if(!pd) return false;
	    pd->ClrVal(ix);
	    return true;
    }

	bool ClrAll(dirref &d,bool rec,bool dironly = false)
    {
	    pooldir *pd = GetDir(d);
	    if(!pd) return false;
	    pd->Clear(rec,dironly);
	    return true;
    }

	AtomList *Peek(dirref &d,const t_atom &key)
    {
	    pooldir *pd = GetDir(d);
	    return pd?pd->PeekVal(key):NULL;
    }

	AtomList *Get(dirref &d,const t_atom &key)
    {
	    pooldir *pd = GetDir(d);
	    return pd?pd->GetVal(key):NULL;
    }

	poolval *Ref(dirref &d,const t_atom &key)
    {
	    pooldir *pd = GetDir(d);
	    return pd?pd->RefVal(key):NULL;
    }

	poolval *Ref(dirref &d,int ix)
    {
	    pooldir *pd = GetDir(d);
	    return pd?pd->RefVal(ix):NULL;
    }

	int CntAll(dirref &d)
    {
	    pooldir *pd = GetDir(d);
	    return pd?pd->CntAll():0;
    }

	int PrintAll(dirref &d);
	int GetAll(dirref &d,t_atom *&keys,AtomList *&lst);

    int CntSub(dirref &d)
    {
	    pooldir *pd = GetDir(d);
	    return pd?pd->CntSub():0;
    }

	int GetSub(dirref &d,const t_atom **&dirs);

    void ItReset(pooliter &it) const { it.Reset(); }

    bool ItInc(pooliter &it)
    {
	    pooldir *pd = GetDir(it.curdir);
	    return pd && pd->ItInc(it);
    }

    bool ItDec(pooliter &it)
    {
	    pooldir *pd = GetDir(it.curdir);
	    return pd && pd->ItDec(it);
    }

	bool Paste(dirref &d,const pooldir *clip,int depth = -1,bool repl = true,bool mkdir = true);
	pooldir *Copy(dirref &d,const t_atom &key,bool cut);
	pooldir *CopyAll(dirref &d,int depth,bool cut);

	bool LdDir(dirref &d,const char *flnm,int depth,bool mkdir = true);
	bool SvDir(dirref &d,const char *flnm,int depth,bool absdir);
	bool Load(const char *flnm) { return LdDir(dirref(),flnm,-1); }
	bool Save(const char *flnm) { return SvDir(dirref(),flnm,-1,true); }
	bool LdDirXML(dirref &d,const char *flnm,int depth,bool mkdir = true);
	bool SvDirXML(dirref &d,const char *flnm,int depth,bool absdir);
	bool LoadXML(const char *flnm) { return LdDirXML(dirref(),flnm,-1); }
	bool SaveXML(const char *flnm) { return SvDirXML(dirref(),flnm,-1,true); }

	int refs;
	const t_symbol *sym;
	pooldata *nxt;

	pooldir *root;

private:
	static const t_atom nullatom;
};

#endif
