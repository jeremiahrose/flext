/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001,2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file flbase.h
	\brief Internal flext base classes
    
	\remark This is all derived from GEM by Mark Danks
*/
 
#ifndef __FLEXT_BASE_H
#define __FLEXT_BASE_H

#include "flstdc.h"
#include "flsupport.h"

#ifdef FLEXT_THREADS
#include <pthread.h>
#endif

class flext_obj;

// ----------------------------------------------------------------------------
/*! \struct flext_hdr
	\brief The obligatory PD or Max/MSP object header
	\internal

    This is in a separate struct to assure that obj is the very first thing.  
    If it were the first thing in flext_obj, then there could be problems with
    the virtual table of the C++ class.
*/
// ----------------------------------------------------------------------------

struct FLEXT_EXT flext_hdr
{
    	/*! \brief The obligatory object header
			MUST reside at memory offset 0 (no virtual table possible)
		*/
    	t_sigobj    	    obj;  

#ifdef PD
		//! PD only: float signal holder for pd
		float defsig;			
#endif

#if defined(MAXMSP) 
		//! MaxMSP only: current inlet used by proxy objects
		long curinlet;      
#endif

    	/*! \brief This points to flext object class 
			This points to the actual polymorphic C++ class
		*/
        flext_obj           *data;
};


// ----------------------------------------------------------------------------
/*! \class flext_obj
	\brief The mother of base classes for all flext externs
	\internal

    Each extern which is written in C++ needs to use the #defines at the
    end of this header file.  
    
    The define
    
        FLEXT_HEADER(NEW_CLASS, PARENT_CLASS)
    
    should be somewhere in your header file.
    One of the defines like
    
    FLEXT_NEW(NEW_CLASS)
    FLEXT_NEW_2(NEW_CLASS, float, float)
    
    should be the first thing in your implementation file.
    NEW_CLASS is the name of your class and PARENT_CLASS is the 
    parent of your class.
*/
// ----------------------------------------------------------------------------

class FLEXT_EXT flext_obj:
	public flext
{
    public:

		// --- overloading of new/delete memory allocation methods ----
		// MaxMSP allows only 16K in overdrive mode!

		void *operator new(size_t bytes);
		void operator delete(void *blk);

		#ifndef __MRC__ // doesn't allow new[] overloading?!
		void *operator new[](size_t bytes) { return operator new(bytes); }
		void operator delete[](void *blk) { operator delete(blk); }
		#endif

		// these are aligned 
		static void *NewAligned(size_t bytes,int bitalign = 128);
		static void FreeAligned(void *blk);
		
		// ---------------------

        //! Constructor
    	flext_obj();

    	//! Destructor
    	virtual ~flext_obj() = 0;
    	
        //! Get the object's canvas
        t_canvas            *thisCanvas()        { return(m_canvas); }

		t_sigobj *thisHdr() { return &x_obj->obj; }
		const t_sigobj *thisHdr() const { return &x_obj->obj; }
		const char *thisName() const { return m_name; } 

#ifdef PD
		t_class *thisClass() { return (t_class *)((t_object *)(x_obj))->te_g.g_pd; }
#elif defined(MAXMSP)
		t_class *thisClass() { return (t_class *)(((t_tinyobject *)x_obj)->t_messlist-1); } 
#endif

		void InitProblem() { init_ok = false; }

// --- help -------------------------------------------------------	

		/*!	\defgroup FLEXT_C_HELP Flext help/assistance functionality

			@{ 
		*/

		static void DefineHelp(t_class *c,const char *ref,const char *dir = NULL,bool addtilde = false);
		void DefineHelp(const char *ref,const char *dir = NULL,bool addtilde = false) { DefineHelp(thisClass(),ref,dir,addtilde); }

		//!	@} 


    protected:    	
		
        //! The object header
        flext_hdr          *x_obj;        	

    private:

        //! The canvas (patcher) that the object is in
        t_canvas            *m_canvas;
        
        //! Flag for successful object construction
        bool				init_ok;

	public:

    	//! Creation callback
    	static void callb_setup(t_class *) {}	

		/*! \brief This is a temporary holder
			\warning don't touch it!
		*/
        static flext_hdr     *m_holder;
		//! Hold object's name during construction
        static const char *m_holdname;  

        //! The object's name in the patcher
		const char *m_name;

		//! Check whether construction was successful
		bool InitOk() const { return init_ok; }

//#ifdef MAXMSP
		//@{
		//! Definitions for MaxMSP external libraries 

		union lib_arg {
			int i;
			float f;
			t_symbol *s;
		};
		
		static t_class *lib_class;
		static t_symbol *lib_name;
		static void libfun_add(bool lib,bool dsp,t_class *&clss,const char *name,void setupfun(t_class *),t_newmethod newfun,void (*freefun)(flext_hdr *),int argtp1,...);
//		static void libfun_add(const char *name,t_newmethod newfun,void (*freefun)(flext_hdr *),int argtp1,...);
		static flext_hdr *libfun_new(t_symbol *s,int argc,t_atom *argv);
		static void libfun_free(flext_hdr *o);
		//@}
//#endif
};


//@{
//! Some utility functions for class setup 

namespace flext_util {
	const char *extract(const char *name,int ix = 0);
	char *strdup(const char *name);
	bool chktilde(const char *name);
}
//@}

// ----------------------------------------
// These should be used in the header
// ----------------------------------------

#define FLEXT_REALHDR(NEW_CLASS, PARENT_CLASS)    	    	\
public:     	    	    \
static t_class *__class__; \
typedef NEW_CLASS thisType;  \
static void callb_free(flext_hdr *hdr)    	    	    	\
{ flext_obj *mydata = hdr->data; delete mydata; \
  hdr->flext_hdr::~flext_hdr(); }   	    	\
static void callb_setup(t_class *classPtr)  	    	\
{ PARENT_CLASS::callb_setup(classPtr); }  	    	    	\
protected:    \
static inline NEW_CLASS *thisObject(void *c) { return FLEXT_CAST<NEW_CLASS *>(((flext_hdr *)c)->data); } 


#define FLEXT_REALHDR_S(NEW_CLASS, PARENT_CLASS,SETUPFUN)    	    	\
public:     	    	    \
static t_class *__class__; \
typedef NEW_CLASS thisType;  \
static void callb_free(flext_hdr *hdr)    	    	    	\
{ flext_obj *mydata = hdr->data; delete mydata; \
  hdr->flext_hdr::~flext_hdr(); }   	    	\
static void callb_setup(t_class *classPtr)  	    	\
{ PARENT_CLASS::callb_setup(classPtr);    	    	\
	NEW_CLASS::SETUPFUN(classPtr); }  	    	    	\
protected:    \
static inline NEW_CLASS *thisObject(void *c) { return FLEXT_CAST<NEW_CLASS *>(((flext_hdr *)c)->data); } 


// generate name of dsp/non-dsp setup function
#define FLEXT_STPF_0(NAME) NAME##_setup
#define FLEXT_STPF_2(NAME) NAME##_setup
#define FLEXT_STPF_1(NAME) NAME##_tilde_setup
#define FLEXT_STPF_3(NAME) NAME##_tilde_setup
#define FLEXT_STPF_(OTP) FLEXT_STPF_##OTP
#define FLEXT_STPF(NAME,OTP) FLEXT_STPF_(OTP)(NAME)


#define REAL_NEW(NAME,NEW_CLASS,DSP,LIB) REAL_INST(LIB,NAME,NEW_CLASS,DSP)
#define REAL_NEW_V(NAME,NEW_CLASS,DSP,LIB) REAL_INST_V(LIB,NAME,NEW_CLASS,DSP)
#define REAL_NEW_1(NAME,NEW_CLASS,DSP,LIB,TYPE1) REAL_INST_1(LIB,NAME,NEW_CLASS,DSP,TYPE1)
#define REAL_NEW_2(NAME,NEW_CLASS,DSP,LIB,TYPE1,TYPE2) REAL_INST_2(LIB,NAME,NEW_CLASS,DSP,TYPE1,TYPE2)
#define REAL_NEW_3(NAME,NEW_CLASS,DSP,LIB,TYPE1,TYPE2,TYPE3) REAL_INST_3(LIB,NAME,NEW_CLASS,DSP,TYPE1,TYPE2,TYPE3)
#define REAL_NEW_4(NAME,NEW_CLASS,DSP,LIB,TYPE1,TYPE2,TYPE3,TYPE4) REAL_INST_4(LIB,NAME,NEW_CLASS,DSP,TYPE1,TYPE2,TYPE3,TYPE4)


/* // 0.4.0

#ifdef PD
#define REAL_EXT(NEW_CLASS,DSP)
#define REAL_LIB(NAME,NEW_CLASS,DSP) REAL_INST(1,NAME,NEW_CLASS,DSP)
#define REAL_LIB_V(NAME,NEW_CLASS,DSP) REAL_INST_V(1,NAME,NEW_CLASS,DSP)
#define REAL_LIB_1(NAME,NEW_CLASS,DSP,TYPE1) REAL_INST_1(1,NAME,NEW_CLASS,DSP,TYPE1)
#define REAL_LIB_2(NAME,NEW_CLASS,DSP,TYPE1,TYPE2) REAL_INST_2(1,NAME,NEW_CLASS,DSP,TYPE1,TYPE2)
#define REAL_LIB_3(NAME,NEW_CLASS,DSP,TYPE1,TYPE2,TYPE3) REAL_INST_3(1,NAME,NEW_CLASS,DSP,TYPE1,TYPE2,TYPE3)
#else // MAXMSP
#define REAL_EXT(NEW_CLASS,DSP) extern "C" FLEXT_EXT int main() { FLEXT_STPF(NEW_CLASS,DSP)(); return 0; }
#define REAL_LIB(NAME,NEW_CLASS,DSP) REAL_NEWLIB(NAME,NEW_CLASS,DSP)
#define REAL_LIB_V(NAME,NEW_CLASS,DSP) REAL_NEWLIB_V(NAME,NEW_CLASS,DSP)
#define REAL_LIB_1(NAME,NEW_CLASS,DSP,TYPE1) REAL_NEWLIB_1(NAME,NEW_CLASS,DSP,TYPE1)
#define REAL_LIB_2(NAME,NEW_CLASS,DSP,TYPE1,TYPE2) REAL_NEWLIB_2(NAME,NEW_CLASS,DSP,TYPE1,TYPE2)
#define REAL_LIB_3(NAME,NEW_CLASS,DSP,TYPE1,TYPE2,TYPE3) REAL_NEWLIB_3(NAME,NEW_CLASS,DSP,TYPE1,TYPE2,TYPE3)
#endif

*/

#ifdef PD
#define REAL_EXT(NEW_CLASS,DSP)
#else // MAXMSP
#define REAL_EXT(NEW_CLASS,DSP) extern "C" FLEXT_EXT int main() { FLEXT_STPF(NEW_CLASS,DSP)(); return 0; }
#endif


// --------------------------------------------------------------------------------------


// --------------------------------------------------
// This is to be called in the library setup function
// ---------------------------------------------------

#define FLEXT_EXP_0 extern "C" FLEXT_EXT
#define FLEXT_EXP_1 
#define FLEXT_EXP(LIB) FLEXT_EXP_##LIB

#define FLEXT_SETUP(cl) \
extern void cl##_setup(); \
cl##_setup()  

#define FLEXT_DSP_SETUP(cl) \
extern void cl##_tilde_setup(); \
cl##_tilde_setup()  

#ifdef PD

#define FLEXT_LIB_SETUP(NAME,SETUPFUN) extern "C" FLEXT_EXT void NAME##_setup() { SETUPFUN(); }

#else // MAXMSP

#define FLEXT_LIB_SETUP(NAME,SETUPFUN) \
extern "C" FLEXT_EXT int main() { \
SETUPFUN(); \
flext_obj::lib_name = gensym(#NAME); \
::setup((t_messlist **)&flext_obj::lib_class,(t_newmethod)&flext_obj::libfun_new,(t_method)flext_obj::libfun_free,sizeof(flext_hdr), 0,A_GIMME,A_NULL); \
return 0; \
}

#endif

// ----------------------------------------
// These definitions are used below
// ----------------------------------------

// Shortcuts for PD/Max type arguments
#define FLEXTTYPE_void A_NULL
#define CALLBTYPE_void void
#define FLEXTTYPE_float A_FLOAT
#define CALLBTYPE_float float
#define FLEXTTYPE_t_float A_FLOAT
#define CALLBTYPE_t_float t_float

//* #define FLEXTTYPE_t_flint A_FLINT

#ifdef PD
#define FLEXTTYPE_int A_FLOAT
#define CALLBTYPE_int float
#else
#define FLEXTTYPE_int A_INT
#define CALLBTYPE_int int
#endif

#define FLEXTTYPE_t_symptr A_SYMBOL
#define CALLBTYPE_t_symptr t_symptr
#define FLEXTTYPE_t_symtype A_SYMBOL
#define CALLBTYPE_t_symtype t_symptr
#define FLEXTTYPE_t_ptrtype A_POINTER
#define CALLBTYPE_t_ptrtype t_ptrtype

#define FLEXTTP(TP) FLEXTTYPE_ ## TP
#define CALLBTP(TP) CALLBTYPE_ ## TP


#if 0 // 0.4.0

#ifdef PD
#define FLEXT_NEWFN ::class_new
#define FLEXT_CLREF(NAME,CLASS) gensym(const_cast<char *>(NAME))
//#define FLEXT_MAIN(MAINNAME) MAINNAME
#define CLNEW_OPTIONS 0  // flags for class creation
//#define LIB_INIT(NAME,NEWMETH,FREEMETH,ARG1,ARG2,ARG3,ARG4) ((void(0))
//#define LIB_INIT(NAME,NEWMETH,FREEMETH,ARGC) ((void(0))
//#define IS_PD 1
//#define IS_MAXMSP 0

#define newobject(CLSS) pd_new(CLSS)

//* #define ARGMEMBER_t_flint f

#elif defined(MAXMSP)
#define FLEXT_NEWFN NULL; ::setup    // extremely ugly!!! I hope Mark Danks doesn't see that......
#define FLEXT_CLREF(NAME,CLASS) (t_messlist **)&(CLASS)
//#define FLEXT_MAIN_0(MAINNAME) main // main for standalone object
//#define FLEXT_MAIN_1(MAINNAME) MAINNAME // main for library object
//#define FLEXT_MAIN(MAINNAME) main
#define CLNEW_OPTIONS 0  // flags for class creation

//#define LIB_INIT(NAME,NEWMETH,FREEMETH,ARG1,ARG2,ARG3,ARG4) flext_obj::libfun_add(NAME,(t_newmethod)(NEWMETH),FREEMETH,ARG1,ARG2,ARG3,ARG4,A_NULL) 

//#define IS_PD 0
//#define IS_MAXMSP 1

//* #define ARGMEMBER_t_flint i

#endif

#endif // 0.4.0

#define ARGMEMBER_int i //*
#define ARGMEMBER_float f
#define ARGMEMBER_t_symptr s
#define ARGMEMBER_t_symtype s
#define ARGCAST(arg,tp) arg.ARGMEMBER_##tp

//#define EXTPROTO_0  extern "C" FLEXT_EXT
//#define EXTPROTO_1
//#define EXTPROTO(LIB) EXTPROTO_ ## LIB

#if 0 // 0.4.0

#ifdef _DEBUG
#define CHECK_TILDE(OBJNAME,DSP) if(DSP) flext::chktilde(OBJNAME)
#else
#define CHECK_TILDE(OBJNAME,DSP) ((void)0)
#endif

#ifdef PD
#define FLEXT_ADDALIAS(NAME,FUN) ::class_addcreator((t_newmethod)FUN,::gensym(const_cast<char *>(NAME)),A_NULL)
#define FLEXT_ADDALIAS1(NAME,FUN,ARG1) ::class_addcreator((t_newmethod)FUN,::gensym(const_cast<char *>(NAME)),ARG1,A_NULL)
#define FLEXT_ADDALIAS2(NAME,FUN,ARG1,ARG2) ::class_addcreator((t_newmethod)FUN,::gensym(const_cast<char *>(NAME)),ARG1,ARG2,A_NULL)
#define FLEXT_ADDALIAS3(NAME,FUN,ARG1,ARG2,ARG3) ::class_addcreator((t_newmethod)FUN,::gensym(const_cast<char *>(NAME)),ARG1,ARG2,ARG3,A_NULL)
#define FLEXT_ADDALIAS4(NAME,FUN,ARG1,ARG2,ARG3,ARG4) ::class_addcreator((t_newmethod)FUN,::gensym(const_cast<char *>(NAME)),ARG1,ARG2,ARG3,ARG4,A_NULL)
#elif defined(MAXMSP)
#define FLEXT_ADDALIAS(NAME,FUN) ::alias(const_cast<char *>(NAME))
#define FLEXT_ADDALIAS1(NAME,FUN,ARG1) FLEXT_ADDALIAS(NAME,FUN)
#define FLEXT_ADDALIAS2(NAME,FUN,ARG1,ARG2) FLEXT_ADDALIAS(NAME,FUN)
#define FLEXT_ADDALIAS3(NAME,FUN,ARG1,ARG2,ARG3) FLEXT_ADDALIAS(NAME,FUN)
#define FLEXT_ADDALIAS4(NAME,FUN,ARG1,ARG2,ARG3,ARG4) FLEXT_ADDALIAS(NAME,FUN)
#else
#error "No alias function defined!"
#endif


#define FLEXT_HELPSTR(NAME) #NAME 
#define FLEXT_HELPSTR_DSP(NAME) #NAME "~"

#ifdef PD
#define FLEXT_DEFHELP(THIS,NAME,NEW_CLASS,OTP) FLEXT_CAST<NEW_CLASS *>(THIS)->DefineHelp((OTP&1)?FLEXT_HELPSTR_DSP(NEW_CLASS):FLEXT_HELPSTR(NEW_CLASS),flext::extract(NAME,-1))
#define FLEXT_CLOPTS(NEW_CLASS,OTP) CLNEW_OPTIONS
#else
#define FLEXT_DEFHELP(THIS,NAME,NEW_CLASS,OTP)
#define FLEXT_CLOPTS(NEW_CLASS,OTP) NULL
#endif

#endif // 0.4.0

// ----------------------------------------------------
// These should never be called or used directly!!!
//
//
// ----------------------------------------------------


#if 0

// ----------------------------------------------------
// no args
// ----------------------------------------------------
#define REAL_INST(LIB,NAME,NEW_CLASS, DSP) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS () \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(NEW_CLASS::__class__); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS;                     \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
    NEW_CLASS::__class__ = FLEXT_NEWFN(                       \
		FLEXT_CLREF(flext::extract(NAME),NEW_CLASS::__class__), 	    	    	    	\
    	(t_newmethod)class_ ## NEW_CLASS,	    	\
    	(t_method)&NEW_CLASS::callb_free,         \
     	sizeof(flext_hdr), FLEXT_CLOPTS(NEW_CLASS,DSP),                          \
     	A_NULL);      	    	    	    	    	\
	for(int ix = 1; ; ++ix) { \
		const char *c = flext::extract(NAME,ix); if(!c) break; \
		FLEXT_ADDALIAS(c,(t_newmethod)class_ ## NEW_CLASS); \
	} \
    NEW_CLASS::callb_setup(NEW_CLASS::__class__); \
} 

#define REAL_NEWLIB(NAME,NEW_CLASS, DSP) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS ()    \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(flext_obj::lib_class); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS;      \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
void FLEXT_STPF(NEW_CLASS,DSP)()   	\
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
	NEW_CLASS::__class__ = flext_obj::lib_class; \
    flext_obj::libfun_add(NAME,(t_method)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,A_NULL); \
    NEW_CLASS::callb_setup(flext_obj::lib_class); \
}   


// ----------------------------------------------------
// variable arg list
// ----------------------------------------------------
#define REAL_INST_V(LIB,NAME,NEW_CLASS, OTP) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (t_symbol *,int argc,t_atom *argv) \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(NEW_CLASS::__class__); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS(argc,argv);                     \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,OTP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,OTP)()   \
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,OTP&1); 	\
    NEW_CLASS::__class__ = FLEXT_NEWFN(                       \
		FLEXT_CLREF(flext::extract(NAME),NEW_CLASS::__class__), 	    	    	    	\
    	(t_newmethod)class_ ## NEW_CLASS,	    	\
    	(t_method)&NEW_CLASS::callb_free,         \
     	sizeof(flext_hdr), FLEXT_CLOPTS(NEW_CLASS,OTP),                          \
     	A_GIMME,                       \
     	A_NULL);      	    	    	    	    	\
	for(int ix = 1; ; ++ix) { \
		const char *c = flext::extract(NAME,ix); if(!c) break; \
		FLEXT_ADDALIAS1(c,(t_newmethod)class_ ## NEW_CLASS,A_GIMME); \
	} \
    NEW_CLASS::callb_setup(NEW_CLASS::__class__); \
} 

#define REAL_NEWLIB_V(NAME,NEW_CLASS, OTP) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (t_symbol *,int argc,t_atom *argv)    \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(flext_obj::lib_class); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS(argc,argv);      \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,OTP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
void FLEXT_STPF(NEW_CLASS,OTP)()   	\
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,OTP&1); 	\
	NEW_CLASS::__class__ = flext_obj::lib_class; \
    flext_obj::libfun_add(NAME,(t_method)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,A_GIMME,A_NULL); \
    NEW_CLASS::callb_setup(flext_obj::lib_class); \
}   


// ----------------------------------------------------
// one arg
// ----------------------------------------------------
#define REAL_INST_1(LIB,NAME,NEW_CLASS, DSP, TYPE1) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (CALLBTP(TYPE1) arg1) \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(NEW_CLASS::__class__); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS((TYPE1)arg1);                     \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
    NEW_CLASS::__class__ = FLEXT_NEWFN(                       \
		FLEXT_CLREF(flext::extract(NAME),NEW_CLASS::__class__), 	    	    	    	\
    	(t_newmethod)class_ ## NEW_CLASS,	    	\
    	(t_method)&NEW_CLASS::callb_free,         \
     	sizeof(flext_hdr), FLEXT_CLOPTS(NEW_CLASS,DSP),                          \
     	FLEXTTP(TYPE1),                       \
     	A_NULL);      	    	    	    	    	\
	for(int ix = 1; ; ++ix) { \
		const char *c = flext::extract(NAME,ix); if(!c) break; \
		FLEXT_ADDALIAS1(c,(t_newmethod)class_ ## NEW_CLASS,FLEXTTP(TYPE1)); \
	} \
    NEW_CLASS::callb_setup(NEW_CLASS::__class__); \
} 

#define REAL_NEWLIB_1(NAME,NEW_CLASS, DSP,TYPE1) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1)    \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(flext_obj::lib_class); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS(ARGCAST(arg1,TYPE1));      \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
void FLEXT_STPF(NEW_CLASS,DSP)()   	\
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
	NEW_CLASS::__class__ = flext_obj::lib_class; \
    flext_obj::libfun_add(NAME,(t_newmethod)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),A_NULL); \
    NEW_CLASS::callb_setup(NEW_CLASS::__class__); \
}   


// ----------------------------------------------------
// two args
// ----------------------------------------------------
#define REAL_INST_2(LIB,NAME,NEW_CLASS, DSP, TYPE1, TYPE2) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (CALLBTP(TYPE1) arg1, CALLBTP(TYPE2) arg2) \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(NEW_CLASS::__class__); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS((TYPE1)arg1, (TYPE2)arg2);                     \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
    NEW_CLASS::__class__ = FLEXT_NEWFN(                       \
		FLEXT_CLREF(flext::extract(NAME),NEW_CLASS::__class__), 	    	    	    	\
    	(t_newmethod)class_ ## NEW_CLASS,	    	\
    	(t_method)&NEW_CLASS::callb_free,         \
     	sizeof(flext_hdr), FLEXT_CLOPTS(NEW_CLASS,DSP),                          \
     	FLEXTTP(TYPE1), FLEXTTP(TYPE2),                       \
     	A_NULL);      	    	    	    	    	\
	for(int ix = 1; ; ++ix) { \
		const char *c = flext::extract(NAME,ix); if(!c) break; \
		FLEXT_ADDALIAS2(c,(t_newmethod)class_ ## NEW_CLASS,FLEXTTP(TYPE1),FLEXTTP(TYPE2)); \
	} \
    NEW_CLASS::callb_setup(NEW_CLASS::__class__); \
} 

#define REAL_NEWLIB_2(NAME,NEW_CLASS, DSP,TYPE1, TYPE2) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1,const flext_obj::lib_arg &arg2)    \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(flext_obj::lib_class); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS(ARGCAST(arg1,TYPE1),ARGCAST(arg2,TYPE2));      \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
void FLEXT_STPF(NEW_CLASS,DSP)()   	\
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
	NEW_CLASS::__class__ = flext_obj::lib_class; \
    flext_obj::libfun_add(NAME,(t_method)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),FLEXTTP(TYPE2),A_NULL); \
    NEW_CLASS::callb_setup(flext_obj::lib_class); \
}   


// ----------------------------------------------------
// three args
// ----------------------------------------------------
#define REAL_INST_3(LIB,NAME,NEW_CLASS, DSP, TYPE1,TYPE2,TYPE3) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (CALLBTP(TYPE1) arg1,CALLBTP(TYPE2) arg2,CALLBTP(TYPE3) arg3) \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(NEW_CLASS::__class__); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS((TYPE1)arg1,(TYPE2)arg2,(TYPE3)arg3);                     \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
    NEW_CLASS::__class__ = FLEXT_NEWFN(                       \
		FLEXT_CLREF(flext::extract(NAME),NEW_CLASS::__class__), 	    	    	    	\
    	(t_newmethod)class_ ## NEW_CLASS,	    	\
    	(t_method)&NEW_CLASS::callb_free,         \
     	sizeof(flext_hdr), FLEXT_CLOPTS(NEW_CLASS,DSP),                          \
     	FLEXTTP(TYPE1), FLEXTTP(TYPE2),FLEXTTP(TYPE3),                       \
     	A_NULL);      	    	    	    	    	\
	for(int ix = 1; ; ++ix) { \
		const char *c = flext::extract(NAME,ix); if(!c) break; \
		FLEXT_ADDALIAS3(c,(t_newmethod)class_ ## NEW_CLASS,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3)); \
	} \
    NEW_CLASS::callb_setup(NEW_CLASS::__class__); \
} 

#define REAL_NEWLIB_3(NAME,NEW_CLASS, DSP,TYPE1,TYPE2,TYPE3) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1,const flext_obj::lib_arg &arg2,const flext_obj::lib_arg &arg3)    \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(flext_obj::lib_class); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS(ARGCAST(arg1,TYPE1),ARGCAST(arg2,TYPE2),ARGCAST(arg3,TYPE3));      \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
void FLEXT_STPF(NEW_CLASS,DSP)()   	\
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
	NEW_CLASS::__class__ = flext_obj::lib_class; \
    flext_obj::libfun_add(NAME,(t_method)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3),A_NULL); \
    NEW_CLASS::callb_setup(flext_obj::lib_class); \
}   

// ----------------------------------------------------
// four args
// ----------------------------------------------------
#define REAL_INST_4(LIB,NAME,NEW_CLASS, DSP, TYPE1,TYPE2,TYPE3,TYPE4) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (CALLBTP(TYPE1) arg1,CALLBTP(TYPE2) arg2,CALLBTP(TYPE3) arg3,CALLBTP(TYPE4) arg4) \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(NEW_CLASS::__class__); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS((TYPE1)arg1,(TYPE2)arg2,(TYPE3)arg3,(TYPE4)arg4);                     \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
	NEW_CLASS::__class__ = FLEXT_NEWFN(                       \
     	FLEXT_CLREF(flext::extract(NAME),NEW_CLASS::__class__), 	    	    	    	\
    	(t_newmethod)class_ ## NEW_CLASS,	    	\
    	(t_method)&NEW_CLASS::callb_free,         \
     	sizeof(flext_hdr), FLEXT_CLOPTS(NEW_CLASS,DSP),                          \
     	FLEXTTP(TYPE1), FLEXTTP(TYPE2),FLEXTTP(TYPE3),FLEXTTP(TYPE4),                     \
     	A_NULL);      	    	    	    	    	\
	for(int ix = 1; ; ++ix) { \
		const char *c = flext::extract(NAME,ix); if(!c) break; \
		FLEXT_ADDALIAS4(c,(t_newmethod)class_ ## NEW_CLASS,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3),FLEXTTP(TYPE4)); \
	} \
    NEW_CLASS::callb_setup(NEW_CLASS::__class__); \
} 

#define REAL_NEWLIB_4(NAME,NEW_CLASS, DSP,TYPE1,TYPE2,TYPE3,TYPE4) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
flext_hdr* class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1,const flext_obj::lib_arg &arg2,const flext_obj::lib_arg &arg3,const flext_obj::lib_arg &arg4)    \
{     	    	    	    	    	    	    	    	\
    flext_hdr *obj = (flext_hdr *)newobject(flext_obj::lib_class); \
    flext_obj::m_holder = obj;                         \
    flext_obj::m_holdname = flext::extract(NAME);                         \
    obj->data = new NEW_CLASS(ARGCAST(arg1,TYPE1),ARGCAST(arg2,TYPE2),ARGCAST(arg3,TYPE3),ARGCAST(arg4,TYPE4));      \
    flext_obj::m_holder = NULL;                                 \
    if(!obj->data->InitOk()) { NEW_CLASS::callb_free(obj); obj = NULL; } \
	else FLEXT_DEFHELP(obj->data,NAME,NEW_CLASS,DSP); \
    return(obj);                                                \
}   	    	    	    	    	    	    	    	\
void FLEXT_STPF(NEW_CLASS,DSP)()   	\
{   	    	    	    	    	    	    	    	\
	CHECK_TILDE(NAME,DSP); 	\
	NEW_CLASS::__class__ = flext_obj::lib_class; \
    flext_obj::libfun_add(NAME,(t_method)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3),FLEXTTP(TYPE4),A_NULL); \
    NEW_CLASS::callb_setup(flext_obj::lib_class); \
}   

#else

#define REAL_INST(LIB,NAME,NEW_CLASS, DSP) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
static flext_obj *class_ ## NEW_CLASS () \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS;                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::libfun_add(LIB,DSP,NEW_CLASS::__class__,NAME,NEW_CLASS::callb_setup,(t_newmethod)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,A_NULL); \
} 

#define REAL_INST_V(LIB,NAME,NEW_CLASS, DSP) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
static flext_obj *class_ ## NEW_CLASS (int argc,t_atom *argv) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(argc,argv);                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::libfun_add(LIB,DSP,NEW_CLASS::__class__,NAME,NEW_CLASS::callb_setup,(t_newmethod)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,A_GIMME,A_NULL); \
}

#define REAL_INST_1(LIB,NAME,NEW_CLASS, DSP, TYPE1) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
static flext_obj *class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(arg1,TYPE1));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::libfun_add(LIB,DSP,NEW_CLASS::__class__,NAME,NEW_CLASS::callb_setup,(t_newmethod)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),A_NULL); \
} 

#define REAL_INST_2(LIB,NAME,NEW_CLASS, DSP, TYPE1,TYPE2) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
static flext_obj *class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1,const flext_obj::lib_arg &arg2) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(arg1,TYPE1),ARGCAST(arg2,TYPE2));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::libfun_add(LIB,DSP,NEW_CLASS::__class__,NAME,NEW_CLASS::callb_setup,(t_newmethod)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),FLEXTTP(TYPE2),A_NULL); \
} 

#define REAL_INST_3(LIB,NAME,NEW_CLASS, DSP, TYPE1, TYPE2, TYPE3) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
static flext_obj *class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1,const flext_obj::lib_arg &arg2,const flext_obj::lib_arg &arg3) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(arg1,TYPE1),ARGCAST(arg2,TYPE2),ARGCAST(arg3,TYPE3));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::libfun_add(LIB,DSP,NEW_CLASS::__class__,NAME,NEW_CLASS::callb_setup,(t_newmethod)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3),A_NULL); \
} 

#define REAL_INST_4(LIB,NAME,NEW_CLASS, DSP, TYPE1,TYPE2, TYPE3, TYPE4) \
t_class * NEW_CLASS::__class__ = NULL;    	    	    	\
static flext_obj *class_ ## NEW_CLASS (const flext_obj::lib_arg &arg1) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(arg1,TYPE1),ARGCAST(arg2,TYPE2),ARGCAST(arg3,TYPE3),ARGCAST(arg4,TYPE4));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::libfun_add(LIB,DSP,NEW_CLASS::__class__,NAME,NEW_CLASS::callb_setup,(t_newmethod)(class_ ## NEW_CLASS),&NEW_CLASS::callb_free,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3),FLEXTTP(TYPE4),A_NULL); \
} 

/*
#define REAL_NEWLIB(NAME,NEW_CLASS, DSP) REAL_INST(1,NAME,NEW_CLASS, DSP)
#define REAL_NEWLIB_V(NAME,NEW_CLASS, DSP) REAL_INST_V(1,NAME,NEW_CLASS, DSP)
#define REAL_NEWLIB_1(NAME,NEW_CLASS, DSP,TYPE1) REAL_INST_1(1,NAME,NEW_CLASS, DSP,TYPE1)
#define REAL_NEWLIB_2(NAME,NEW_CLASS, DSP,TYPE1,TYPE2) REAL_INST_2(1,NAME,NEW_CLASS, DSP,TYPE1,TYPE2)
#define REAL_NEWLIB_3(NAME,NEW_CLASS, DSP,TYPE1,TYPE2,TYPE3) REAL_INST_3(1,NAME,NEW_CLASS, DSP,TYPE1,TYPE2,TYPE3)
#define REAL_NEWLIB_4(NAME,NEW_CLASS, DSP,TYPE1,TYPE2,TYPE3,TYPE4) REAL_INST_4(1,NAME,NEW_CLASS, DSP,TYPE1,TYPE2,TYPE3,TYPE4)
*/
#endif


// Shortcuts for method arguments:
#define FLEXTARG_float a_float
#define FLEXTARG_int a_int
#define FLEXTARG_bool a_int
#define FLEXTARG_t_float a_float
#define FLEXTARG_t_symtype a_symbol
#define FLEXTARG_t_symptr a_symbol
#define FLEXTARG_t_ptrtype a_pointer

#define FLEXTARG(TP) FLEXTARG_ ## TP


#endif




