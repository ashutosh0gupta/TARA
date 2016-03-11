/*
 * temp_functions.h
 *
 *  Created on: Mar 10, 2016
 *      Author: admin-hp
 */
#include<vector>
#include<string>
#include<stack>
#ifndef TEMP_FUNCTIONS_H_
#define TEMP_FUNCTIONS_H_

		inline bool detect_multiple_ones(std::vector<bool> Row)
		{
			unsigned count=0;
			for(unsigned i = 0; i < Row.size(); i++)
			{
				if(Row[i]==true)
				{
					count++;
				}
				if(count > 1)
				{
					return true;
				}
			}
			return false;
		}

		inline bool detect_ones(std::vector<bool> Row)
		{
			for(unsigned i = 0; i < Row.size(); i++)
			{
				if(Row[i]==true)
				{
					return true;
				}
			}
			return false;
		}

		inline unsigned detect_next_one(std::vector<bool> Row, unsigned offset)
		{
			for(unsigned i = offset+1; i < Row.size(); i++)
			{
				if(Row[i]==true)
				{
					return i;
				}
			}
			return 0;
		}


		struct graph_vertices{
			bool multiple;	//multiple edges from this vertex to some vertex
			bool is_traversed;
			//std::string name;
			unsigned index;			//row index of Adjacency_Matrix
			unsigned next_index;	// if multiple edges then points to the next vertex to which the edge is directed
			static unsigned index_count;
		};
		unsigned graph_vertices::index_count=0;

		inline bool cycles_detected( std::stack< graph_vertices > A, graph_vertices G)
		{
			while(!A.empty())
			{
				if(G.index == A.top().index)
				{
					return true;
				}
				else
				{
					A.pop();
				}
			}
			return false;
		}

		inline void search_cycles( std::stack< graph_vertices > A, graph_vertices G, unsigned trav_next_index)
		{
			std::cout<<G.index<<"->";
			fflush(stdout);
			while(G.index!=A.top().index && !A.empty())
			{
				std::cout<<A.top().index<<"->";
				fflush(stdout);
				A.pop();
			}
		}

#endif /* TEMP_FUNCTIONS_H_ */
