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

#include "program.h"
#include "cssa_exception.h"
#include "helpers/helpers.h"
#include "int_to_string.h"
#include <string.h>
#include "helpers/z3interf.h"
using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;

using namespace std;

z3:: expr program::build_rf_val(std::shared_ptr<cssa::instruction> l1, std::shared_ptr<cssa::instruction> l2,z3::context & C, std::string str,bool pre)
{


    z3::expr exit=C.bool_val(false);
    z3::expr s = C.bool_const("s");

    for(variable v2: l2->variables_read)
    {

      if(check_correct_global_variable(v2,str))
      {

         if(pre==0)
         {
           for(variable v1: l1->variables_write)
            {
              z3::expr conjecture= implies (s,(z3::expr)v2==(z3::expr)v1);
              return conjecture;
            }
         }
         else
          {

            for(variable v1: new_globals)
            {

                if(check_correct_global_variable(v1,str))
                {
                    z3::expr conjecture= implies (s,(z3::expr)v2==(z3::expr)v1);
                    return conjecture;
                }
            }

          }

        }

    }

    //std::cout<<"\ninstr_to_var\t"<<instr_to_var[l2]->name<<"\n";
    //std::cout<<"\nvariable_written\t"<<variable_written[str]->instr<<"\n";

    return exit;
}



z3::expr program::build_rf_grf(std::shared_ptr<cssa::instruction> l1, std::shared_ptr<cssa::instruction> l2,z3::context & C,std::string str,bool pre, const input::program& input)
{
  shared_ptr<hb_enc::location> start_location = input.start_name();
  z3::expr s=build_rf_val(l1,l2,C,str,pre);
  z3::expr c=C.bool_const("c");

  if(pre==0)
  {
    c=_hb_encoding.make_hb(l1->loc,l2->loc);
  }

  else
  {

    c=_hb_encoding.make_hb(start_location,l2->loc);
  }


  z3::expr conjecture=implies(s,c);
  return conjecture;
}

z3::expr program::build_rf_some(std::shared_ptr<cssa::instruction> l1, std::shared_ptr<cssa::instruction> l2, std::shared_ptr<cssa::instruction> l3,z3::context & C,std::string str,bool pre)
{
  z3::expr s1=C.bool_const("s1");
  z3::expr s2=C.bool_const("s2");

  z3::expr conjecture2=build_rf_val(l3,l1,C,str,false);
  z3::expr conjecture1=C.bool_val(true);
  if(pre==1)
  {
      z3::expr conjecture1=build_rf_val(l2,l1,C,str,true);
  }
  else
  {
    z3::expr conjecture1=build_rf_val(l2,l1,C,str,false);
  }

  z3::expr conjecture=(conjecture1 || conjecture2);
  return conjecture;
}





z3::expr program::build_ws(std::shared_ptr<cssa::instruction> l1, std::shared_ptr<cssa::instruction> l2, z3::context & C,bool pre,const input::program& input)
{
  shared_ptr<hb_enc::location> start_location = input.start_name();
  z3::expr c1=C.bool_const("c1");
  z3::expr c2=C.bool_const("c2");
  //std::cout<<"\nhere 1 \n";
  if(pre==0)
  {

      c1=_hb_encoding.make_hb(l1->loc,l2->loc);
      c2=_hb_encoding.make_hb(l2->loc,l1->loc);
  }
  else
  {
      //std::cout<<"\nhere 3 \n";
      c1=_hb_encoding.make_hb(start_location,l2->loc);
    //std::cout<<"\nhere 4 \n";
      c2=_hb_encoding.make_hb(l2->loc,start_location);
      //std::cout<<"\nhere 5 \n";
  }

  z3::expr conjecture=implies(!c1,c2);
  //std::cout<<"\nhere 6 \n";
  return conjecture;
}




z3::expr program::build_fr(std::shared_ptr<cssa::instruction> l1, std::shared_ptr<cssa::instruction> l2, std::shared_ptr<cssa::instruction> l3, z3::context & C,std::string str,bool pre,bool pre_where,const input::program& input)
{
  shared_ptr<hb_enc::location> start_location = input.start_name();
  z3::expr c1=C.bool_const("c1");
  z3::expr c2=C.bool_const("c2");
  z3::expr s=C.bool_const("s");;
  if(pre_where==true)
  {
       s=build_rf_val(l1,l2,C,str,!pre);
  }
  else
  {
     s=build_rf_val(l1,l2,C,str,pre);
  }

  if(pre==0)                                  //pre tells whether there is an initialisation instruction
  {
      c1=_hb_encoding.make_hb(l1->loc,l3->loc);
      c2=_hb_encoding.make_hb(l2->loc,l3->loc);
  }
  else
  {
    if(pre_where==0)                          // pre_where tells whether l1 or l3 has initialisations instruction
    {
          c1=_hb_encoding.make_hb(start_location,l3->loc);
          c2=_hb_encoding.make_hb(l2->loc,l3->loc);
    }
    else
    {
      c1=_hb_encoding.make_hb(l1->loc,start_location);
      c2=_hb_encoding.make_hb(l2->loc,start_location);
    }

  }

  z3::expr conjecture=implies((s && c1),c2);
  return conjecture;
}

void program::build_ses(const input::program& input,z3::context& c) {

    bool flag=0;
  unsigned int count_global_occur;
  unordered_set<std::shared_ptr<instruction>> RF_SOME;
  unordered_set<std::shared_ptr<instruction>> RF_WS;
  unordered_map<std::shared_ptr<instruction>,bool> if_pre;
  std::shared_ptr<cssa::instruction> write3;                       //for rf_ws
  std::shared_ptr<cssa::instruction> write4;                        //for_rf_ws
  std::shared_ptr<cssa::instruction> write5;                       // for fr
  std::shared_ptr<cssa::instruction> write6;                       // for fr
  z3::expr conjec1=c.bool_val(true);
  z3::expr conjec2=c.bool_val(true);
  z3::expr conjec3=c.bool_val(true);
  z3::expr conjec4=c.bool_val(true);
  z3::expr conjec5=c.bool_val(true);
  z3::expr conjec6=c.bool_val(true);
  z3::expr conjec7=c.bool_val(true);
  for(std::string v1:input.globals)
  {

      RF_WS.clear();
      if_pre.clear();
      count_global_occur=0;
      std::cout<<"\nglobal variables "<<v1;
      for(unsigned int k=0;k<threads.size();k++)
      {
        for(unsigned int n=0;n<input.threads[k].size();n++)
        {
          for(variable v2:(threads[k]->instructions[n])->variables_read_copy)
          {
            RF_SOME.clear();
            flag=0;

            std::cout<<"\nread outside:\t"<<v2<<"\n";
            if(check_correct_global_variable(v2,v1))
            {

                for(unsigned int k2=0;k2<threads.size();k2++)
                {
                  thread& thread = *threads[k2];
                  for(unsigned int n2=0;n2<input.threads[k2].size();n2++)
                  {
                    //std::cout<<"thread[i].instr\t"<<thread[n2].instr<<"\n";

                    for(variable v3:(threads[k2]->instructions[n2])->variables_write)
                    {
                         std::cout<<"\nwrite outside\t"<<v3<<"\n";
                        if(check_correct_global_variable(v3,v1))
                        {
                          std::cout<<"\ninside\n";
                          if(k2!=k)
                          {
                              RF_SOME.insert(threads[k2]->instructions[n2]);
                              RF_WS.insert(threads[k2]->instructions[n2]);
                              count_global_occur++;
                              // std::cout<<"\nread:\t"<<v2<<"\n";
                              // std::cout<<"\nwrite\t"<<v3<<"\n";
                              conjec1=build_rf_val(threads[k2]->instructions[n2],threads[k]->instructions[n],c,v1,false);
                              rf_val=rf_val && conjec1;
                              //std::cout<<"\nconjec1 in 1\t"<<conjec1<<"\n";
                              //std::cout<<"\nrf_val\t"<<rf_val<<"\n";

                              conjec2=build_rf_grf(threads[k2]->instructions[n2],threads[k]->instructions[n],c,v1,false,input);
                              rf_grf=rf_grf && conjec2;
                              //std::cout<<"\nconjec2 in 1\t"<<conjec2<<"\n";
                              //std::cout<<"\nrf_grf\t"<<rf_grf<<"\n";

                          }
                          else if(n2<n)
                          {
                              RF_SOME.insert(threads[k2]->instructions[n2]);
                              RF_WS.insert(threads[k2]->instructions[n2]);
                              count_global_occur++;
                              // std::cout<<"\nread: "<<v2<<"\n";
                              // std::cout<<"\nwrite\t"<<v3<<"\n";

                              conjec1=build_rf_val(threads[k2]->instructions[n2],threads[k]->instructions[n],c,v1,false);
                              rf_val=rf_val && conjec1;
                              // std::cout<<"\nconjec1 in 2\t"<<conjec1<<"\n";
                              // std::cout<<"\nrf_val\t"<<rf_val<<"\n";

                              conjec2=build_rf_grf(threads[k2]->instructions[n2],threads[k]->instructions[n],c,v1,false,input);
                              rf_grf=rf_grf && conjec2;
                              // std::cout<<"\nconjec2 in 2\t"<<conjec2<<"\n";
                              // std::cout<<"\nrf_grf\t"<<rf_grf<<"\n";

                          }
                        }
                    }
                    if(flag!=1)
                    {
                        for(variable vi:new_globals)
                        {
                          if(check_correct_global_variable(vi,v1))
                          {
                            RF_SOME.insert(threads[k]->instructions[n]);
                            RF_WS.insert(threads[k]->instructions[n]);
                            if_pre[threads[k]->instructions[n]]=1;

                            count_global_occur++;
                            // std::cout<<"\nread: "<<v2<<"\n";
                            // std::cout<<"\nwrite initial\t"<<vi<<"\n";

                            conjec1=build_rf_val(threads[k]->instructions[n],threads[k]->instructions[n],c,v1,true);
                            rf_val=rf_val && conjec1;
                            // std::cout<<"\nconjec1 in 3\t"<<conjec1<<"\n";
                            // std::cout<<"\nrf_val\t"<<rf_val<<"\n";

                            conjec2=build_rf_grf(threads[k]->instructions[n],threads[k]->instructions[n],c,v1,true,input);
                            rf_grf=rf_grf && conjec2;
                             // std::cout<<"\nconjec2 in 3\t"<<conjec2<<"\n";
                             // std::cout<<"\nrf_grf\t"<<rf_grf<<"\n";

                            flag=1;
                            break;
                          }
                        }
                    }


                  }
               }
            }



            if(count_global_occur>1)
              {

                unsigned int counter=1;

                // auto itr=RF_SOME.begin();
                 std::shared_ptr<cssa::instruction> write1;                       // for rf_some
                 std::shared_ptr<cssa::instruction> write2;                        // for rf_some

                 for(auto itr=RF_SOME.begin();itr!=RF_SOME.end();itr++)
                {
                  // std::cout<<"\ncheck itr\t"<<(*itr)->name<<"\n";
                  // std::cout<<"\nif_pre\t"<<if_pre[(*itr)]<<"\n";

                  if(counter%2==1)
                      {
                        write1=*itr;
                        if(counter!=1)
                         {
                            if((if_pre[write1]!=1)&&(if_pre[write2]!=1))
                            {
                                conjec3=build_rf_some(threads[k]->instructions[n],write1,write2,c,v1,false);
                                //std::cout<<"\nconjec3 in 1\t"<<conjec3<<"\n";
                                // std::cout<<"\nread in some (if) pre=false\n"<<threads[k]->instructions[n]->name;
                                // std::cout<<"\nwrite1 in some (if) pre=false\n"<<write1->name;
                                // std::cout<<"\nwrite2 in some (if) pre=false\n"<<write2->name;

                            }
                            else if(if_pre[write2]==1)
                            {
                                conjec3=build_rf_some(threads[k]->instructions[n],write2,write1,c,v1,true);
                                //std::cout<<"\nconjec3 in 2\t"<<conjec3<<"\n";
                                // std::cout<<"\nread in some (if) pre=false\n"<<threads[k]->instructions[n]->name;
                                // std::cout<<"\nwrite1 in some (if) pre=false\n"<<write1->name;
                                // std::cout<<"\nwrite2 in some (if) pre=false\n"<<write2->name;
                            }
                            else if(if_pre[write1]==1)
                            {
                                conjec3=build_rf_some(threads[k]->instructions[n],write1,write2,c,v1,true);
                                //std::cout<<"\nconjec3 in 3\t"<<conjec3<<"\n";
                                // std::cout<<"\nread in some (else) pre=true\n"<<threads[k]->instructions[n]->name;
                                // std::cout<<"\nwrite1 in some (else) pre=true\n"<<write1->name;
                                // std::cout<<"\nwrite2 in some (else) pre=true\n"<<write2->name;
                            }

                         }
                      }
                      else if(counter%2==0)
                      {

                        write2= *itr;

                        if((if_pre[write1]!=1)&&(if_pre[write2]!=1))
                            {
                                conjec3=build_rf_some(threads[k]->instructions[n],write1,write2,c,v1,false);
                                //std::cout<<"\nconjec3 in 1\t"<<conjec3<<"\n";
                                // std::cout<<"\nread in some (if) pre=false\n"<<threads[k]->instructions[n]->name;
                                // std::cout<<"\nwrite1 in some (if) pre=false\n"<<write1->name;
                                // std::cout<<"\nwrite2 in some (if) pre=false\n"<<write2->name;

                            }
                            else if(if_pre[write2]==1)
                            {
                                conjec3=build_rf_some(threads[k]->instructions[n],write2,write1,c,v1,true);
                                //std::cout<<"\nconjec3 in 2\t"<<conjec3<<"\n";
                                // std::cout<<"\nread in some (if) pre=false\n"<<threads[k]->instructions[n]->name;
                                // std::cout<<"\nwrite1 in some (if) pre=false\n"<<write1->name;
                                // std::cout<<"\nwrite2 in some (if) pre=false\n"<<write2->name;
                            }
                            else if(if_pre[write1]==1)
                            {
                                conjec3=build_rf_some(threads[k]->instructions[n],write1,write2,c,v1,true);
                                //std::cout<<"\nconjec3 in 3\t"<<conjec3<<"\n";
                                // std::cout<<"\nread in some (else) pre=true\n"<<threads[k]->instructions[n]->name;
                                // std::cout<<"\nwrite1 in some (else) pre=true\n"<<write1->name;
                                // std::cout<<"\nwrite2 in some (else) pre=true\n"<<write2->name;
                            }

                      }
                      counter++;
                }

              }
              rf_some= rf_some && conjec3;
              //std::cout<<"\nrf_some\t"<<rf_some<<"\n";
              for(auto iterator1=RF_SOME.begin();iterator1!=RF_SOME.end();)
              {
                write5=*iterator1;
                iterator1++;
                for(auto iterator2=iterator1;iterator2!=RF_SOME.end();iterator2++)
                {
                  write6=*iterator2;
                  if((if_pre[write5]!=1)&&(if_pre[write6]!=1))
                  {
                    // std::cout<<"........\nwrite1 + first if\t"<<write5->name<<"\n";
                    // std::cout<<"\nread1 + first if\t"<<threads[k]->instructions[n]->name<<"\n";
                    // std::cout<<"\nwrite2 + first if\t"<<write6->name<<"\n..........";
                    conjec6=build_fr(write5,threads[k]->instructions[n],write6,c,v1,false,false,input);
                    conjec7=build_fr(write6,threads[k]->instructions[n],write5,c,v1,false,false,input);
                    fr = fr && conjec6 && conjec7;
                    //std::cout<<"\nfr in 1\t"<<fr<<"\n";
                  }
                  else if(if_pre[write6]==1)
                  {
                      // std::cout<<"........\nwrite1 + second if\t"<<write5->name<<"\n";
                      // std::cout<<"\nread1 + second if\t"<<threads[k]->instructions[n]->name<<"\n";
                      // std::cout<<"\nwrite2 + second if\t"<<write6->name<<"\n........";
                      //if(if_pre[write5]!=1)
                        conjec6=build_fr(write5,threads[k]->instructions[n],write6,c,v1,true,true,input);
                      //if(if_pre[write6]!=1)
                        conjec7=build_fr(write6,threads[k]->instructions[n],write5,c,v1,true,false,input);
                      fr = fr && conjec6 && conjec7;
                      //std::cout<<"\nfr in 2\t"<<fr<<"\n";
                  }
                  else if(if_pre[write5]==1)
                  {
                      // std::cout<<".......\nwrite1 + third if\t"<<write5->name<<"\n";
                      // std::cout<<"\nread1 + third if\t"<<threads[k]->instructions[n]->name<<"\n";
                      // std::cout<<"\nwrite2 + third if\t"<<write6->name<<"\n......";
                      conjec6=build_fr(write5,threads[k]->instructions[n],write6,c,v1,true,false,input);
                      conjec7=build_fr(write6,threads[k]->instructions[n],write5,c,v1,true,true,input);
                      fr = fr && conjec6 && conjec7;
                      //std::cout<<"\nfr in 3\t"<<fr<<"\n";
                  }
                  else
                  {
                    continue;
                  }
                }
              }

          }
        }
      }
      for(auto itr1=RF_WS.begin();itr1!=RF_WS.end();)
      {
        write3=*itr1;

        itr1++;
        for(auto itr2=itr1;itr2!=RF_SOME.end();itr2++)
        {
            write4=*itr2;
            //std::cout<<"\ncheck itr\t"<<write4->name<<"\n";
            if((if_pre[write4]!=1)&&(if_pre[write3]!=1))
            {
                // std::cout<<"\nwrite3 name "<<write3->name<<"\n";
                // std::cout<<"\nwrite4 name "<<write4->name<<"\n";
                conjec4=build_ws(write4,write3,c,false,input);
                rf_ws= rf_ws && conjec4;
                //std::cout<<"\nrf_ws in 1\t"<<rf_ws<<"\n";
            }
            else if((if_pre[write4]==1))
            {
                // std::cout<<"\nwrite3 name "<<write3->name<<"\n";
                // std::cout<<"write4 name: initialisation instruction\n";
                conjec4=build_ws(write4,write3,c,true,input);
                rf_ws= rf_ws && conjec4;
                //std::cout<<"\nrf_ws in 2\t"<<rf_ws<<"\n";
            }
            else if((if_pre[write3]==1))
            {
                // std::cout<<"\nwrite4 name "<<write4->name<<"\n";
                // std::cout<<"write3 name: initialisation instruction\n";
                conjec4=build_ws(write3,write4,c,true,input);
                rf_ws= rf_ws && conjec4;
                //std::cout<<"\nrf_ws in 3\t"<<rf_ws<<"\n";
            }
            else
            {
              continue;
            }
        }
      }
  }
  std::cout<<"\nrf_val\t"<<rf_val<<"\n";
  std::cout<<"\nrf_grf\t"<<rf_grf<<"\n";
  std::cout<<"\nrf_ws\t"<<rf_ws<<"\n";
  std::cout<<"\nrf_some\t"<<rf_some<<"\n";
  std::cout<<"\nfr\t"<<fr<<"\n";
}
