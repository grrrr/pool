/* 

pool - hierarchical storage object for PD and Max/MSP

Copyright (c) 2002-2004 Thomas Grill
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "pool.h"
#include <iostream>

using namespace std;

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
            if(!s && (*c == '-' || *c == '+')) {} // minus or plus is ok
            else
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

static BL ParseAtoms(C *tmp,flext::AtomList &l)
{
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

static BL ParseAtoms(string &s,flext::AtomList &l) 
{ 
    return ParseAtoms((C *)s.c_str(),l); 
}

static BL ReadAtoms(istream &is,flext::AtomList &l,C del)
{
	C tmp[1024];
	is.getline(tmp,sizeof tmp,del); 
	if(is.eof() || !is.good()) 
        return false;
    else
        return ParseAtoms(tmp,l);
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
//        if(IsSymbol(l[i]) os << "\"";
		WriteAtom(os,l[i]);
//        if(IsSymbol(l[i]) os << "\"";
		if(i < l.Count()-1) os << ' ';
	}
}

BL pooldir::LdDir(istream &is,I depth,BL mkdir)
{
	for(I i = 1; !is.eof(); ++i) {
		AtomList d,k,*v = new AtomList;
		BL r = 
            ReadAtoms(is,d,',') && 
            ReadAtoms(is,k,',') &&
            ReadAtoms(is,*v,'\n');

		if(r) {
			if(depth < 0 || d.Count() <= depth) {
				pooldir *nd = mkdir?AddDir(d):GetDir(d);
				if(nd) {
                    if(k.Count() == 1) {
	    				nd->SetVal(k[0],v); v = NULL;
                    }
                    else if(k.Count() > 1)
                        post("pool - file format invalid: key must be a single word");
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
    I cnt = 0;
	for(I vi = 0; vi < vsize; ++vi) {
		for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
			WriteAtoms(os,dir);
			os << " , ";
			WriteAtom(os,ix->key);
			os << " , ";
			WriteAtoms(os,*ix->data);
			os << endl;
            ++cnt;
		}
	}
    if(!cnt) {
        // no key/value pairs present -> force empty directory
		WriteAtoms(os,dir);
		os << " , ," << endl;
    }
	if(depth) {
        // save sub-directories
		I nd = depth > 0?depth-1:-1;
		for(I di = 0; di < dsize; ++di) {
			for(pooldir *ix = dirs[di].d; ix; ix = ix->nxt) {
				ix->SvDir(os,nd,AtomList(dir).Append(ix->dir));
			}
		}
	}
	return true;
}

class xmltag {
public:
    string tag,attr;
    bool Ok() const { return tag.length() > 0; }
    bool operator ==(const C *t) const { return !tag.compare(t); }

    void Clear() 
    { 
#if defined(_MSC_VER) && (_MSC_VER < 0x1200)
        // incomplete STL implementation
        tag = ""; attr = ""; 
#else
        tag.clear(); attr.clear(); 
#endif
    }

    enum { t_start,t_end,t_empty } type;
};

static bool gettag(istream &is,xmltag &tag)
{
    static const char *commstt = "<!--",*commend = "-->";

    for(;;) {
        // eat whitespace
        while(isspace(is.peek())) is.get();

        // no tag begin -> break
        if(is.peek() != '<') break;
        is.get(); // swallow <

        char tmp[1024],*t = tmp;

        // parse for comment start
        const char *c = commstt;
        while(*++c) {
            if(*c != is.peek()) break;
            *(t++) = is.get();
        }

        if(!*c) { // is comment
            char cmp[2] = {0,0}; // set to some unusual initial value

            for(int ic = 0; ; ic = (++ic)%2) {
                char c = is.get();
                if(c == '>') {
                    // if third character is > then check also the former two
                    int i;
                    for(i = 0; i < 2 && cmp[(ic+i)%2] == commend[i]; ++i) {}
                    if(i == 2) break; // match: comment end found!
                }
                else
                    cmp[ic] = c;
            }
        }
        else {
            // parse until > with consideration of "s
            bool intx = false;
            for(;;) {
                *t = is.get();
                if(*t == '"') intx = !intx;
                else if(*t == '>' && !intx) {
                    *t = 0;
                    break;
                }
                t++;
            }

            // look for tag slashes

            char *tb = tmp,*te = t-1,*tf;

            for(; isspace(*tb); ++tb) {}
            if(*tb == '/') { 
                // slash at the beginning -> end tag
                tag.type = xmltag::t_end;
                for(++tb; isspace(*tb); ++tb) {}
            }
            else {
                for(; isspace(*te); --te) {}
                if(*te == '/') { 
                    // slash at the end -> empty tag
                    for(--te; isspace(*te); --te) {}
                    tag.type = xmltag::t_empty;
                }
                else 
                    // no slash -> begin tag
                    tag.type = xmltag::t_start;
            }

            // copy tag text without slashes
            for(tf = tb; tf <= te && *tf && !isspace(*tf); ++tf) {}
            tag.tag.assign(tb,tf-tb);
            while(isspace(*tf)) ++tf;
            tag.attr.assign(tf,te-tf+1);

            return true;
        }
    }

    tag.Clear();
    return false;
}

static void getvalue(istream &is,string &s)
{
    char tmp[1024],*t = tmp; 
    bool intx = false;
    for(;;) {
        char c = is.peek();
        if(c == '"') intx = !intx;
        else if(c == '<' && !intx) break;
        *(t++) = is.get();
    }
    *t = 0;
    s = tmp;
}

BL pooldir::LdDirXMLRec(istream &is,I depth,BL mkdir,AtomList &d)
{
    AtomList k,v;
    bool inval = false,inkey = false,indata = false;
    int cntval = 0;

	while(!is.eof()) {
        xmltag tag;
        gettag(is,tag);
        if(!tag.Ok()) {
            // look for value
            string s;
            getvalue(is,s);

            if(s.length() &&
                (
                    (!inval && inkey && d.Count()) ||  /* dir */
                    (inval && (inkey || indata)) /* value */
                )
            ) {
                BL ret = true;
                if(indata) {
                    if(v.Count())
                        post("pool - XML load: value data already given, ignoring new data");
                    else
                        ret = ParseAtoms(s,v);
                }
                else // inkey
                    if(inval) {
                        if(k.Count())
                            post("pool - XML load, value key already given, ignoring new key");
                        else
                            ret = ParseAtoms(s,k);
                    }
                    else {
                        t_atom &dkey = d[d.Count()-1];
                        const char *ds = GetString(dkey);
                        FLEXT_ASSERT(ds);
                        if(*ds) 
                            post("pool - XML load: dir key already given, ignoring new key");
                        else
                            SetString(dkey,s.c_str());
                        ret = true;
                    }
                if(!ret) post("pool - error interpreting XML value (%s)",s.c_str());
            }
            else
                post("pool - error reading XML data");
        }
        else if(tag == "dir") {
            if(tag.type == xmltag::t_start) {
                // warn if last directory key was not given
                if(d.Count() && GetSymbol(d[d.Count()-1]) == sym__)
                    post("pool - XML load: dir key must be given prior to subdirs, ignoring items");

                AtomList dnext(d.Count()+1);
                // copy existing dir
                dnext.Set(d.Count(),d.Atoms(),0,false);
                // initialize current dir key as empty
                SetSymbol(dnext[d.Count()],sym__);

                // read next level
                LdDirXMLRec(is,depth,mkdir,dnext); 
            }
            else if(tag.type == xmltag::t_end) {
                if(!cntval && mkdir) {
                    // no values have been found in dir -> make empty dir
                    AddDir(d);
                }

                // break tag loop
                break;
            }
        }
        else if(tag == "value") {
            if(tag.type == xmltag::t_start) {
                inval = true;
                ++cntval;
                k.Clear(); v.Clear();
            }
            else if(tag.type == xmltag::t_end) {
                // set value after tag closing, but only if level <= depth
        	    if(depth < 0 || d.Count() <= depth) {
                    int fnd;
                    for(fnd = d.Count()-1; fnd >= 0; --fnd)
                        if(GetSymbol(d[fnd]) == sym__) break;

                    // look if last dir key has been given
                    if(fnd >= 0) {
                        if(fnd == d.Count()-1)
                            post("pool - XML load: dir key must be given prior to values");

                        // else: one directoy level has been left unintialized, ignore items
                    }
                    else {
                        // only use first word of key
                        if(k.Count() == 1) {
		        		    pooldir *nd = mkdir?AddDir(d):GetDir(d);
        				    if(nd) 
                                nd->SetVal(k[0],new AtomList(v));
                            else
                                post("pool - XML load: value key must be exactly one word, value not stored");
				        }
                    }
                }
                inval = false;
            }
        }
        else if(tag == "key") {
            if(tag.type == xmltag::t_start) {
                inkey = true;
            }
            else if(tag.type == xmltag::t_end) {
                inkey = false;
            }
        }
        else if(tag == "data") {
            if(!inval) 
                post("pool - XML tag <data> not within <value>");

            if(tag.type == xmltag::t_start) {
                indata = true;
            }
            else if(tag.type == xmltag::t_end) {
                indata = false;
            }
        }
        else if(!d.Count() && tag == "pool" && tag.type == xmltag::t_end) {
            // break tag loop
            break;
        }
#ifdef FLEXT_DEBUG
        else {
            post("pool - unknown XML tag '%s'",tag.tag.c_str());
        }
#endif
    }
    return true;
}

BL pooldir::LdDirXML(istream &is,I depth,BL mkdir)
{
	while(!is.eof()) {
        xmltag tag;
        if(!gettag(is,tag)) break;

        if(tag == "pool") {
            if(tag.type == xmltag::t_start) {
                AtomList empty; // must be a separate definition for gcc
                LdDirXMLRec(is,depth,mkdir,empty);
            }
            else
                post("pool - pool not initialized yet");
        }
        else if(tag == "!DOCTYPE") {
            // ignore
        }
#ifdef FLEXT_DEBUG
        else {
            post("pool - unknown XML tag '%s'",tag.tag.c_str());
        }
#endif
    }
    return true;
}

static void indent(ostream &s,I cnt) 
{
    for(I i = 0; i < cnt; ++i) s << '\t';
}

BL pooldir::SvDirXML(ostream &os,I depth,const AtomList &dir,I ind)
{
	int i,lvls = ind?(dir.Count()?1:0):dir.Count();

	for(i = 0; i < lvls; ++i) {
		indent(os,ind+i);
		os << "<dir>" << endl;
		indent(os,ind+i+1);
		os << "<key>";
		WriteAtom(os,dir[i]);
		os << "</key>" << endl;
	}

	for(I vi = 0; vi < vsize; ++vi) {
		for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
            indent(os,ind+lvls);
            os << "<value><key>";
			WriteAtom(os,ix->key);
            os << "</key><data>";
			WriteAtoms(os,*ix->data);
			os << "</data></value>" << endl;
		}
	}

	if(depth) {
		I nd = depth > 0?depth-1:-1;
		for(I di = 0; di < dsize; ++di) {
			for(pooldir *ix = dirs[di].d; ix; ix = ix->nxt) {
				ix->SvDirXML(os,nd,AtomList(dir).Append(ix->dir),ind+lvls);
			}
		}
	}

	for(i = lvls-1; i >= 0; --i) {
		indent(os,ind+i);
		os << "</dir>" << endl;
	}
	return true;
}
