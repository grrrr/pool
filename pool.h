/* 

pool - hierarchic storage object for PD and Max/MSP

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __POOL_H
#define __POOL_H

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 304)
#error You need at least flext version 0.3.4
#endif

#define POOL_VERSION "0.0.2"

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

class istream;
class ostream;

class pooldir
{
public:
	pooldir(const A &dir);
	~pooldir();

	V Clear(BL rec,BL dironly = false);

	pooldir *GetDir(I argc,const A *argv,BL rmv = NULL);
	pooldir *GetDir(const AtomList &d,BL rmv = NULL) { return GetDir(d.Count(),d.Atoms(),rmv); }
	BL DelDir(const AtomList &d);
	V AddDir(I argc,const A *argv);
	V AddDir(const AtomList &d) { AddDir(d.Count(),d.Atoms()); }
	V SetVal(const A &key,AtomList *data);
	V ClrVal(const A &key) { SetVal(key,NULL); }
	AtomList *GetVal(const A &key);
	I GetAll(A *&keys,AtomList *&lst);
	I GetSub(const t_atom **&dirs);
	BL LdDir(istream &is,I depth);
	BL SvDir(ostream &os,I depth,const AtomList &dir = AtomList());

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
	BL MkDir(const AtomList &d); 
	BL ChkDir(const AtomList &d);
	BL RmDir(const AtomList &d);

	BL Set(const AtomList &d,const A &key,AtomList *data);
	BL Clr(const AtomList &d,const A &key);
	BL ClrAll(const AtomList &d,BL rec,BL dironly = false);
	AtomList *Get(const AtomList &d,const A &key);
	I GetAll(const AtomList &d,A *&keys,AtomList *&lst);
	I GetSub(const AtomList &d,const t_atom **&dirs);

	BL LdDir(const AtomList &d,const C *flnm,I depth);
	BL SvDir(const AtomList &d,const C *flnm,I depth,BL absdir);
	BL Load(const C *flnm) { return LdDir(AtomList(),flnm,-1); }
	BL Save(const C *flnm) { return SvDir(AtomList(),flnm,-1,true); }

	I refs;
	const S *sym;
	pooldata *nxt;

	pooldir root;

private:
	static t_atom nullatom;
};

#endif
