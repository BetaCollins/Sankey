#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <assert.h>
#include "System.h"
#include "DGNode.h"

using namespace std;

vector<vector<DGNode>> nodes = {
		//root (count=1)
		{
				{ -1, 0, 3, "", 100.00 }
		},
		//domain (count=3)
		{
				{ 0, 0, 1, "Archaea", 0.35 },
				{ 0, 1, 9, "Bacteria", 92.57 },
				{ 0, 0, 0, "", 7.05 }
		},
		//phylum (count=10)
		{
				{ 0, 0, 1, "", 0.34 },
				{ 1, 1, 1, "Actinobacteria", 4.7 },
				{ 1, 2, 1, "Bacteroidetes", 1.32 },
				{ 1, 3, 4, "Firmicutes", 37.04 },
				{ 1, 7, 1, "Fusobacteria", 0.80 },
				{ 1, 8, 1, "Nitrospira", 1.26 },
				{ 1, 9, 6, "Proteobacteria", 10.28 },
				{ 1, 15, 1, "", 0.49 },
				{ 1, 0, 0, "", 20.78 },
				{ 1, 0, 0, "", 15.52 }
		},
		//class (count=15)
		{
				{ 0, 0, 1, "", 0.34 },
				{ 1, 1, 3, "Actinobacteria", 4.70 },
				{ 2, 4, 1, "Bacteriodales", 1.18 },
				{ 3, 5, 4, "Bacilli", 26.27 },
				{ 3, 9, 1, "Cloistridia", 5.04 },
				{ 3, -1, 0, "", 2.22 },
				{ 3, -1, 0, "", 3.23 },
				{ 4, 10, 1, "Fusobacteriales", 0.8 },
				{ 5, 11, 1, "Nitrospira", 1.26 },
				{ 6, 12, 1, "Alphaproteobacteria", 0.91 },
				{ 6, 13, 2, "Betaproteobacteria", 1.54 },
				{ 6, 15, 1, "Deltaproteobacteria", 1.4 },
				{ 6, 16, 1, "Epsilonproteobacteria", 0.48 },
				{ 6, 17, 3, "Gammaproteobacteria", 4.85 },
				{ 6, -1, 0, "", 1.02 },
				{ 7, 20, 1, "", 0.49 }
		},
		//order (count=21)
		{
				{ 0, 0, 1, "", 0.34 },
				{ 1, 1, 3, "Actinomycetales", 2.02 },
				{ 1, 4, 1, "Bifidobacteriales", 1.71 },
				{ 1, -1, 0, "", 0.88 },
				{ 2, 5, 1, "Prevotellaceae", 1.00 },
				{ 3, 6, 1, "Bacillales", 0.81 },
				{ 3, 7, 3, "Lactobacillales", 21.73 },
				{ 3, -1, 0, "", 1.02 },
				{ 3, -1, 0, "", 2.67 },
				{ 4, 10, 5, "Clostridiales", 4.69 },
				{ 7, 15, 1, "Fusobateriaceae", 0.74 },
				{ 8, 16, 1, "Nirospirales", 1.26 },
				{ 9, 17, 1, "Rhizobiales", 0.70 },
				{ 10, 18, 1, "Burkholderiales", 0.70 },
				{ 10, 19, 1, "", 0.66 },
				{ 11, 20, 1, "", 1.08 },
				{ 12, 21, 1, "Campylobacterales", 0.48 },
				{ 13, 22, 1, "Enterobacteriales", 0.28 },
				{ 13, 23, 2, "Pseudomonadales", 3.25 },
				{ 13, 25, 1, "", 0.74 },
				{ 15, 26, 1, "", 0.49 }
		},
		//family (count=27)
		{
				{ 0, 0, 1, "", 0.34 },
				{ 1, 1, 1, "Corynebacteriaceae", 0.31 },
				{ 1, 2, 1, "Nocardioidaceae", 0.45 },
				{ 1, 3, 1, "", 0.42 },
				{ 2, 4, 3, "Bifidobacteriaceae", 1.71 },
				{ 4, 7, 1, "", 1.00 },
				{ 5, 8, 1, "Bacillaceae", 0.39 },
				{ 6, 9, 1, "Lactobacillaceae", 17.79 },
				{ 6, 0, 0, "", 0.91 },
				{ 6, 0, 0, "", 2.62 },
				{ 9, 10, 3, "Acidaminococcaceae", 1.61 },
				{ 9, 0, 0, "Clostridiaceae", 0.41 },
				{ 9, 0, 0, "Peptostreptococcaceae", 0.80 },
				{ 9, 13, 1, "", 0.30 },
				{ 9, 0, 0, "", 1.29 },
				{ 10, 14, 1, "", 0.74 },
				{ 11, 15, 1, "Nitrospiraceae", 1.26 },
				{ 12, 16, 1, "Phyllobacteriaceae", 0.60 },
				{ 13, 17, 1, "Comamonadaceae", 0.44 },
				{ 14, 18, 1, "", 0.66 },
				{ 15, 19, 1, "", 1.08 },
				{ 16, 20, 1, "Campylobacteraceae", 0.28 },
				{ 17, 21, 1, "Enterobacteriaceae", 0.28 },
				{ 18, 22, 1, "Moraxellaceae", 2.49 },
				{ 18, 23, 1, "Pseudomonadaceae", 0.76 },
				{ 19, 24, 1, "", 0.74 },
				{ 20, 25, 1, "", 0.49 }
		},
		//genus (count=26)
		{
				{ 0, 0, 1, "", 0.34 },
				{ 1, 0, 0, "Corynebacterium", 0.31 },
				{ 2, 1, 1, "Nocardioides", 0.45 },
				{ 3, 2, 1, "Kocuria", 0.41 },
				{ 4, 3, 2, "Bifidobacterium", 0.92 },
				{ 4, 0, 0, "", 0.26 },
				{ 4, 0, 0, "", 0.48 },
				{ 5, 5, 2, "Prevotella", 1.00 },
				{ 6, 0, 0, "Bacillus", 0.30 },
				{ 7, 7, 4, "Lactobacilus", 17.52 },
				{ 10, 11, 1, "Dendrosporobacter", 0.28 },
				{ 10, 12, 1, "Dialister", 0.34 },
				{ 10, 13, 1, "Veillonella", 0.81 },
				{ 13, 14, 1, "", 0.29 },
				{ 15, 15, 1, "Streptobacillus", 0.73 },
				{ 16, 16, 1, "Magnetobacterium", 1.24 },
				{ 17, 17, 1, "", 0.60 },
				{ 18, 18, 1, "Comamonas", 0.41 },
				{ 19, 19, 1, "", 0.66 },
				{ 20, 20, 1, "", 1.00 },
				{ 21, 0, 0, "Campylobacter", 0.27 },
				{ 22, 21, 1, "", 0.25 },
				{ 23, 22, 1, "Psychrobater", 2.48 },
				{ 24, 23, 1, "Pseudomonas", 0.72 },
				{ 25, 24, 1, "", 0.73 },
				{ 26, 25, 1, "", 0.47 }
		},
		//species (count=26)
		{
				{ 0, 0, 0, "", 0.34 },
				{ 2, 0, 0, "", 0.44 },
				{ 3, 0, 0, "Rosea", 0.41 },
				{ 4, 0, 0, "", 0.45 },
				{ 4, 0, 0, "", 0.34 },
				{ 7, 0, 0, "", 0.27 },
				{ 7, 0, 0, "", 0.60 },
				{ 9, 0, 0, "Crispatus", 0.52 },
				{ 9, 0, 0, "", 0.99 },
				{ 9, 0, 0, "", 6.71 },
				{ 9, 0, 0, "", 8.74 },
				{ 10, 0, 0, "", 0.28 },
				{ 11, 0, 0, "", 0.25 },
				{ 12, 0, 0, "", 0.81 },
				{ 13, 0, 0, "", 0.29 },
				{ 14, 0, 0, "", 0.73 },
				{ 15, 0, 0, "", 1.24 },
				{ 16, 0, 0, "", 0.60 },
				{ 17, 0, 0, "", 0.41 },
				{ 18, 0, 0, "", 0.66 },
				{ 19, 0, 0, "", 1.08 },
				{ 21, 0, 0, "", 0.25 },
				{ 22, 0, 0, "wp30", 2.47 },
				{ 23, 0, 0, "HDG1", 0.57 },
				{ 24, 0, 0, "", 0.73 },
				{ 25, 0, 0, "", 0.47 }
		}
};

int Main(vector<string> args)
{
	cout << "Build Data!!!" << endl;

	//init();

	filebuf fb;
	fb.open("data.txt",std::ios::out);
	ostream os(&fb);

	//dump nodes to console for data validation
	  ///DEBUG
	int levels = 8;
	assert(nodes.size() == levels);
	for (int level = 0; level < levels; ++level)
	{
		cerr << ">>>>>>>LEVEL:" << level << " (count=" << nodes[level].size() << ")" << endl;
		for (int i = 0; i < nodes[level].size(); ++i)
		{
			int first_child = nodes[level][i].first_child;
			cerr << "NODE[" << level << "," << i << "]" << "::" << nodes[level][i].label \
					<< "::Parent[" << nodes[level][i].parent << "]::FirstChild[" << nodes[level][i].first_child \
					<< "]::NumChildren[" << nodes[level][i].num_children << "]";
			//check to see if the parent of this node is valid (valid is [parent > -1 && parent < the max index of the previous level])  -1 denotes a node with no parent
			if(nodes[level][i].parent > -1 || (level > 0 && nodes[level][i].parent > nodes[level - 1].size() - 1))
			{
				cerr << "(Parent valid)" << endl;
			}
			else
			{
				cerr << "(Parent INVALID)" << endl;
			}
			for (int child = 0; child < nodes[level][i].num_children; ++child)
			{
				int childIndex = first_child + child;
				if(childIndex < 0
						|| childIndex > nodes[level + 1].size() - 1
						|| nodes[level + 1][childIndex].parent != i)
				{
					cerr << "Child::" << first_child + child << " (INVALID)" << endl;
				}
				else
				{
					cerr << "Child::" << first_child + child << " (valid)" << endl;
				}

			}
		}
	}
	  ////



	Write(os, nodes);

	fb.close();
	return 0;
}
