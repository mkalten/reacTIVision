/*	Fiducial tracking library.
	Copyright (C) 2004 Ross Bencina <rossb@audiomulch.com>
	Maintainer (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.
 
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.
 
	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "treeidmap.h"
#include "default_trees.h"

#include <string.h>
#include <assert.h>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

static int find_maximum_tree_depth( const std::string& s )
{
    int result = 0;
    for( int i=1; i < (int)s.size(); ++i ){ // skip first character which is the black/white flag

        int d = s[i] - '0';
        if( d > result )
            result = d;
    }

    return result;
}

/*
static int find_maximum_descendent_count( const std::string& s )
{
    int result = 0;

    std::vector<int> adjacenciesAtLevel( s.size(), 0 );
    int currentLevel = 0;
    for( int i=1; i < (int)s.size(); ++i ){ // skip first character which is the black/white flag

        int d = s[i] - '0';

        if( d > 0 ){

            adjacenciesAtLevel[d-1]++;
            if( adjacenciesAtLevel[d-1] > result )
                result = adjacenciesAtLevel[d-1];

        }

        if( d < currentLevel ){
            for( std::vector<int>::iterator j = adjacenciesAtLevel.begin() + d;
                    j != adjacenciesAtLevel.begin() + currentLevel; ++j ){

                *j = 0;
            }
        }

        currentLevel = d;
    }

    return result;
}
*/


class TreeIdMapImplementation{
    TreeIdMap* owner_;

    struct charstar_less{
        bool operator()( const char *lhs, const char *rhs ) const
        {
            return strcmp( lhs, rhs ) < 0;
        }
    };

    typedef std::map<const char*, int, charstar_less> map_type;
    map_type treeIdMap_;

    std::vector< const char *> strings_;

public:
    TreeIdMapImplementation( TreeIdMap* treeidmap, const char *tree )
        : owner_( treeidmap )
    {

		int minNodeCount = 0x7FFF;
		int maxNodeCount = 0;
		int minDepth = 0x7FFF;
		int maxDepth = 0;
		int maxAdjacencies = 0;
		
		const char* file_buffer[1024];
		const char** tree_buffer;
		int tree_length = 0;
		
		if (strstr(tree,".trees")!=NULL) {
			std::ifstream is( tree );

			if( !is.good() ) {
				std::cout << "error opening tree file: " << tree << std::endl;
			} else {
				std::string s;
				while( !is.eof() ) {
					s.clear();
					is >> s;
					if( s.empty() ) continue;
					
					char *ss = new char[128];
					strcpy( ss , s.c_str() );
					file_buffer[tree_length] = ss;
					tree_length++;
				}
				
				tree_buffer = file_buffer;
			}
		} else if(strcmp(tree,"default")==0) {
			tree_buffer = default_tree;
			tree_length = default_tree_length;
		} else if(strcmp(tree,"small")==0) {
			tree_buffer = small_tree;
			tree_length = small_tree_length;
		} else if(strcmp(tree,"mini")==0) {
			tree_buffer = mini_tree;
			tree_length = mini_tree_length;
		}

		treeIdMap_.insert( std::make_pair( "w012211", YAMA_ID ));
		
		int id = 0;
		for (int j=0;j<tree_length;j++) {
			
			std::string s(tree_buffer[j]);
			
			int depthSequenceLength = (int)( s.size() - 1 );
			
			char *ss = new char[ s.size() + 1 ];
			strings_.push_back( ss );
			strcpy( ss, s.c_str() );
			std::pair<map_type::iterator, bool> i = treeIdMap_.insert( std::make_pair( ss, id++ ) );
			if( i.second ){
				if( depthSequenceLength < minNodeCount )
					minNodeCount = depthSequenceLength;
				if( depthSequenceLength > maxNodeCount )
					maxNodeCount = depthSequenceLength;
				
				int maxTreeDepth = find_maximum_tree_depth( s );
				
				if( maxTreeDepth < minDepth )
					minDepth = maxTreeDepth;
				if( maxTreeDepth > maxDepth )
					maxDepth = maxTreeDepth;
				
				/*
				 int maxNodeAdjacencies = find_maximum_descendent_count( s ) + 1;
				 if( maxNodeAdjacencies > maxAdjacencies )
				 maxAdjacencies = maxNodeAdjacencies;
					*/
			}else{
				std::cout << "error inserting tree '" << s << "' into map\n";
			}
		}
		
		maxAdjacencies = maxNodeCount;
		owner_->tree_count = treeIdMap_.size();
		owner_->min_node_count = minNodeCount;
		owner_->max_node_count = maxNodeCount;
		owner_->min_depth = minDepth;
		owner_->max_depth = maxDepth;
		owner_->max_adjacencies = maxAdjacencies;
	}


/*
    TreeIdMapImplementation( TreeIdMap* treeidmap )
        : owner_( treeidmap )
    {

        int minNodeCount = 0x7FFF;
        int maxNodeCount = 0;
        int minDepth = 0x7FFF;
        int maxDepth = 0;
        int maxAdjacencies = 0;

		treeIdMap_.insert( std::make_pair( "w012211", YAMA_ID ));
		
        int id = 0;
        for (int j=0;j<default_tree_length;j++) {

		std::string s(default_tree[j]);

                int depthSequenceLength = (int)( s.size() - 1 );

                char *ss = new char[ s.size() + 1 ];
                strings_.push_back( ss );
                strcpy( ss, s.c_str() );
                std::pair<map_type::iterator, bool> i = treeIdMap_.insert( std::make_pair( ss, id++ ) );
                if( i.second ){
                    if( depthSequenceLength < minNodeCount )
                        minNodeCount = depthSequenceLength;
                    if( depthSequenceLength > maxNodeCount )
                        maxNodeCount = depthSequenceLength;

                    int maxTreeDepth = find_maximum_tree_depth( s );

                    if( maxTreeDepth < minDepth )
                        minDepth = maxTreeDepth;
                    if( maxTreeDepth > maxDepth )
                        maxDepth = maxTreeDepth;

 
                    //int maxNodeAdjacencies = find_maximum_descendent_count( s ) + 1;
                    //if( maxNodeAdjacencies > maxAdjacencies )
                    //    maxAdjacencies = maxNodeAdjacencies;
 
                }else{
                    std::cout << "error inserting tree '" << s << "' into map\n";
            }
        }

		maxAdjacencies = maxNodeCount;
		owner_->tree_count = treeIdMap_.size();
        owner_->min_node_count = minNodeCount;
        owner_->max_node_count = maxNodeCount;
        owner_->min_depth = minDepth;
        owner_->max_depth = maxDepth;
        owner_->max_adjacencies = maxAdjacencies;
    }
*/

    ~TreeIdMapImplementation()
    {
        while( !strings_.empty() ){
            delete [] strings_.back();
            strings_.pop_back();
        }
    }

    int treestring_to_id( const char *treestring )
    {
        map_type::iterator i = treeIdMap_.find( treestring );
        if( i != treeIdMap_.end() ) return i->second;
        else  return INVALID_TREE_ID;
    }
};


void initialize_treeidmap( TreeIdMap* treeidmap, const char* tree )
{
    treeidmap->implementation_ = new TreeIdMapImplementation( treeidmap, tree );
}

void terminate_treeidmap( TreeIdMap* treeidmap )
{
    delete (TreeIdMapImplementation*)treeidmap->implementation_;
    treeidmap->implementation_ = 0;
}

// returns -1 for unfound id
int treestring_to_id( TreeIdMap* treeidmap, const char *treestring )
{
    return ((TreeIdMapImplementation*)treeidmap->implementation_)->treestring_to_id( treestring );
}
