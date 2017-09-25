#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "System.h"
#include "DGNode.h"

using namespace std;

int Main(vector<string> args)
{
	cout << "Read Data!!!" << endl;
	filebuf fb;
	fb.open("data.txt",std::ios::in);
	istream is(&fb);

	vector<vector<DGNode>> nodes;
	Read(is, nodes);

//	while(!is.end())
//	{
//		DGNode node;
//		Read(is, node);
//		cout << node.label << "," << node.count << "," << node.parent << "," << node.first_child << "," << node.num_children << endl;
//	}
	fb.close();

	for(size_t i = 0; i < nodes.size(); i++)
	{
		cout << ">>>" << endl;
		for(size_t j = 0; j < nodes[i].size(); j++)
		{
			DGNode& node = nodes[i][j];
			cout << node.label << "," << node.count << "," << node.parent << "," << node.first_child << "," << node.num_children << endl;
		}
	}

	return 0;
}
