/*
 * Copyright (c) 2007-2012 Carnegie Mellon University.
 * 
 * All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following acknowledgments
 * and disclaimers.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 * 
 * 3. The names “Carnegie Mellon University,” "CERT” and/or “Software
 * Engineering Institute" shall not be used to endorse or promote
 * products derived from this software without prior written
 * permission. For written permission, please contact
 * permission@sei.cmu.edu.
 * 
 * 4. Products derived from this software may not be called "CERT" nor
 * may "CERT" appear in their names without prior written permission
 * of permission@sei.cmu.edu.
 * 
 * 5. Redistributions of any form whatsoever must retain the following
 * acknowledgment:
 * 
 * "This product includes software developed by CERT with funding and
 * support from the Department of Defense under Contract No. FA
 * 8721-05-C-0003.  The U.S. Government's rights to use, modify,
 * reproduce, release, perform, display, or disclose this material are
 * restricted by the Rights in Technical Data-Noncommercial Items
 * clauses (DFAR 252-227.7013 and DFAR 252-227.7013 Alternate I
 * contained in the foregoing identified contract.
 * 
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY “AS IS” AND
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AS TO ANY MATTER, AND ALL SUCH WARRANTIES,
 * INCLUDING WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE, ARE EXPRESSLY DISCLAIMED.  WITHOUT LIMITING THE
 * GENERALITY OF THE FOREGOING, CARNEGIE MELLON UNIVERSITY DOES NOT
 * MAKE ANY WARRANTY OF ANY KIND RELATING TO EXCLUSIVITY,
 * INFORMATIONAL CONTENT, ERROR-FREE OPERATION, RESULTS TO BE OBTAINED
 * FROM USE, FREEDOM FROM PATENT, TRADEMARK AND COPYRIGHT INFRINGEMENT
 *  AND/OR FREEDOM FROM THEFT OF TRADE SECRETS.”
 */

/**
 * \file POS.C
 */

#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include "rose.h"
#include "utilities.h"
#include <assert.h>

/**
 * Use the readlink() function properly
 *
 * \note, we do this by ensuring the next line after a readlink is comparison
 * of the return to -1
 */
bool POS30_C( const SgNode *node ) {
	const SgFunctionRefExp *fnRef = isSgFunctionRefExp(node);
	if (!fnRef)
		return false;
	if (!isCallOfFunctionNamed(fnRef, "readlink"))
		return false;

	const SgInitializedName *var = getVarAssignedTo(fnRef, NULL);
	/**
	 * First, let's see if we are checking the return value as part of a
	 * complicated expression
	 */
	const SgBinaryOp *compOp = findParentOfType(fnRef, SgEqualityOp);
	if (!compOp)
		compOp = findParentOfType(fnRef, SgNotEqualOp);
	/**
	 * If not, let's try to look for it on the next line
	 */
	if (!compOp && var) {
		const SgStatement *nextSt = findInBlockByOffset(fnRef, 1);
		if (nextSt) {
			FOREACH_SUBNODE(nextSt, nodes, i, V_SgBinaryOp) {
				const SgBinaryOp *iOp = isSgBinaryOp(*i);
				assert(iOp);
				/**
				 * We are looking for == or !=
				 */
				if (!(isSgEqualityOp(iOp) || isSgNotEqualOp(iOp)))
					continue;
				/**
				 * And one of the operands has to be our saved return value
				 */
				const SgVarRefExp *iVar = isSgVarRefExp(removeImplicitPromotions(iOp->get_lhs_operand()));
				if (!iVar)
					iVar = isSgVarRefExp(removeImplicitPromotions(iOp->get_rhs_operand()));
				if (iVar && (getRefDecl(iVar) == var)) {
					compOp = iOp;
					break;
				}
			}
		}
	}
	/**
	 * Now that we found our comparison, make sure it's being checked against
	 * the correct value
	 */
	if (compOp) {
		const SgExpression *expr = removeImplicitPromotions(compOp->get_lhs_operand());
		if (isVal(expr, -1))
			return false;
		expr = removeImplicitPromotions(compOp->get_rhs_operand());
		if (isVal(expr, -1))
			return false;
	}
	/**
	 * If for whatever reason we didn't find a good comparison, that's cause
	 * to worry
	 */
	print_error(node, "POS30-C", "Use the readlink() function properly");
	return true;;
}

/**
 * Do not use vfork (was overload operator&&)
 */
bool POS33_C( const SgNode *node ) {
	const SgFunctionRefExp *fnRef = isSgFunctionRefExp(node);
	if (!(fnRef && isCallOfFunctionNamed(fnRef, "vfork")))
		return false;
	print_error( node, "POS33-C", "Do not use vfork()");
	return true;
}

/**
 * Do not call putenv() with auto var
 */
bool POS34_C( const SgNode *node ) {
	const SgFunctionRefExp *fnRef = isSgFunctionRefExp(node);
	if (!(fnRef && isCallOfFunctionNamed(fnRef, "putenv")))
		return false;

	// ok, bail iff putenv's arg is a char* (not char[])
	const SgExpression *arg0 = getFnArg(fnRef, 0);
	assert( arg0 != NULL);

	// bail iff putenv's arg is a static variable
	const SgVarRefExp* var = isSgVarRefExp( arg0);
	if (var == NULL) return false; // we don't handle non-var putenv args

	const SgInitializedName* decl = getRefDecl(var);
	assert(decl);
	if (isGlobalVar(decl) || isStaticVar(decl))
		return false;

	/**
	 * \todo We only know how to deal with arrays for now
	 */
	if (!isSgArrayType(arg0->get_type())) {
	  
	  //TODO - Need to try to catch AssignOps first, and return if we do.

	  /**
	   * Catch assignments like char *var = &auto[0];
	   */

	  const SgAssignInitializer *init = isSgAssignInitializer(decl->get_initializer());
	  if(init != NULL) {
	    const SgExpression *rhs = init->get_operand();
	    if(isSgArrayType(rhs->get_type())) {
	      print_error( node, "POS34-C", "Do not call putenv() with an automatic variable");
	      return true;
	    }
	  }

	  return false;
	}	

	print_error( node, "POS34-C", "Do not call putenv() with an automatic variable");
	return true;
}

/**
 * Avoid race conditions while checking for the existence of a symbolic link
 */
bool POS35_C( const SgNode *node ) {
	const SgFunctionRefExp *fnRef = isSgFunctionRefExp(node);
	if (!fnRef)
		return false;

	bool need_fileno = false;
	if (isCallOfFunctionNamed(fnRef, "fopen"))
		need_fileno = true;
	else if (!isCallOfFunctionNamed(fnRef, "open"))
		return false;

	const SgExpression *fnExp = removeImplicitPromotions(getFnArg(fnRef,0));
	assert(fnExp);

	const SgVarRefExp *ref = isSgVarRefExp(fnExp);
	if (!ref)
		return false;
	const SgInitializedName *var = getRefDecl(ref);

	const SgInitializedName *fd = getVarAssignedTo(fnRef, NULL);
	if (!fd) {
		/**
		 * Ignoring the return value here is a violation of EXP12-C
		 * Or alternatively, we might just be returning the value
		 */
		return false;
	}

	const SgFunctionRefExp *iFn = NULL;
	const SgVarRefExp *iVar = NULL;

	bool lstat = false;
	bool after = false;
	bool fstat = false;

	FOREACH_SUBNODE(findParentOfType(node, SgFunctionDefinition), nodes, i, V_SgFunctionRefExp) { 
		iFn = isSgFunctionRefExp(*i);
		assert(iFn);

		if (iFn == fnRef) {
			after = true;
			continue;
		}

		if (!(isCallOfFunctionNamed(iFn, "lstat")
			||isCallOfFunctionNamed(iFn, "fstat")))
			continue;

		iVar = isSgVarRefExp(removeImplicitPromotions(getFnArg(iFn,0)));
		if (!iVar)
			continue;

		if (!after && isCallOfFunctionNamed(iFn, "lstat")) {
			if (getRefDecl(iVar) == var)
				lstat = true;
			continue;
		}

		if (after && need_fileno && !fstat
		&& isCallOfFunctionNamed(iFn, "fileno")) {
			if (getRefDecl(iVar) == fd) {
				fd = getVarAssignedTo(iFn, NULL);
				if (!fd) break;
				need_fileno = false;
			}
			continue;
		}

		if (after && isCallOfFunctionNamed(iFn, "fstat")) {
			if (getRefDecl(iVar) == fd)
				fstat = true;
			break;
		}
	}

	if (lstat && (!fstat || need_fileno)) {
//		if (isCallOfFunctionNamed(node, "fopen"))
//			print_error(node, "FIO32-C", "Do not perform operations on devices that are only appropriate for files");
//		else
			print_error(node, "POS35-C", "Avoid race conditions while checking for the existence of a symbolic link");
		return true;
	}

	return true;
}

/**
 * Observe correct revocation order while relinquishing privileges
 *
 * \note Since there's really no clean way to do this, we'll just traverse up
 * to the first SgBasicBlock above a setgid(), then find the previous
 * statement, and see if it contains a call to setuid()
 */
bool POS36_C( const SgNode *node ) {
	const SgFunctionRefExp *fnRef = isSgFunctionRefExp(node);
	if (!(fnRef && isCallOfFunctionNamed(fnRef, "setgid")))
		return false;

	const SgStatement *prevStat = findInBlockByOffset(fnRef, -1);
	if (!prevStat)
		return false;

	FOREACH_SUBNODE(prevStat, nodes, i, V_SgFunctionRefExp) {
		if (isCallOfFunctionNamed(isSgFunctionRefExp(*i), "setuid")) {
			print_error(node, "POS36-C", "Observe correct revocation order while relinquishing privileges");
			return true;
		}
	}
	return false;
}

bool POS_C(const SgNode *node) {
  bool violation = false;
  violation |= POS30_C(node);
  violation |= POS33_C(node);
  violation |= POS34_C(node);
  violation |= POS35_C(node);
  violation |= POS36_C(node);
  return violation;
}
