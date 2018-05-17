/*++
Copyright (c) 2015 Microsoft Corporation

Module Name:

    special_relations_decl_plugin.h

Abstract:

    <abstract>

Author:

    Nikolaj Bjorner (nbjorner) 2015-15-9.

Revision History:

--*/
#ifndef SPECIAL_RELATIONS_DECL_PLUGIN_H_
#define SPECIAL_RELATIONS_DECL_PLUGIN_H_

#include "ast/ast.h"



enum special_relations_op_kind {
    OP_SPECIAL_RELATION_LO,
    OP_SPECIAL_RELATION_PO,
    OP_SPECIAL_RELATION_PO_AO,
    OP_SPECIAL_RELATION_PLO,
    OP_SPECIAL_RELATION_TO,
    LAST_SPECIAL_RELATIONS_OP
};

class special_relations_decl_plugin : public decl_plugin {
    symbol m_lo;
    symbol m_po;
    symbol m_po_ao;
    symbol m_plo;
    symbol m_to;
public:
    special_relations_decl_plugin();
    virtual ~special_relations_decl_plugin() {}

    virtual decl_plugin * mk_fresh() {
        return alloc(special_relations_decl_plugin);
    }
    
    virtual func_decl * mk_func_decl(decl_kind k, unsigned num_parameters, parameter const * parameters, 
                                     unsigned arity, sort * const * domain, sort * range);

    virtual void get_op_names(svector<builtin_name> & op_names, symbol const & logic);


    virtual sort * mk_sort(decl_kind k, unsigned num_parameters, parameter const * parameters) { return 0; }
};

enum sr_property {
    sr_transitive    = 0x01,                              // Rxy & Ryz -> Rxz
    sr_reflexive     = 0x02,                              // Rxx
    sr_antisymmetric = 0x04,                              // Rxy & Ryx -> x = y
    sr_lefttree      = 0x08,                              // Ryx & Rzx -> Ryz | Rzy
    sr_righttree     = 0x10,                              // Rxy & Rxz -> Ryx | Rzy
    sr_po            = 0x01 | 0x02 | 0x04,                // partial order
    sr_po_ao         = 0x01 | 0x02 | 0x04 | 0x30,         // partial order, ao //ASHU:
    sr_lo            = 0x01 | 0x02 | 0x04 | 0x08 | 0x10,  // linear order
    sr_plo           = 0x01 | 0x02 | 0x04 | 0x20,         // piecewise linear order
    sr_to            = 0x01 | 0x02 | 0x04 | 0x10,         // right-tree
};

class special_relations_util {
    ast_manager& m;
    family_id    m_fid;
public:
    special_relations_util(ast_manager& m) : m(m), m_fid(m.get_family_id("special_relations")) {}
    
    bool is_special_relation(func_decl* f) const { return f->get_family_id() == m_fid; }
    bool is_special_relation(app* e) const { return is_special_relation(e->get_decl()); }
    sr_property get_property(func_decl* f) const;
    sr_property get_property(app* e) const { return get_property(e->get_decl()); }
  //Start: code added by ASHU

  bool is_lo(expr const * e) const { return is_app_of(e, m_fid, OP_SPECIAL_RELATION_LO); }
  bool is_po(expr const * e) const { return is_app_of(e, m_fid, OP_SPECIAL_RELATION_PO); }
  bool is_po_ao(expr const * e) const { return is_app_of(e, m_fid, OP_SPECIAL_RELATION_PO_AO); }
  bool is_plo(expr const * e) const { return is_app_of(e, m_fid, OP_SPECIAL_RELATION_PLO); }
  bool is_to(expr const * e) const { return is_app_of(e, m_fid, OP_SPECIAL_RELATION_TO); }

    app * mk_lo (expr * arg1, expr * arg2) { return m.mk_app( m_fid, OP_SPECIAL_RELATION_LO,  arg1, arg2); }
    app * mk_po (expr * arg1, expr * arg2) { return m.mk_app( m_fid, OP_SPECIAL_RELATION_PO,  arg1, arg2); }
    app * mk_po_ao (expr * arg1, expr * arg2) { return m.mk_app( m_fid, OP_SPECIAL_RELATION_PO_AO,  arg1, arg2); }
    app * mk_plo(expr * arg1, expr * arg2) { return m.mk_app( m_fid, OP_SPECIAL_RELATION_PLO, arg1, arg2); }
    app * mk_to (expr * arg1, expr * arg2) { return m.mk_app( m_fid, OP_SPECIAL_RELATION_TO,  arg1, arg2); }
  //End:  code added by ASHU

};


#endif /* SPECIAL_RELATIONS_DECL_PLUGIN_H_ */

