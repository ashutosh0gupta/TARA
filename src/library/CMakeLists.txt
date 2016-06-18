cmake_minimum_required(VERSION 2.8.7)

find_package (BISON REQUIRED)
find_package(FLEX REQUIRED)

#set(CMAKE_POSITION_INDEPENDENT_CODE ON) # requires version 2.8.9

BISON_TARGET(MyParser input/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
FLEX_TARGET(MyScanner input/lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

set(SOURCES api/metric.cpp api/output/synthesis.cpp api/output/barrier_synthesis.cpp api/output/bugs.cpp api/output/smt.cpp api/trace_analysis.cpp prune/prune_functions.cpp api/output/nf.cpp api/output/output_base.cpp prune/diffvar.cpp prune/remove_implied.cpp prune/remove_thin.cpp constants.cpp prune/data_flow.cpp prune/unsat_core.cpp prune/prune_base.cpp hb_enc/integer.cpp hb_enc/encoding.cpp api/options.cpp cssa/thread.cpp cssa/program.cpp cssa/wmm.cpp cssa/program_functions.cpp input/program.cpp input/parse.cpp cinput/cinput.cpp helpers/z3interf.cpp helpers/int_to_string.cpp input/instruction.cpp ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS})

#add_library (taraobj OBJECT ${SOURCES})
#add_library (tarastatic STATIC $<TARGET_OBJECTS:taraobj>)
#add_library (tara SHARED $<TARGET_OBJECTS:taraobj>)

add_library (tarastatic STATIC ${SOURCES})

#target_link_libraries (shared z3 ${Boost_LIBRARIES} bdd rt)
