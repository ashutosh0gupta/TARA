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

		inline int detect_next_one(std::vector<bool> Row, unsigned offset)
		{
			for(unsigned i = offset+1; i < Row.size(); i++)
			{
				if(Row[i]==true)
				{
					return i;
				}
			}
			return -1;
		}

		inline unsigned number_of_edges(std::vector<std::vector<bool>> Matrix)
		{
			unsigned count=0;
			for(unsigned i = 0; i < Matrix.size(); i++)
			{
				for(unsigned j = 0; j < Matrix.size(); j++)
				{
					if(Matrix[i][j]==true)
					{
						count++;
					}
				}

			}
			return count;
		}

		struct graph_vertices{
			bool multiple;	//multiple edges from this vertex to some vertex
			bool is_traversed;
			std::string name;
			unsigned type;
			unsigned index;			//row index of Adjacency_Matrix
			int next_index;	// if multiple edges then points to the next vertex to which the edge is directed
			static unsigned index_count;
		};
		unsigned graph_vertices::index_count=0;

		inline unsigned cycles_detected( std::stack< graph_vertices> A, graph_vertices G)
		{
			unsigned count=0;
			bool flag=false;
			while(!A.empty())
			{
				if(G.index==A.top().index)
				{
					flag=true;
				}
				if(G.index == A.top().index && count>2)
				{
						return 2;
				}
				else
				{
					A.pop();
					count++;
				}
			}
			if(flag)
				return 1;
			else
				return 0;
		}

		inline void search_cycles( std::stack< graph_vertices> A, graph_vertices G, unsigned trav_next_index)
		{
			std::string temp=G.name;
			std::cout<<G.name<<"<-";
			fflush(stdout);
			while(G.index!=A.top().index && !A.empty())
			{
				std::cout<<A.top().name<<"<-";
				fflush(stdout);
				A.pop();
			}
			std::cout<<temp<<"\n";
		}

#endif /* TEMP_FUNCTIONS_H_ */
