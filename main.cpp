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
	V m_pool(I argc,const A *argv);

	// clear all data in pool
	V m_reset();

	// output absolute directory paths?
	V m_absdir(BL abs) { absdir = abs; }
	// always output current directory
	V m_echodir(BL e) { echo = e; }

	// handle directories
	V m_getdir();

	V m_mkdir(I argc,const A *argv,BL abs = true);		// make and change to dir
	V m_chdir(I argc,const A *argv,BL abs = true);		// change to dir
	V m_rmdir(I argc,const A *argv,BL abs = true);		// remove dir
	V m_updir(I argc,const A *argv);		// one or more levels up

	V m_mksub(I argc,const A *argv) { m_mkdir(argc,argv,false); }
	V m_chsub(I argc,const A *argv) { m_chdir(argc,argv,false); }
	V m_rmsub(I argc,const A *argv) { m_rmdir(argc,argv,false); }

	// handle data
	V m_set(I argc,const A *argv);
	V m_clr(I argc,const A *argv);
	V m_clrall();	// only values
	V m_clrrec();	// also subdirectories
	V m_clrsub();	// only subdirectories
	V m_get(I argc,const A *argv);
	V m_getall();	// only values
	V m_getrec(I argc,const A *argv);	// also subdirectories
	V m_getsub(I argc,const A *argv);	// only subdirectories

	// load/save from/to file
	V m_load(I argc,const A *argv);
	V m_save(I argc,const A *argv);

	// save directories
	V m_svdir(I argc,const A *argv);   // save values in current dir
	V m_svrec(I argc,const A *argv);   // save values recursively

	// set send/receive symbols
	V m_recv(I argc,const A *argv);
	V m_send(I argc,const A *argv);

private:
	static BL KeyChk(const A &a);
	static BL ValChk(I argc,const A *argv);
	static BL ValChk(const AtomList &l) { return ValChk(l.Count(),l.Atoms()); }
	V ToOutAtom(I ix,const A &a);

	V getdir(const S *tag);
	V getrec(const S *tag,I level,const AtomList &rdir = AtomList());
	V getsub(const S *tag,I level,const AtomList &rdir = AtomList());

	V echodir() { if(echo) getdir(MakeSymbol("echo")); }

	BL priv,absdir,echo;
	pooldata *pl;
	AtomList curdir;

	static pooldata *head,*tail;

	V SetPool(const S *s);
	V FreePool();

	static pooldata *GetPool(const S *s);
	static V RmvPool(pooldata *p);

	FLEXT_CALLBACK_V(m_pool)
	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK_B(m_absdir)
	FLEXT_CALLBACK_B(m_echodir)
	FLEXT_CALLBACK(m_getdir)
	FLEXT_CALLBACK_V(m_mkdir)
	FLEXT_CALLBACK_V(m_chdir)
	FLEXT_CALLBACK_V(m_updir)
	FLEXT_CALLBACK_V(m_rmdir)
	FLEXT_CALLBACK_V(m_mksub)
	FLEXT_CALLBACK_V(m_chsub)
	FLEXT_CALLBACK_V(m_rmsub)
	FLEXT_CALLBACK_V(m_set)
	FLEXT_CALLBACK_V(m_clr)
	FLEXT_CALLBACK(m_clrall)
	FLEXT_CALLBACK(m_clrrec)
	FLEXT_CALLBACK(m_clrsub)
	FLEXT_CALLBACK_V(m_get)
	FLEXT_CALLBACK(m_getall)
	FLEXT_CALLBACK_V(m_getrec)
	FLEXT_CALLBACK_V(m_getsub)
	FLEXT_CALLBACK_V(m_load)
	FLEXT_CALLBACK_V(m_save)
	FLEXT_CALLBACK_V(m_svdir)
	FLEXT_CALLBACK_V(m_svrec)
	FLEXT_CALLBACK_V(m_recv)
	FLEXT_CALLBACK_V(m_send)
};

FLEXT_NEW_V("pool",pool);


pooldata *pool::head,*pool::tail;


V pool::setup(t_class *)
{
	post("");
	post("pool %s - hierarchical storage object, (C)2002 Thomas Grill",POOL_VERSION);
	post("");

	head = tail = NULL;
}

pool::pool(I argc,const A *argv):
	absdir(true),echo(false)
{
	SetPool(argc >= 1 && IsSymbol(argv[0])?GetSymbol(argv[0]):NULL);

	AddInAnything();
	AddOutList();
	AddOutAnything();
	AddOutList();
	AddOutAnything();
	SetupInOut();

	FLEXT_ADDMETHOD_(0,"set",m_set);
	FLEXT_ADDMETHOD_(0,"reset",m_reset);
	FLEXT_ADDMETHOD_(0,"absdir",m_absdir);
	FLEXT_ADDMETHOD_(0,"echodir",m_echodir);
	FLEXT_ADDMETHOD_(0,"getdir",m_getdir);
	FLEXT_ADDMETHOD_(0,"mkdir",m_mkdir);
	FLEXT_ADDMETHOD_(0,"chdir",m_chdir);
	FLEXT_ADDMETHOD_(0,"rmdir",m_rmdir);
	FLEXT_ADDMETHOD_(0,"updir",m_updir);
	FLEXT_ADDMETHOD_(0,"mksub",m_mksub);
	FLEXT_ADDMETHOD_(0,"chsub",m_chsub);
	FLEXT_ADDMETHOD_(0,"rmsub",m_rmsub);
	FLEXT_ADDMETHOD_(0,"set",m_set);
	FLEXT_ADDMETHOD_(0,"clr",m_clr);
	FLEXT_ADDMETHOD_(0,"clrall",m_clrall);
	FLEXT_ADDMETHOD_(0,"clrrec",m_clrrec);
	FLEXT_ADDMETHOD_(0,"clrsub",m_clrsub);
	FLEXT_ADDMETHOD_(0,"get",m_get);
	FLEXT_ADDMETHOD_(0,"getall",m_getall);
	FLEXT_ADDMETHOD_(0,"getrec",m_getrec);
	FLEXT_ADDMETHOD_(0,"getsub",m_getsub);
	FLEXT_ADDMETHOD_(0,"load",m_load);
	FLEXT_ADDMETHOD_(0,"save",m_save);
	FLEXT_ADDMETHOD_(0,"svdir",m_svdir);
	FLEXT_ADDMETHOD_(0,"svrec",m_svrec);
//	FLEXT_ADDMETHOD_(0,"recv",m_recv);
//	FLEXT_ADDMETHOD_(0,"send",m_send);
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

V pool::m_pool(I argc,const A *argv) 
{
	const S *s = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - pool: superfluous arguments ignored",thisName());
		s = GetASymbol(argv[0]);
		if(!s) post("%s - pool: invalid pool name, pool set to private",thisName());
	}

	SetPool(s);
}

V pool::m_reset() 
{
	pl->Reset();
}


V pool::getdir(const S *tag)
{
	ToOutAnything(3,tag,0,NULL);
	ToOutList(2,curdir);
}

V pool::m_getdir() { getdir(MakeSymbol("getdir")); }

V pool::m_mkdir(I argc,const A *argv,BL abs)
{
	if(!ValChk(argc,argv))
		post("%s - mkdir: invalid directory name",thisName());
	else {
		AtomList ndir;
		if(abs) ndir(argc,argv);
		else (ndir = curdir).Append(argc,argv);
		if(!pl->MkDir(ndir)) {
			post("%s - mkdir: directory couldn't be created",thisName());
		}
	}

	echodir();
}

V pool::m_chdir(I argc,const A *argv,BL abs)
{
	if(!ValChk(argc,argv)) 
		post("%s - chdir: invalid directory name",thisName());
	else {
		AtomList prv(curdir);
		if(abs) curdir(argc,argv);
		else curdir.Append(argc,argv);
		if(!pl->ChkDir(curdir)) {
			post("%s - chdir: directory couldn't be changed",thisName());
			curdir = prv;
		}
	}

	echodir();
}

V pool::m_updir(I argc,const A *argv)
{
	I lvls = 1;
	if(argc > 0) {
		if(CanbeInt(argv[0])) {
			if(argc > 1)
				post("%s - updir: superfluous arguments ignored",thisName());
			lvls = GetAInt(argv[0]);
			if(lvls < 0)
				post("%s - updir: invalid level specification - set to 1",thisName());
		}
		else
			post("%s - updir: invalid level specification - set to 1",thisName());
	}

	AtomList prv(curdir);

	if(lvls > curdir.Count()) {
		post("%s - updir: level exceeds directory depth - corrected",thisName());
		curdir();
	}
	else
		curdir.Part(0,curdir.Count()-lvls);

	if(!pl->ChkDir(curdir)) {
		post("%s - updir: directory couldn't be changed",thisName());
		curdir = prv;
	}

	echodir();
}

V pool::m_rmdir(I argc,const A *argv,BL abs)
{
	if(abs) curdir(argc,argv);
	else curdir.Append(argc,argv);

	if(!pl->RmDir(curdir)) 
		post("%s - rmdir: directory couldn't be removed",thisName());
	curdir();

	echodir();
}

V pool::m_set(I argc,const A *argv)
{
	if(!argc || !KeyChk(argv[0])) 
		post("%s - set: invalid key",thisName());
	else if(!ValChk(argc-1,argv+1)) {
		post("%s - set: invalid data values",thisName());
	}
	else 
		if(!pl->Set(curdir,argv[0],new AtomList(argc-1,argv+1)))
			post("%s - set: value couldn't be set",thisName());

	echodir();
}

V pool::m_clr(I argc,const A *argv)
{
	if(!argc || !KeyChk(argv[0]))
		post("%s - clr: invalid key",thisName());
	else {
		if(argc > 1) 
			post("%s - clr: superfluous arguments ignored",thisName());

		if(!pl->Clr(curdir,argv[0]))
			post("%s - clr: value couldn't be cleared",thisName());
	}

	echodir();
}

V pool::m_clrall()
{
	if(!pl->ClrAll(curdir,false))
		post("%s - clrall: values couldn't be cleared",thisName());

	echodir();
}

V pool::m_clrrec()
{
	if(!pl->ClrAll(curdir,true))
		post("%s - clrrec: values couldn't be cleared",thisName());

	echodir();
}

V pool::m_clrsub()
{
	if(!pl->ClrAll(curdir,true,true))
		post("%s - clrsub: directories couldn't be cleared",thisName());

	echodir();
}

V pool::m_get(I argc,const A *argv)
{
	if(!argc || !KeyChk(argv[0]))
		post("%s - get: invalid key",thisName());
	else {
		if(argc > 1) 
			post("%s - get: superfluous arguments ignored",thisName());

		AtomList *r = pl->Get(curdir,argv[0]);

		ToOutAnything(3,MakeSymbol("get"),0,NULL);
		if(absdir)
			ToOutList(2,curdir);
		else
			ToOutList(2,0,NULL);
		ToOutAtom(1,argv[0]);
		if(r) {
			ToOutList(0,*r);
			delete r;
		}
		else
			ToOutBang(0);
	}

	echodir();
}

V pool::getrec(const S *tag,I level,const AtomList &rdir)
{
	AtomList gldir(curdir);
	gldir.Append(rdir);

	{
		A *k;
		AtomList *r;
		I cnt = pl->GetAll(gldir,k,r);
		if(!k) 
			post("%s - %s: error retrieving values",thisName(),GetString(tag));
		else {
			for(I i = 0; i < cnt; ++i) {
				ToOutAnything(3,tag,0,NULL);
				ToOutList(2,absdir?gldir:rdir);
				ToOutAtom(1,k[i]);
				ToOutList(0,r[i]);
			}
			delete[] k;
			delete[] r;
		}
	}

	if(level != 0) {
		const A **r;
		I cnt = pl->GetSub(gldir,r);
		if(!r) 
			post("%s - %s: error retrieving directories",thisName(),GetString(tag));
		else {
			for(I i = 0; i < cnt; ++i) {
				getrec(tag,level > 0?level-1:-1,AtomList(rdir).Append(*r[i]));
			}
			delete[] r;
		}
	}
}

V pool::m_getall()
{
	getrec(MakeSymbol("getall"),0);
	ToOutBang(3);

	echodir();
}

V pool::m_getrec(I argc,const A *argv)
{
	I lvls = -1;
	if(argc > 0) {
		if(CanbeInt(argv[0])) {
			if(argc > 1)
				post("%s - getrec: superfluous arguments ignored",thisName());
			lvls = GetAInt(argv[0]);
		}
		else 
			post("%s - getrec: invalid level specification - set to infinite",thisName());
	}
	getrec(MakeSymbol("getrec"),lvls);
	ToOutBang(3);

	echodir();
}


V pool::getsub(const S *tag,I level,const AtomList &rdir)
{
	AtomList gldir(curdir);
	gldir.Append(rdir);

	const A **r;
	I cnt = pl->GetSub(gldir,r);
	if(!r) 
		post("%s - %s: error retrieving directories",thisName(),GetString(tag));
	else {
		for(I i = 0; i < cnt; ++i) {
			AtomList ndir(absdir?gldir:rdir);
			ndir.Append(*r[i]);

			ToOutAnything(3,tag,0,NULL);
			ToOutList(2,curdir);
			ToOutList(1,ndir);

			if(level != 0)
				getsub(tag,level > 0?level-1:-1,AtomList(rdir).Append(*r[i]));
		}
		delete[] r;
	}
}

V pool::m_getsub(I argc,const A *argv)
{
	I lvls = 0;
	if(argc > 0) {
		if(CanbeInt(argv[0])) {
			if(argc > 1)
				post("%s - getsub: superfluous arguments ignored",thisName());
			lvls = GetAInt(argv[0]);
		}
		else 
			post("%s - getsub: invalid level specification - set to 0",thisName());
	}

	getsub(MakeSymbol("getsub"),lvls);
	ToOutBang(3);

	echodir();
}

V pool::m_load(I argc,const A *argv)
{
	const C *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - load: superfluous arguments ignored",thisName());
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

	if(!flnm) 
		post("%s - load: no filename given",thisName());
	else if(!pl->Load(flnm))
		post("%s - load: error loading data",thisName());

	echodir();
}

V pool::m_save(I argc,const A *argv)
{
	const C *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - save: superfluous arguments ignored",thisName());
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

	if(!flnm) 
		post("%s - save: no filename given",thisName());
	else if(!pl->Save(flnm))
		post("%s - save: error saving data",thisName());

	echodir();
}

V pool::m_svdir(I argc,const A *argv)
{
	const C *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - svdir: superfluous arguments ignored",thisName());
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

	if(!flnm)
		post("%s - svdir: invalid filename",thisName());
	else {
		if(!pl->SvDir(curdir,flnm,0,absdir)) 
		post("%s - svdir: directory couldn't be saved",thisName());
	}

	echodir();
}

V pool::m_svrec(I argc,const A *argv)
{
	const C *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - svrec: superfluous arguments ignored",thisName());
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

	if(!flnm)
		post("%s - svrec: invalid filename",thisName());
	else {
		if(!pl->SvDir(curdir,flnm,-1,absdir)) 
		post("%s - svrec: directory couldn't be saved",thisName());
	}

	echodir();
}

V pool::m_recv(I argc,const A *argv)
{
	post("%s - recv: sorry, not implemented",thisName());
}

V pool::m_send(I argc,const A *argv)
{
	post("%s - send: sorry, not implemented",thisName());
}


BL pool::KeyChk(const t_atom &a)
{
	return IsSymbol(a) || IsFloat(a) || IsInt(a);
}

BL pool::ValChk(I argc,const t_atom *argv)
{
	for(I i = 0; i < argc; ++i) {
		const t_atom &a = argv[i];
		if(!IsSymbol(a) && !IsFloat(a) && !IsInt(a)) return false;
	}
	return true;
}

V pool::ToOutAtom(I ix,const t_atom &a)
{
	if(IsSymbol(a))
		ToOutSymbol(ix,GetSymbol(a));
	else if(IsFloat(a))
		ToOutFloat(ix,GetFloat(a));
	else if(IsInt(a))
		ToOutInt(ix,GetInt(a));
	else
		post("%s - output atom: type not supported!",thisName());
}



pooldata *pool::GetPool(const S *s)
{
	pooldata *pi = head;
	for(; pi && pi->sym != s; pi = pi->nxt);

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
	pooldata *prv = NULL,*pi = head;
	for(; pi && pi != p; prv = pi,pi = pi->nxt);

	if(pi && !pi->Pop()) {
		if(prv) prv->nxt = pi->nxt;
		else head = pi->nxt;
		if(!pi->nxt) tail = pi;

		delete pi;
	}
}

