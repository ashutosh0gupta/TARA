/*
 * Copyright 2017, TIFR
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

#include "cinput.h"
#include <iostream>
#include "build_program.h"
#include "helpers/helpers.h"
#include <utility>
#include <algorithm>
#include "helpers/helpers.h"
#include <vector>
#include <boost/filesystem.hpp>

using namespace tara::cssa;
using namespace std;
using namespace tara;
using namespace tara::cinput;
using namespace tara::helpers;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// pragam'ed to aviod warnings due to llvm included files

#include "llvm/IRReader/IRReader.h"

#include "llvm/IR/InstIterator.h"

#include "llvm/LinkAllPasses.h"
#include "llvm/Pass.h"
// #include "llvm/PassManager.h" // 3.6
#include "llvm/IR/PassManager.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRPrintingPasses.h"
// #include "llvm/IR/InstrTypes.h"
// #include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"


#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Dwarf.h"

#include "llvm/Analysis/CFGPrinter.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/ADT/StringMap.h"

#pragma GCC diagnostic pop

#define CLANG_VERSION "4.0"

//todo: directly conver c to object file without creating bc file
void c2bc( const std::string& filename, const std::string& outname ) {
  // make a system call
  std::ostringstream cmd;
  cmd << "clang-" << CLANG_VERSION <<" -emit-llvm -O0 -g -std=c++11 "
      << filename << " -o " << outname << " -c";
  if( system( cmd.str().c_str() ) != 0 ) exit(1);
}


//todo: add unique_ptr support
tara::program* tara::cinput::parse_cpp_file( helpers::z3interf& z3_,
                                             api::options& o,
                                             hb_enc::encoding& hb_encoding,
                                             std::string& cfile ) {
  boost::filesystem::path cf( cfile );
  boost::filesystem::path bc_file( o.output_dir );
  cf += ".bc";
  bc_file += cf.filename();
  if( o.print_input > 0 ) {
    std::cerr << "dumping bytecode file : " << bc_file << std::endl;
  }

  c2bc( cfile, bc_file.string() );

  if( o.print_input > 2 ) {
    setLLVMConfigViaCommandLineOptions( "-debug-pass=Structure" );
  }

  std::unique_ptr<llvm::Module> module;
  llvm::SMDiagnostic err;
  //llvm::PassManager passMan; // 3.6
  llvm::legacy::PassManager passMan;
  llvm::PassRegistry& reg = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeAnalysis(reg);


  //todo: get rid of clang call
  // why are we parsing IR file.. why not directly .cpp??

  // llvm::LLVMContext& context = llvm::getGlobalContext(); //4.0 moved it to only c-api
  static llvm::LLVMContext context;
  module = llvm::parseIRFile( bc_file.string(), err, context);
  if( module.get() == 0 ) {
    cinput_error( "failed to parse the input file!");
  }
  program* p = new program( z3_, o, hb_encoding );

  hb_enc::se_set prev_events;
  hb_enc::source_loc sloc("the_launcher"),floc("the_finisher");
  std::vector<z3::expr> history;
  z3::expr tru = z3_.mk_true();
  auto start = mk_se_ptr( hb_encoding, INT_MAX, prev_events, tru, history, sloc,
                          hb_enc::event_t::pre, hb_enc::o_tag_t::sc);
  auto final = mk_se_ptr( hb_encoding, INT_MAX, prev_events, tru, history, floc,
                          hb_enc::event_t::post, hb_enc::o_tag_t::sc);

  p->init_loc = start;
  p->post_loc = final;

  // setting up events for global variable initializations
  for( auto iter_glb= module->global_begin(),end_glb = module->global_end();
    iter_glb != end_glb; ++iter_glb ) {
    llvm::GlobalVariable* glb = &*iter_glb; //3.8
    const std::string gvar = (std::string)(glb->getName());
    llvm::Type* ty = glb->getType();
    if( o.print_input  > 0 ) {
      llvm::outs() << "Global : " ;
      glb->print( llvm::outs() ); llvm::outs() << "\n";
      //ty->print( llvm::outs() ); llvm::outs() << " "
    }
    if( auto pty = llvm::dyn_cast<llvm::PointerType>(ty) ) {
      z3::sort sort = llvm_to_z3_sort( z3_.c, pty->getElementType() );
      p->add_global( gvar, sort );
      p->wr_events[ p->get_global( gvar ) ].insert( start );
      if( glb->hasUniqueInitializer() ) {
        auto c = glb->getInitializer();
        build_program::ValueExprMap dummy_vmap;
        auto val = build_program::get_term( z3_, c, dummy_vmap );
        p->append_pre( p->get_global( gvar ), val );
      }
    }else
      cinput_error( (std::string)(glb->getName()) << " not a global pointer!");
  }
#ifndef NDEBUG
  if( o.print_input > 3 ) { // verbosity
    llvm::DebugFlag = true;
  }
#endif

  // Work around due to a bug in LLVM 4.0====================
  // setting unroll count via commmand line parsing
  std::string ustr = "-unroll-count=" + std::to_string(o.loop_unroll_count);
  setLLVMConfigViaCommandLineOptions( ustr );
  //================================================

  passMan.add( llvm::createPromoteMemoryToRegisterPass() );
  // passMan.add( llvm::createCFGSimplificationPass() ); // some params
  passMan.add( llvm::createLoopRotatePass() ); // some params
  passMan.add( llvm::createLoopUnrollPass( 1000, o.loop_unroll_count ) );
  passMan.add( new SplitAtAssumePass() );
  // passMan.add( llvm::createCFGPrinterPass() );
  auto build_p = new build_program( z3_, o, hb_encoding, p );
  passMan.add( build_p );
  passMan.run( *module.get() );

  // for( const auto& g : p->globals ) {
  //   p->rd_events[g].push_back( final );
  // }

  if( o.print_input > 0 ) {
    dump_dot_module( o.output_dir, module );
    p->print_dot( "" );
  }

  // delete build_p; // todo: deletion of pass object build_p

  return p;
}
