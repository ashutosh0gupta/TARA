#include<iostream>
#include"z3.h"
#include"api_log_macros.h"
#include"api_context.h"
#include"api_util.h"
#include"ast_pp.h"

extern "C" {

    Z3_bool Z3_API Z3_is_sr_lo(Z3_context c, Z3_ast s) {
        Z3_TRY;
        LOG_Z3_is_string(c, s);
        RESET_ERROR_CODE();
        bool result = mk_c(c)->sr_util().is_lo( to_expr(s) );
        return result?Z3_TRUE:Z3_FALSE;
        Z3_CATCH_RETURN(Z3_FALSE);
    }

    Z3_bool Z3_API Z3_is_sr_po(Z3_context c, Z3_ast s) {
        Z3_TRY;
        LOG_Z3_is_string(c, s);
        RESET_ERROR_CODE();
        bool result = mk_c(c)->sr_util().is_po( to_expr(s) );
        return result?Z3_TRUE:Z3_FALSE;
        Z3_CATCH_RETURN(Z3_FALSE);
    }

    Z3_bool Z3_API Z3_is_sr_plo(Z3_context c, Z3_ast s) {
        Z3_TRY;
        LOG_Z3_is_string(c, s);
        RESET_ERROR_CODE();
        bool result = mk_c(c)->sr_util().is_plo( to_expr(s) );
        return result?Z3_TRUE:Z3_FALSE;
        Z3_CATCH_RETURN(Z3_FALSE);
    }

    Z3_bool Z3_API Z3_is_sr_to(Z3_context c, Z3_ast s) {
        Z3_TRY;
        LOG_Z3_is_string(c, s);
        RESET_ERROR_CODE();
        bool result = mk_c(c)->sr_util().is_to( to_expr(s) );
        return result?Z3_TRUE:Z3_FALSE;
        Z3_CATCH_RETURN(Z3_FALSE);
    }

    MK_BINARY(Z3_mk_sr_lo , mk_c(c)->get_special_relations_fid(), OP_SPECIAL_RELATION_LO, SKIP);
    MK_BINARY(Z3_mk_sr_po , mk_c(c)->get_special_relations_fid(), OP_SPECIAL_RELATION_PO, SKIP);
    MK_BINARY(Z3_mk_sr_plo, mk_c(c)->get_special_relations_fid(), OP_SPECIAL_RELATION_PLO, SKIP);
    MK_BINARY(Z3_mk_sr_to , mk_c(c)->get_special_relations_fid(), OP_SPECIAL_RELATION_TO, SKIP);

};
