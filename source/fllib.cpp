/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001,2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

// Code for handling of object creation functions

#include "flbase.h"
#include "flsupport.h"

#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#define ALIASDEL ','

#define ALIASSLASHES ":/\\"
#ifdef MAXMSP
	#define ALIASSLASH ':'
#elif defined(NT)
	#define ALIASSLASH '/'
#else
	#define ALIASSLASH '/'
#endif

//! Extract space-delimited words from a string
static const char *extract(const char *name,int ix = 0)
{
	static char tmp[1024];
	const char *n = name;
	
	const char *del = strchr(name,ALIASDEL);

	if(del) {
		if(ix < 0) {
			char *t = tmp;
			while(n < del && (isspace(*n) || strchr(ALIASSLASHES,*n))) ++n;
			while(n < del && !isspace(*n)) {
				char c = *(n++);
				*(t++) = strchr(ALIASSLASHES,c)?ALIASSLASH:c;
			}
			while(*t == ALIASSLASH && t > tmp) --t;
			*t = 0;

			return tmp;
		}

		n = del+1;
	}

	while(*n && isspace(*n)) ++n;
	
	for(int i = 0; n && *n; ++i) {
		if(i == ix) {
			char *t = tmp;

			for(; *n && !isspace(*n); ++t,++n) *t = *n;
			*t = 0;
			return *tmp?tmp:NULL;
		}
		else {
			while(*n && !isspace(*n)) ++n;
			while(*n && isspace(*n)) ++n;		
		}
	}

	return NULL;
}


//! Check if object's name ends with a tilde
static bool chktilde(const char *objname)
{
//	int stplen = strlen(setupfun);
	bool tilde = true; //!strncmp(setupfun,"_tilde",6);

	if((objname[strlen(objname)-1] == '~'?1:0)^(tilde?1:0)) {
		if(tilde) 
			error("flext: %s (no trailing ~) is defined as a tilde object",objname);
		else
			error("flext::check_tilde: %s is no tilde object",objname);
		return true;
	} 
	else
		return false;
}



// this class stands for one registered object
// it holds the class, type flags, constructor and destructor of the object and the creation arg types
// it will never be destroyed
class libobject {
public:
	libobject(t_class *&cl,flext_obj *(*newf)(int,t_atom *),void (*freef)(flext_hdr *)); 
	
	flext_obj *(*newfun)(int,t_atom *);
	void (*freefun)(flext_hdr *c);

	t_class *const &clss;
	bool lib,dsp;
	int argc;
	int *argv;
};

libobject::libobject(t_class *&cl,flext_obj *(*newf)(int,t_atom *),void (*freef)(flext_hdr *)): 
	clss(cl),newfun(newf),freefun(freef),argc(0),argv(NULL) 
{}
	
// this class stands for one registered object name
// it holds a pointer to the respective object
// it will never be destroyed
class libname {
public:
	libname(const t_symbol *n,libobject *o): name(n),obj(o),nxt(NULL) {}	

	const t_symbol *name;
	libobject *obj;

	static void add(libname *n);
	static libname *find(const t_symbol *s);
	
protected:
	libname *nxt;
	void addrec(libname *n);
	static libname *root;
};

void libname::addrec(libname *n) { if(nxt) nxt->addrec(n); else nxt = n; }

libname *libname::root = NULL;

void libname::add(libname *l) {
	if(root) root->addrec(l);
	else root = l;
}

libname *libname::find(const t_symbol *s) {
	libname *l;
	for(l = root; l; l = l->nxt)
		if(s == l->name) break;
	return l;
}

// for MAXMSP, the library is represented by a special object (class) registered at startup
// all objects in the library are clones of that library object - they share the same class
#ifdef MAXMSP
static t_class *lib_class = NULL;
static const t_symbol *lib_name = NULL;
#endif

void flext_obj::lib_init(const char *name,void setupfun())
{
#ifdef MAXMSP
	lib_name = MakeSymbol(name);
	::setup(
		(t_messlist **)&lib_class,
		(t_newmethod)obj_new,(t_method)obj_free,
		sizeof(flext_hdr),NULL,A_GIMME,A_NULL);
#endif
	setupfun();
}

void flext_obj::obj_add(bool lib,bool dsp,const char *idname,const char *names,void setupfun(t_class *),flext_obj *(*newfun)(int,t_atom *),void (*freefun)(flext_hdr *),int argtp1,...)
{
#ifdef _DEBUG
	if(dsp) chktilde(idname);
#endif

	t_class **cl = 
#ifdef MAXMSP
		lib?&lib_class:
#endif
		new t_class *;
	const t_symbol *nsym = MakeSymbol(extract(names));
	
	// register object class
#ifdef PD
    *cl = ::class_new(
		(t_symbol *)nsym,
    	(t_newmethod)obj_new,(t_method)obj_free,
     	sizeof(flext_hdr),0,A_GIMME,A_NULL);
#elif defined(MAXMSP)
	if(!lib) {
		::setup(
			(t_messlist **)cl,
    		(t_newmethod)obj_new,(t_method)obj_free,
     		sizeof(flext_hdr),NULL,A_GIMME,A_NULL);
	}
#endif

	// make new dynamic object
	libobject *lo = new libobject(*cl,newfun,freefun);
	lo->lib = lib;
	lo->dsp = dsp;

	// parse the argument type list and store it with the object
	if(argtp1 == A_GIMME)
		lo->argc = -1;
	else {
		int argtp,i;
		va_list marker;
		
		// parse a first time and count only
		va_start(marker,argtp1);
		for(argtp = argtp1; argtp != A_NULL; ++lo->argc) argtp = (int)va_arg(marker,int); 
		va_end(marker);

		lo->argv = new int[lo->argc];
	
		// now parse and store
		va_start(marker,argtp1);
		for(argtp = argtp1,i = 0; i < lo->argc; ++i) {
			lo->argv[i] = argtp;
			argtp = (int)va_arg(marker,int); 
		}
		va_end(marker);
	}

	// make help reference
	flext_obj::DefineHelp(lo->clss,idname,extract(names,-1),dsp);

	for(int ix = 0; ; ++ix) {
		// in this loop register all the possible aliases of the object
	
		const char *c = ix?extract(names,ix):GetString(nsym);
		if(!c || !*c) break;

		// add to name list
		libname *l = new libname(MakeSymbol(c),lo);
		libname::add(l);
	
#ifdef PD
		if(ix > 0) 
			// in PD the first name is already registered with class creation
			::class_addcreator((t_newmethod)obj_new,(t_symbol *)l->name,A_GIMME,A_NULL);
#elif defined(MAXMSP)
		if(ix > 0 || lib) 
			// in MaxMSP the first alias gets its name from the name of the object file,
			// unless it is a library (then the name can be different)
			::alias(const_cast<char *>(c));  
#endif	
	}

	// call class setup function
    setupfun(lo->clss);
}
	

typedef flext_obj *(*libfun)(int,t_atom *);

flext_hdr *flext_obj::obj_new(const t_symbol *s,int argc,t_atom *argv)
{
	flext_hdr *obj = NULL;
	libname *l = libname::find(s);
	if(l) {
		bool ok = true;
		t_atom args[FLEXT_MAXNEWARGS]; 
		libobject *lo = l->obj;

		if(lo->argc >= 0) {
#ifdef _DEBUG
			if(lo->argc > FLEXT_MAXNEWARGS) { ERRINTERNAL(); ok = false; }
#endif

			if(argc == lo->argc) {
				for(int i = 0; /*ok &&*/ i < lo->argc; ++i) {
					switch(lo->argv[i]) {
#ifdef MAXMSP
					case A_INT:
						if(flext::IsInt(argv[i])) args[i] = argv[i];
						else if(flext::IsFloat(argv[i])) flext::SetInt(args[i],(int)flext::GetFloat(argv[i]));
						else ok = false;
						break;
#endif
					case A_FLOAT:
						if(flext::IsInt(argv[i])) flext::SetFloat(args[i],(float)flext::GetInt(argv[i]));
						else if(flext::IsFloat(argv[i])) args[i] = argv[i];
						else ok = false;
						break;
					case A_SYMBOL:
						if(flext::IsSymbol(argv[i])) args[i] = argv[i];
						else ok = false;
						break;
					}
				}
			
				if(!ok)
					post("%s: Creation arguments do not match",s->s_name);
			}
			else {
				error("%s: %s creation arguments",s->s_name,argc < lo->argc?"Not enough":"Too many");
				ok = false;
			}
		}

		if(ok) {
#ifdef PD
			obj = (flext_hdr *)::pd_new(lo->clss);
#elif defined(MAXMSP)
			obj = (flext_hdr *)::newobject(lo->clss);
#endif
		    flext_obj::m_holder = obj;
			flext_obj::m_holdname = l->name;

			// get actual flext object (newfun calls "new flext_obj()")
			if(lo->argc >= 0)
				// for interpreted arguments
				obj->data = lo->newfun(lo->argc,args);
			else
				// for variable argument list
				obj->data = lo->newfun(argc,argv); 

			flext_obj::m_holder = NULL;
			flext_obj::m_holdname = NULL;

			if(!obj->data || 
				// check constructor exit flag
				!obj->data->InitOk() || 
				// call virtual init function 
				!obj->data->Init()) 
			{ 
				// there was some init error, free object
				lo->freefun(obj); 
				obj = NULL; 
			}
		}
	}
#ifdef _DEBUG
	else
#ifdef MAXMSP
		// in MaxMSP an object with the name of the library exists, even if not explicitely declared!
		if(s != lib_name) 
#endif
		error("Class %s not found in library!",s->s_name);
#endif

	return obj;
}

void flext_obj::obj_free(flext_hdr *hdr)
{
	const t_symbol *name = hdr->data->thisNameSym();
	libname *l = libname::find(name);

	if(l) {
		// call virtual exit function
		hdr->data->Exit();

		// now call object destructor and deallocate
		l->obj->freefun(hdr);
	}
#ifdef _DEBUG
	else 
#ifdef MAXMSP
		// in MaxMSP an object with the name of the library exists, even if not explicitely declared!
		if(name != lib_name) 
#endif
		error("Class %s not found in library!",name);
#endif
}


