cmake_minimum_required(VERSION 2.8.7)

find_package (BISON REQUIRED)
find_package(FLEX REQUIRED)

#set(CMAKE_POSITION_INDEPENDENT_CODE ON) # requires version 2.8.9

BISON_TARGET(MyParser input/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
FLEX_TARGET(MyScanner input/lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

set(SOURCES api/metric.cpp api/output/synthesis.cpp api/output/fence_synth.cpp api/output/bugs.cpp api/output/smt.cpp api/trace_analysis.cpp prune/prune_functions.cpp api/output/nf.cpp api/output/output_base.cpp prune/diffvar.cpp prune/remove_implied.cpp prune/remove_thin.cpp prune/remove_non_cycled.cpp prune/remove_non_relax_rf.cpp constants.cpp prune/data_flow.cpp prune/unsat_core.cpp prune/prune_base.cpp hb_enc/integer.cpp hb_enc/encoding.cpp hb_enc/symbolic_event.cpp hb_enc/hb.cpp hb_enc/cycles.cpp hb_enc/depends.cpp api/options.cpp input/ctrc_input.cpp input/ctrc_program.cpp cssa/wmm.cpp cssa/c11.cpp cssa/rmo.cpp cssa/arm8-2.cpp cssa/power.cpp program/program.cpp program/orderings.cpp input/program.cpp input/parse.cpp cinput/cinput.cpp cinput/llvm_utils.cpp cinput/build_program.cpp helpers/z3interf.cpp helpers/maxsat.cpp input/instruction.cpp ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS})

#add_library (taraobj OBJECT ${SOURCES})
#add_library (tarastatic STATIC $<TARGET_OBJECTS:taraobj>)
#add_library (tara SHARED $<TARGET_OBJECTS:taraobj>)

add_library (tarastatic STATIC ${SOURCES})

#target_link_libraries (shared z3 ${Boost_LIBRARIES} bdd rt)
