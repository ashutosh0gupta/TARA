/*
 * MaxSAT.cpp
 *
 *  Created on: Apr 1, 2016
 *      Author: admin-hp
 */
#include <z3++.h>
#include <iostream>
#include <string>
void assert_hard_constraints(z3::solver &s, unsigned num_cnstrs, z3::expr_vector cnstrs)
{
	//z3::solver s(ctx);
    unsigned i;
    for (i = 0; i < num_cnstrs; i++) {
    	s.add(cnstrs[i]);
        //Z3_assert_cnstr(ctx, cnstrs[i]);
    }
}

z3::expr_vector mk_fresh_bool_var_array(z3::context &ctx, unsigned num_vars)
{
    z3::expr_vector result(ctx);
    unsigned i;
    for (i = 0; i < num_vars; i++) {
    	std::stringstream x_name;
    	x_name << "x" << i;
    	z3::expr x=ctx.bool_const(x_name.str().c_str());
        result.push_back(x);
    }
    return result;
}

z3::expr_vector assert_soft_constraints(z3::solver&s ,z3::context& ctx, unsigned num_cnstrs, z3::expr_vector cnstrs)
{
    unsigned i;
    z3::expr_vector aux_vars(ctx);
    aux_vars = mk_fresh_bool_var_array(ctx, num_cnstrs);
    for (i = 0; i < num_cnstrs; i++) {
        z3::expr assumption = cnstrs[i];
        //Z3_assert_cnstr(ctx, mk_binary_or(ctx, assumption, aux_vars[i]));
        s.add(assumption||aux_vars[i]);
    }
    return aux_vars;
}



/**
  \brief Implement one step of the Fu&Malik algorithm.
  See fu_malik_maxsat function for more details.

  Input:    soft constraints + aux-vars (aka answer literals)
  Output:   done/not-done  when not done return updated set of soft-constraints and aux-vars.
  - if SAT --> terminates
  - if UNSAT
     * compute unsat core
     * add blocking variable to soft-constraints in the core
         - replace soft-constraint with the one with the blocking variable
         - we should also add an aux-var
         - replace aux-var with a new one
     * add at-most-one constraint with blocking
*/
int fu_malik_maxsat_step(z3::solver &s,z3::context &ctx, unsigned num_soft_cnstrs, z3::expr_vector soft_cnstrs, z3::expr_vector aux_vars)
{
    // create assumptions
    z3::expr_vector assumptions(ctx);
    z3::expr_vector core(ctx);
    //unsigned core_size;
    //z3::expr dummy_proof = ctx.bool_const("dummy_proof"); // we don't care about proofs in this example
    z3::model m=s.get_model();
    z3::expr is_sat=ctx.bool_val(false);
    unsigned i = 0;
    for (i = 0; i < num_soft_cnstrs; i++) {
        // Recall that we asserted (soft_cnstrs[i] \/ aux_vars[i])
        // So using (NOT aux_vars[i]) as an assumption we are actually forcing the soft_cnstrs[i] to be considered.
        assumptions.push_back(!aux_vars[i]);	//[i] = Z3_mk_not(ctx, aux_vars[i]);
        //core[i]=false;
    }
    //core.clear();

    //is_sat = s.check(assumptions);//Z3_check_assumptions(ctx, num_soft_cnstrs, assumptions, &m, &dummy_proof, &core_size, core);
    core=s.unsat_core();
    if (s.check(assumptions) != z3::check_result::unsat) {
        return 1; // done
    }
    else {
        z3::expr_vector block_vars(ctx);	// = (Z3_ast*) malloc(sizeof(Z3_ast) * core_size);
        unsigned k = 0;
        // update soft-constraints and aux_vars
        for (i = 0; i < num_soft_cnstrs; i++) {
            unsigned j;
            // check whether assumption[i] is in the core or not
            for (j = 0; j < core.size(); j++) {
                if (assumptions[i] == core[j])
                    break;
            }
            if (j < core.size()) {
                // assumption[i] is in the unsat core... so soft_cnstrs[i] is in the unsat core
                z3::expr block_var =ctx.bool_const("block_var");//= mk_fresh_bool_var(ctx);
                z3::expr new_aux_var = ctx.bool_const("new_aux_var");//mk_fresh_bool_var(ctx);
                soft_cnstrs[i]     = (soft_cnstrs[i] || block_var);//mk_binary_or(ctx, soft_cnstrs[i], block_var);
                aux_vars[i]        = new_aux_var;
                block_vars[k]      = block_var;
                k++;
                // Add new constraint containing the block variable.
                // Note that we are using the new auxiliary variable to be able to use it as an assumption.
                s.add(soft_cnstrs[i]||new_aux_var);//Z3_assert_cnstr(ctx, mk_binary_or(ctx, soft_cnstrs[i], new_aux_var));
            }
        }
        //assert_at_most_one(ctx, k, block_vars);
        return 0; // not done.
    }
}

/**
   \brief Fu & Malik procedure for MaxSAT. This procedure is based on
   unsat core extraction and the at-most-one constraint.

   Return the number of soft-constraints that can be
   satisfied.  Return -1 if the hard-constraints cannot be
   satisfied. That is, the formula cannot be satisfied even if all
   soft-constraints are ignored.

   For more information on the Fu & Malik procedure:

   Z. Fu and S. Malik, On solving the partial MAX-SAT problem, in International
   Conference on Theory and Applications of Satisfiability Testing, 2006.
*/
int fu_malik_maxsat(z3::context& ctx, unsigned num_hard_cnstrs, z3::expr_vector hard_cnstrs, unsigned num_soft_cnstrs, z3::expr_vector soft_cnstrs)
{
    z3::expr_vector aux_vars(ctx);
    //z3::expr is_sat=ctx.bool_const("is_sat");
    unsigned k;
    z3::solver s(ctx);
    assert_hard_constraints(s, num_hard_cnstrs, hard_cnstrs);
    printf("checking whether hard constraints are satisfiable...\n");
    //is_sat = s.check();
    if (s.check() ==z3::check_result::sat) {
        // It is not possible to make the formula satisfiable even when ignoring all soft constraints.
        return -1;
    }
    if (num_soft_cnstrs == 0)
        return 0; // nothing to be done...
    /*
      Fu&Malik algorithm is based on UNSAT-core extraction.
      We accomplish that using auxiliary variables (aka answer literals).
    */
    aux_vars = assert_soft_constraints(s,ctx, num_soft_cnstrs, soft_cnstrs);
    k = 0;
    for (;;) {
        printf("iteration %d\n", k);
        if (fu_malik_maxsat_step(s,ctx, num_soft_cnstrs, soft_cnstrs, aux_vars)) {
            return num_soft_cnstrs - k;
        }
        k++;
    }
}
