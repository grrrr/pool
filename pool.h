/* 

pool - hierarchic storage object for PD and Max/MSP

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __POOL_H
#define __POOL_H

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 303)
#error You need at least flext version 0.3.3
#endif


#define POOL_VERSION "0.0.1"

#define V void
#define I int
#define F float
#define C char
#define BL bool
#define A t_atom
#define S t_symbol

typedef flext_base::AtomList AtomList;


class poolval
{
public:
	poolval(const A &key,AtomList *data);
	~poolval();

	poolval &Set(AtomList *data);

	A key;
	AtomList *data;
	poolval *nxt;
};

class pooldir
{
public:
	pooldir(const A &dir);
	~pooldir();

	V Clear(BL rec);

	pooldir *GetDir(I argc,const A *argv,BL rmv = NULL);
	pooldir *GetDir(const AtomList &d,BL rmv = NULL) { return GetDir(d.Count(),d.Atoms(),rmv); }
	BL DelDir(const AtomList &d);
	V AddDir(I argc,const A *argv);
	V AddDir(const AtomList &d) { AddDir(d.Count(),d.Atoms()); }
	V SetVal(const A &key,AtomList *data);
	V ClrVal(const A &key) { SetVal(key,NULL); }
	AtomList *GetVal(const A &key);
	I GetAll(A *&keys,AtomList *&lst);

	A dir;
	pooldir *nxt;

	pooldir *dirs;
	poolval *vals;
};

class pooldata
{
public:
	pooldata(const S *s = NULL);
	~pooldata();

	V Push() { ++refs; }
	BL Pop() { return --refs > 0; }

	V Reset();
	bool MkDir(const AtomList &d); 
	bool ChkDir(const AtomList &d);
	bool RmDir(const AtomList &d);
	bool SvDir(const AtomList &d,const C *flnm);

	bool Set(const AtomList &d,const A &key,AtomList *data);
	bool Clr(const AtomList &d,const A &key);
	bool ClrAll(const AtomList &d,BL rec);
	AtomList *Get(const AtomList &d,const A &key);
	I GetAll(const AtomList &d,A *&keys,AtomList *&lst);

	bool Load(const C *flnm);
	bool Save(const C *flnm);

	I refs;
	const S *sym;
	pooldata *nxt;

	pooldir root;

private:
	static t_atom nullatom;
};

#endif
