/*
 * Gurobify: Gurobify provides an interface to Gurobi from GAP.
 * Jesse Lansdown
 */

#include "src/compiled.h"          /* GAP headers */
#include "gurobi_c.h"
#include <stdlib.h>
#include <stdio.h>


Obj GurobiSolve(Obj self, Obj lp_file, Obj ParameterArguments )
{

	//TODO: Add option of additional constraints
	//TODO: Add numeric focus parameter - infact, all parameters?
	int number_of_arguments = 7;

	if( ! IS_PLIST( ParameterArguments ) ){
        ErrorMayQuit( "Error: arguments is not a list!", 0, 0 );
    }
	int length = LEN_PLIST( ParameterArguments );
    if (length != number_of_arguments){
        ErrorMayQuit("Error: wrong number of parameter-arguments!", 0, 0);
    }

    Obj TimeLimitON = ELM_PLIST( ParameterArguments, 1 );
    Obj TimeLimitValue = ELM_PLIST( ParameterArguments, 2 );
    Obj BestObjStopON = ELM_PLIST( ParameterArguments, 3 );
    Obj BestObjStopValue = ELM_PLIST( ParameterArguments, 4 );
    Obj CutOffON = ELM_PLIST( ParameterArguments, 5 );
    Obj CutOffValue = ELM_PLIST( ParameterArguments, 6 );
    Obj ConsoleOutputON = ELM_PLIST( ParameterArguments, 7 );

    GRBenv *env = NULL;
    GRBmodel *model = NULL;
    GRBenv *modelenv = NULL;

    int optimstatus;
	double objval;
    int error = 0;

    //TODO: Check that lp_file is actually a string
    char *lp_file_name = CSTR_STRING(lp_file);

    error = GRBloadenv(&env, NULL);     // We are not interested in a log file, so the second argument of GRBloadenv is NULL
    if (error || env == NULL)
        ErrorMayQuit( "Error: failed to create new environment.", 0, 0 );
    
    if (ConsoleOutputON != True && ConsoleOutputON != False){
    	ErrorMayQuit( "Error: ConsoleOutputON requires a true/false switch.", 0, 0 );
    }
    else{
    	if (ConsoleOutputON != True){
	   		error = GRBsetintparam(env, "LogToConsole", 0);
			   	if (error)
			        ErrorMayQuit( "Error: LogToConsole not updated correctly.", 0, 0 );
    	}
	}

    if ( (TimeLimitON != True) && (TimeLimitON != False) ) {
    	ErrorMayQuit( "Error: TimeLimitON requires a true/false switch.", 0, 0 );
    }
    else{
    	if (TimeLimitON == True ){
    		if (IS_INTOBJ(TimeLimitValue) || ! IS_MACFLOAT(TimeLimitValue)){
    	    	ErrorMayQuit( "Error: TimeLimitValue requires a double.", 0, 0 );
			}
			else{
			   	error = GRBsetdblparam(env, "TimeLimit", VAL_MACFLOAT(TimeLimitValue) );
			   	if (error)
			        ErrorMayQuit( "Error: TimeLimit not updated correctly.", 0, 0 );
			}
    	}
	}

	if ( (BestObjStopON != True) && (BestObjStopON != False) ) {
    	ErrorMayQuit( "Error: BestObjStopON requires a true/false switch.", 0, 0 );
    }
    else{
    	if (BestObjStopON == True ){
    		if (IS_INTOBJ(BestObjStopValue) || ! IS_MACFLOAT(BestObjStopValue)){
    	    	ErrorMayQuit( "Error: CutOffValue requires a double.", 0, 0 );
			}
			else{
			   	error = GRBsetdblparam(env, "BestObjStop", VAL_MACFLOAT(BestObjStopValue) );
			   	if (error)
			        ErrorMayQuit( "Error: BestObjStop not updated correctly.", 0, 0 );
			}
    	}
	}


	if ( (CutOffON != True) && (CutOffON != False) ) {
    	ErrorMayQuit( "Error: CutOffON requires a true/false switch.", 0, 0 );
    }
    else{
    	if (CutOffON == True ){
    		if (IS_INTOBJ(CutOffValue) || ! IS_MACFLOAT(CutOffValue)){
    	    	ErrorMayQuit( "Error: CutOffValue requires a double.", 0, 0 );
			}
			else{
			   	error = GRBsetdblparam(env, GRB_DBL_PAR_CUTOFF, VAL_MACFLOAT(CutOffValue) );
			   	if (error)
			        ErrorMayQuit( "Error: Cutoff not updated correctly.", 0, 0 );
			}
    	}
	}



	error = GRBsetdblparam(env, GRB_DBL_PAR_CUTOFF, 2.0 );




    error = GRBreadmodel(env, lp_file_name, &model);


    if (error)
        ErrorMayQuit( "Error: model was not read correctly.", 0, 0 );

    error = GRBoptimize(model);

    if (error)
        ErrorMayQuit( "Error: model was not able to be optimized", 0, 0 );

    error = GRBgetintattr(model, GRB_INT_ATTR_STATUS, &optimstatus);
    if (error)
        ErrorMayQuit( "Error: unable to obtain optimisation status", 0, 0 );
	
	Obj results = NEW_PLIST( T_PLIST , 3);
	SET_LEN_PLIST( results , 3 );
    
    if (optimstatus == GRB_OPTIMAL){
	    error = GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, &objval);			//TODO: First check that the optimstatus returned a success
	    if (error)
	        ErrorMayQuit( "Error: unable to obtain optimal value", 0, 0 );

	    int number_of_variables;
	    error = GRBgetintattr(model, "NumVars", &number_of_variables);		//TODO: check for errors
	    double sol[number_of_variables];
	    error = GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, number_of_variables, sol);		//TODO: check for errors
	    
	    Obj solution = NEW_PLIST( T_PLIST , number_of_variables);
		SET_LEN_PLIST( solution , number_of_variables );				// For some reason this gives a warning. 
																		// When number of variables is replaced by
																		// the actual number it gives no warning and
																		// causes no trouble
	    int i;
	    for (i = 0; i < number_of_variables; i = i+1 ){
			SET_ELM_PLIST(solution, i+1, INTOBJ_INT(sol[i]));		//Need to check that it is infact an integer (ie not just for MIPs)
	//		SET_ELM_PLIST(solution, i+1, NEW_MACFLOAT(sol[i]));		//This line gives the solution as doubles, but unnecessary when not a MIP
	    }

		SET_ELM_PLIST(results, 1, INTOBJ_INT(optimstatus));
		SET_ELM_PLIST(results, 2, NEW_MACFLOAT(objval));			//TODO: Check if can return an integer first, otherwise return a float
		SET_ELM_PLIST(results, 3, solution);
	}
	else{
		Obj unsure = NEW_STRING(3);
		C_NEW_STRING_CONST(unsure, "?");
		Obj empty_solution = NEW_PLIST( T_PLIST , 0);
		SET_LEN_PLIST( empty_solution , 0);
		SET_ELM_PLIST(results, 1, INTOBJ_INT(optimstatus));
		SET_ELM_PLIST(results, 2, unsure);			//TODO: Check if can return an integer first, otherwise return a float
		SET_ELM_PLIST(results, 3, empty_solution);
	}
	GRBfreemodel(model);
	GRBfreeenv(env);

    return results;
}

Obj TestCommandWithParams(Obj self, Obj param, Obj param2)
{
    /* simply return the first parameter */
    if( ! IS_PLIST( param ) ){
        ErrorMayQuit( "Param is not a list!", 0, 0 );
    }
    if( ! IS_INTOBJ(param2) ){
        ErrorMayQuit( "Param2 is not an integer!", 0, 0);
    }
    int index = INT_INTOBJ(param2);
    int length = LEN_PLIST( param );
    if (index > length){
        ErrorMayQuit("Param2 is larger than the size of param!", 0, 0);
    }
    Obj a = ELM_PLIST( param, index );
    int b = INT_INTOBJ(a);
    Obj return_val = INTOBJ_INT( b );

    return return_val;
}


typedef Obj (* GVarFunc)(/*arguments*/);

#define GVAR_FUNC_TABLE_ENTRY(srcfile, name, nparam, params) \
  {#name, nparam, \
   params, \
   (GVarFunc)name, \
   srcfile ":Func" #name }

// Table of functions to export
static StructGVarFunc GVarFuncs [] = {
    GVAR_FUNC_TABLE_ENTRY("Gurobify.c", GurobiSolve, 2, "lp_file,  TimeLimitON, TimeLimitValue, CutOffON, CutOffValue, ConsoleOutputON"),
    GVAR_FUNC_TABLE_ENTRY("Gurobify.c", TestCommandWithParams, 2, "param, param2"),

  { 0 } /* Finish with an empty entry */

};

/******************************************************************************
*F  InitKernel( <module> )  . . . . . . . . initialise kernel data structures
*/
static Int InitKernel( StructInitInfo *module )
{
    /* init filters and functions                                          */
    InitHdlrFuncsFromTable( GVarFuncs );

    /* return success                                                      */
    return 0;
}

/******************************************************************************
*F  InitLibrary( <module> ) . . . . . . .  initialise library data structures
*/
static Int InitLibrary( StructInitInfo *module )
{
    /* init filters and functions */
    InitGVarFuncsFromTable( GVarFuncs );

    /* return success                                                      */
    return 0;
}

/******************************************************************************
*F  InitInfopl()  . . . . . . . . . . . . . . . . . table of init functions
*/
static StructInitInfo module = {
 /* type        = */ MODULE_DYNAMIC,
 /* name        = */ "Gurobify",
 /* revision_c  = */ 0,
 /* revision_h  = */ 0,
 /* version     = */ 0,
 /* crc         = */ 0,
 /* initKernel  = */ InitKernel,
 /* initLibrary = */ InitLibrary,
 /* checkInit   = */ 0,
 /* preSave     = */ 0,
 /* postSave    = */ 0,
 /* postRestore = */ 0
};

StructInitInfo *Init__Dynamic( void )
{
    return &module;
}
