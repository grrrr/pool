/* 

pool - hierarchic storage object for PD and Max/MSP

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "pool.h"

class pool:
	public flext_base
{
	FLEXT_HEADER_S(pool,flext_base,setup)

public:
	pool(I argc,const A *argv);
	~pool();

	static V setup(t_class *);

	pooldata *Pool() { return pl; }

protected:

	// switch to other pool
	void m_pool(I argc,const A *argv);

	// clear all data in pool
	void m_reset();

	void m_mkdir(I argc,const A *argv);		// make and change to dir
	void m_chdir(I argc,const A *argv);		// change to dir
	void m_rmdir();							// remove and reset current dir
	void m_svdir(I argc,const A *argv);		// save current dir

	void m_set(I argc,const A *argv);
	void m_clr(const S *sym);
	void m_clrall();
	void m_get(const S *sym);
	void m_getall();

	// load/save from/to file
	void m_load(I argc,const A *argv);
	void m_save(I argc,const A *argv);

	// set send/receive symbols
	void m_recv(I argc,const A *argv);
	void m_send(I argc,const A *argv);

private:
	BL priv;
	pooldata *pl;
	AtomList curdir;

	static pooldata *head,*tail;

	V SetPool(const S *s);
	V FreePool();

	static pooldata *GetPool(const S *s);
	static V RmvPool(pooldata *p);

	FLEXT_CALLBACK_V(m_pool)
	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK_V(m_mkdir)
	FLEXT_CALLBACK_V(m_chdir)
	FLEXT_CALLBACK(m_rmdir)
	FLEXT_CALLBACK_V(m_svdir)
	FLEXT_CALLBACK_V(m_set)
	FLEXT_CALLBACK_S(m_clr)
	FLEXT_CALLBACK(m_clrall)
	FLEXT_CALLBACK_S(m_get)
	FLEXT_CALLBACK(m_getall)
	FLEXT_CALLBACK_V(m_load)
	FLEXT_CALLBACK_V(m_save)
	FLEXT_CALLBACK_V(m_recv)
	FLEXT_CALLBACK_V(m_send)
};

FLEXT_NEW_V("pool",pool);


pooldata *pool::head,*pool::tail;


V pool::setup(t_class *)
{
	post("");
	post("pool %s - hierarchic storage object, (C)2002 Thomas Grill",POOL_VERSION);
	post("");

	head = tail = NULL;
}

pool::pool(I argc,const A *argv)
{
	SetPool(argc >= 1 && IsSymbol(argv[0])?GetSymbol(argv[0]):NULL);

	AddInAnything();
	AddOutAnything();
	AddOutBang();
	SetupInOut();

	FLEXT_ADDMETHOD_(0,"set",m_set);
	FLEXT_ADDMETHOD_(0,"reset",m_reset);
	FLEXT_ADDMETHOD_(0,"mkdir",m_mkdir);
	FLEXT_ADDMETHOD_(0,"chdir",m_chdir);
	FLEXT_ADDMETHOD_(0,"rmdir",m_rmdir);
	FLEXT_ADDMETHOD_(0,"svdir",m_svdir);
	FLEXT_ADDMETHOD_(0,"set",m_set);
	FLEXT_ADDMETHOD_(0,"clr",m_clr);
	FLEXT_ADDMETHOD_(0,"clrall",m_clrall);
	FLEXT_ADDMETHOD_(0,"get",m_get);
	FLEXT_ADDMETHOD_(0,"getall",m_getall);
	FLEXT_ADDMETHOD_(0,"load",m_load);
	FLEXT_ADDMETHOD_(0,"save",m_save);
	FLEXT_ADDMETHOD_(0,"recv",m_recv);
	FLEXT_ADDMETHOD_(0,"send",m_send);
}

pool::~pool()
{
	FreePool();
}

V pool::SetPool(const S *s)
{
	if(pl) FreePool();

	if(s) {
		priv = false;
		pl = GetPool(s);
	}
	else {
		priv = true;
		pl = new pooldata;
	}
}

V pool::FreePool()
{
	curdir(); // reset current directory

	if(pl) {
		if(!priv) 
			RmvPool(pl);
		else
			delete pl;
		pl = NULL;
	}
}

void pool::m_pool(I argc,const A *argv) 
{
	const S *s = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - pool: superfluous arguments omitted",thisName());
		s = GetASymbol(argv[0]);
		if(!s) post("%s - pool: invalid pool name, pool set to private",thisName());
	}

	SetPool(s);
}

void pool::m_reset() 
{
	pl->Reset();
}

void pool::m_mkdir(I argc,const A *argv)
{
	curdir(argc,argv);
	if(!pl->MkDir(curdir)) 
		post("%s - mkdir: directory couldn't be created",thisName());
	else 
		curdir();
}

void pool::m_chdir(I argc,const A *argv)
{
	curdir(argc,argv);
	if(!pl->ChkDir(curdir)) 
		post("%s - chdir: directory couldn't be changed",thisName());
	else
		curdir();
}

void pool::m_rmdir()
{
	if(!pl->RmDir(curdir)) 
		post("%s - rmdir: directory couldn't be removed",thisName());
	curdir();
}

void pool::m_svdir(I argc,const A *argv)
{
	const C *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - svdir: superfluous arguments omitted",thisName());
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

	if(!flnm)
		post("%s - svdir: invalid filename",thisName());
	else if(!pl->SvDir(curdir,flnm)) 
		post("%s - svdir: directory couldn't be saved",thisName());
}

void pool::m_set(I argc,const A *argv)
{
	const S *key = NULL;
	if(argc && IsSymbol(argv[0])) key = GetSymbol(argv[0]);

	if(!key)
		post("%s - set: invalid key given",thisName());
	else if(!pl->Set(curdir,key,new AtomList(argc-1,argv+1)))
		post("%s - set: value couldn't be set",thisName());
}

void pool::m_clr(const S *key)
{
	if(!pl->Clr(curdir,key))
		post("%s - clr: value couldn't be cleared",thisName());
}

void pool::m_clrall()
{
	if(!pl->ClrAll(curdir))
		post("%s - clrall: values couldn't be cleared",thisName());
}

void pool::m_get(const S *key)
{
	AtomList *r = pl->Get(curdir,key);
	if(!r) 
		post("%s - get: no corresponding value found",thisName());
	else {
		ToOutList(0,*r);
		delete r;
	}
}

void pool::m_getall()
{
	AtomList *r;
	I cnt = pl->GetAll(curdir,r);
	if(!r) 
		post("%s - getall: no corresponding values found",thisName());
	else {
		for(I i = 0; i < cnt; ++i) ToOutList(0,r[i]);
		delete[] r;
		ToOutBang(1);
	}
}

void pool::m_load(I argc,const A *argv)
{
	const C *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - load: superfluous arguments omitted",thisName());
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

	if(!flnm) 
		post("%s - load: no filename given",thisName());
	else if(!pl->Load(flnm))
		post("%s - load: error loading data",thisName());
}

void pool::m_save(I argc,const A *argv)
{
	const C *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - save: superfluous arguments omitted",thisName());
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

	if(!flnm) 
		post("%s - save: no filename given",thisName());
	else if(!pl->Save(flnm))
		post("%s - save: error saving data",thisName());
}

void pool::m_recv(I argc,const A *argv)
{
	post("%s - recv: sorry, not implemented",thisName());
}

void pool::m_send(I argc,const A *argv)
{
	post("%s - send: sorry, not implemented",thisName());
}




pooldata *pool::GetPool(const S *s)
{
	for(pooldata *pi = head; pi && pi->sym != s; pi = pi->nxt);

	if(pi) {
		pi->Push();
		return pi;
	}
	else {
		pooldata *p = new pooldata(s);
		p->Push();

		// now add to chain
		if(head) head->nxt = p;
		else head = p;
		tail = p;
		return p;
	}
}

V pool::RmvPool(pooldata *p)
{
	for(pooldata *prv = NULL,*pi = head; pi && pi != p; prv = pi,pi = pi->nxt);

	if(pi && !pi->Pop()) {
		if(prv) prv->nxt = pi->nxt;
		else head = pi->nxt;
		if(!pi->nxt) tail = pi;

		delete pi;
	}
}

