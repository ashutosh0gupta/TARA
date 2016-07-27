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

#include "cinput.h"
#include <iostream>
#include "build_program.h"
#include "helpers/helpers.h"
#include <utility>
#include <algorithm>
#include "cssa/cssa_exception.h"
#include "helpers/helpers.h"
#include <vector>
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
#include "llvm/PassManager.h"
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
// #include "llvm/Support/Debug.h"
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

void c2bc( const std::string& filename, const std::string& outname ) {
  // make a system call
  std::ostringstream cmd;
  cmd << "clang-3.6 -emit-llvm -O0 -g " << filename << " -o " << outname << " -c";
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
  // std::string bc_file = cfile+".bc";
  c2bc( cfile, bc_file.string() );

  std::unique_ptr<llvm::Module> module;
  llvm::SMDiagnostic err;
  llvm::LLVMContext& context = llvm::getGlobalContext();
  llvm::PassManager passMan;
  llvm::PassRegistry& reg = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeAnalysis(reg);


  //todo: get rid of clang call
  // why are we parsing IR file.. why not directly .cpp??
  module = llvm::parseIRFile( bc_file.string(), err, context);
  if( module.get() == 0 ) {
    // give err msg
  }
  program* p = new program( z3_, o, hb_encoding );

  hb_enc::se_set prev_events;
  auto start = mk_se_ptr( hb_encoding, INT_MAX, prev_events,
                          "the_launcher", hb_enc::event_t::pre );
  auto final = mk_se_ptr( hb_encoding, INT_MAX, prev_events,
                          "the_finisher", hb_enc::event_t::post );

  // add_event( INT_MAX, start );
  p->init_loc.insert( start );
  p->post_loc.insert( final );

  // llvm::PointerType* iptr = llvm::Type::getInt32PtrTy( context );

  for( auto iter_glb= module->global_begin(),end_glb = module->global_end();
    iter_glb != end_glb; ++iter_glb ) {
    llvm::GlobalVariable* glb = iter_glb;
    const std::string gvar = (std::string)(glb->getName());
    llvm::Type* ty = glb->getType();
    if( o.print_input  > 0 ) {
      llvm::outs() << "Global : " ;
      glb->print( llvm::outs() );
      llvm::outs() << "\n";
      //ty->print( llvm::outs() ); llvm::outs() << " "
    }
    if( auto pty = llvm::dyn_cast<llvm::PointerType>(ty) ) {
      z3::sort sort = llvm_to_z3_sort( z3_.c, pty->getElementType() );
      p->add_global( gvar, sort );
      p->wr_events[ p->get_global( gvar ) ].insert( start );
    }else
      cinput_error( (std::string)(glb->getName()) << " not a global pointer!");
  }

  passMan.add( llvm::createPromoteMemoryToRegisterPass() );
  passMan.add( new SplitAtAssumePass() );
  // passMan.add( llvm::createCFGPrinterPass() );
  passMan.add( new build_program( z3_, o, hb_encoding, p ) );
  passMan.run( *module.get() );

  for( const auto& g : p->globals ) {
    p->rd_events[g].push_back( final );
  }

  if( o.print_input > 0 ) {
    p->print_dot( "" );
  }

  return p;
}
