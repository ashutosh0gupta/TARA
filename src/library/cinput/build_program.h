/*
 * Copyright 2014, IST Austria
 *
 * This file is part of TARA.
 *
 * TARA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TARA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with TARA.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TARA_CINPUT_BUILD_PROGRAM_H
#define TARA_CINPUT_BUILD_PROGRAM_H

#include <map>
#include "helpers/z3interf.h"
#include "api/options.h"
#include "helpers/helpers.h"
#include "program/program.h"
#include "cinput/cinput.h"
#include <utility>
#include "hb_enc/symbolic_event.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/Pass.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#pragma GCC diagnostic pop

typedef llvm::BasicBlock bb;
typedef std::set<const bb*> bb_set_t;
typedef std::vector<const bb*> bb_vec_t;

namespace tara {
namespace cinput {

  //----------------------------------
  // todo: should be moved to its own file
  int readInt( const llvm::ConstantInt* );
  bool is_llvm_false( llvm::Value* );
  bool is_llvm_true( llvm::Value* );
  z3::sort llvm_to_z3_sort( z3::context& c, llvm::Type* t );
  std::string getInstructionLocationName(const llvm::Instruction* I );
  hb_enc::source_loc getInstructionLocation(const llvm::Instruction* I );
  hb_enc::source_loc getBlockLocation(const bb* b );
  void initBlockCount( llvm::Function &f,
                       std::map<const bb*, bb_set_t>& back_edges,
                       std::vector<const bb*>& bs,
                       std::map<const bb*, unsigned>& block_to_id);
  bool is_dangling( const bb* b, std::map<const bb*, bb_set_t>& back_edges );

  void removeBranchingOnPHINode( llvm::BranchInst *branch );
  void remove_unreachable_blocks( llvm::BasicBlock* b );
  void setLLVMConfigViaCommandLineOptions( std::string strs );
  void dump_dot_module( boost::filesystem::path&,
                        std::unique_ptr<llvm::Module>& );


 //  void ordered_blocks( const llvm::Function &F,
 // );

  //-------------------------------------------------------

  class split_step {
  public:
    split_step( const bb* b_,
                unsigned block_id_,
                unsigned succ_num_,
                z3::expr e_):
      b(b_), block_id(block_id_), succ_num(succ_num_), cond(e_) {}
    const bb* b;
    unsigned block_id;
    unsigned succ_num;
    z3::expr cond;
    void print(std::ostream& os) const;
    // friend bool operator==(const split_step& step1, const split_step& step2);
  };

  typedef std::vector<split_step> split_history;
  void print( std::ostream& os, const split_history&  hs );

  class SplitAtAssumePass : public llvm::FunctionPass {
  public:
    static char ID;
    SplitAtAssumePass() : llvm::FunctionPass(ID) {}
    virtual bool runOnFunction( llvm::Function &f );
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;
    // const char * getPassName() const
#ifndef NDEBUG // llvm 4.0.0 vs 3.8 issue
 llvm::StringRef
#else
 const char *
#endif
 getPassName() const;
    // llvm::StringRef
  };

  // conditional pointing; conditions may overlap due to non-determinism
  typedef std::vector< std::pair< z3::expr, tara::variable > > points_set_t;

  class build_program : public llvm::FunctionPass {

  public:
    typedef std::map< const llvm::Value*, z3::expr > ValueExprMap;
    ValueExprMap m;
    std::map< const llvm::Value*, tara::variable > localVars;
    typedef std::map< const llvm::Value*, hb_enc::depends_set > local_data_dependency;
    typedef std::map< const bb*, hb_enc::depends_set > local_ctrl_dependency;
    static char ID;
    std::string name;

  private:
    helpers::z3interf& z3;
    api::options& o;
    hb_enc::encoding& hb_enc;
    // Cfrontend::Config& config;
    tara::program* p;
    unsigned inst_counter = 0;
    ValueExprMap valMap;
    std::map< const bb*, unsigned> block_to_id;
    std::map< const bb*, split_history > block_to_split_stack;
    std::map< const bb*, z3::expr > block_to_exit_bit;
    std::map< const bb*, hb_enc::se_set> block_to_trailing_events;
    std::map< const bb*, bb_set_t > loop_ignore_edges;
    std::map< const bb*, bb_set_t > rev_loop_ignore_edges;
    //-----------------------------------
    // local data structure of dependency
    //hb_enc::depends_set data_dep_ses;
    //hb_enc::depends_set ctrl_dep_ses;
    build_program::local_data_dependency local_map;
    build_program::local_ctrl_dependency local_ctrl;
    std::map< const bb*, std::map< tara::variable, hb_enc::depends_set > >
    local_release_heads;
    //-----------------------------------

    class pointing {
    public:
      pointing( points_set_t p_set_, z3::expr null_cond_ )
        : p_set(p_set_) { null_cond.insert( null_cond_ ); }
      pointing( points_set_t p_set_ ) : p_set(p_set_) {}
      points_set_t p_set;
      std::set<z3::expr> null_cond;
      bool has_null() { return !null_cond.empty(); }
      z3::expr get_null_cond() {
        assert( null_cond.size() == 1 );
        return *null_cond.begin();
      }
    };
    std::map< const llvm::Value*, pointing > points_to;

    //thread creation map
    std::map< const llvm::Value*, std::string > ptr_to_create;

    //
    std::map< bb*, z3::expr > block_to_path_con;
    z3::expr phi_instr = z3.mk_true();
    z3::expr phi_cond = z3.mk_true();

    void join_histories( const std::vector<const bb*>& preds,
                         const std::vector<split_history>& hs,
                         split_history& h,
                         z3::expr& path_cond,
                         std::map<const bb*,z3::expr>& conds
                         );

   hb_enc::depends_set get_depends( const llvm::Value* op );
    void join_depends( const llvm::Value* op1,const llvm::Value* op2,
                       hb_enc::depends_set& result );

   hb_enc::depends_set get_ctrl( const bb* b);
   z3::expr getPhiMap ( const llvm::Value* op, ValueExprMap& m );
  public:
    build_program( helpers::z3interf& z3_,
                   api::options& o_,
                   hb_enc::encoding& hb_enc_,
                   tara::program* program_ )
    : llvm::FunctionPass(ID)
    , z3(z3_)
    , o(o_)
    , hb_enc( hb_enc_ )
    , p( program_ )
{}

    virtual bool runOnFunction( llvm::Function & );

    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;

    // const char * getPassName() const;
    // llvm::StringRef

#ifndef NDEBUG // llvm 4.0.0 vs 3.8 issue
 llvm::StringRef
#else
 const char *
#endif
    getPassName() const;

  private:

    z3::expr translateBlock( unsigned thr_id,
                             const bb*,
                             hb_enc::se_set& prev_events,
                             z3::expr& path_cond,
                             std::vector< z3::expr >& history,
                             std::map<const bb*,z3::expr>& conds,
                             std::map<const hb_enc::se_ptr, z3::expr>& bconds );

    z3::expr getTerm( const llvm::Value* op ,ValueExprMap& m ) {
      return get_term( z3, op, m );
    }

    static void insert_term_map( const llvm::Value* op ,
                                 z3::expr e, ValueExprMap& m ) {
      auto pair = std::make_pair( op, e );
      m.insert( pair );
      // m.at(op) = e; //todo: implement this
    }

    bool isLocalVar( const llvm::Value* g  ) {
      auto it = localVars.find( g );
      if( it == localVars.end() ) return false;
      return true;
    }

    void collect_loop_backedges();
  public:
    static
    z3::expr get_term(helpers::z3interf&, const llvm::Value*,ValueExprMap&);

  };

}}
#endif // TARA_CINPUT_BUILD_PROGRAM_H
