
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
//#include <cstdlib>
#include <sstream>
#include <utility>
#include "System.h"
#include "DGNode.h"
#include "Classification.h"
#include "DGNode.h"
#include <numeric>

//function to read lines from cross platform files.  handles Windows \n\r, UNIX \n, and early Mac \r
std::istream& safeGetline(std::istream& is, std::string& t)
{
	t.clear();

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	for (;;) {
		int c = sb->sbumpc();
		switch (c) {
		case '\n':
			return is;
		case '\r':
			if (sb->sgetc() == '\n')
				sb->sbumpc();
			return is;
		case EOF:
			// Also handle the case when the last line has no line ending
			if (t.empty())
				is.setstate(std::ios::eofbit);
			return is;
		default:
			t += (char)c;
		}
	}
}

//validates the flattened hierarchy stored in a 2d vector of DGNodes.
//1. compares a DGNodes parent field with the parent's children field.
//2.
bool validate(const std::vector<std::vector<DGNode>>& nodes)
{
	std::stringstream errStream;
	bool valid = true;
	int levels = 8;
	for (int level = 0; level < levels; ++level)
	{
		errStream << ">>>>>>>LEVEL:" << level << " (count=" << nodes[level].size() << ")" << endl;
		for (int i = 0; i < nodes[level].size(); ++i)
		{
			int first_child = nodes[level][i].first_child;
			errStream << "NODE[" << level << "," << i << "]" << "::" << nodes[level][i].label \
					<< "::Parent[" << nodes[level][i].parent << "]::FirstChild[" << nodes[level][i].first_child \
					<< "]::NumChildren[" << nodes[level][i].num_children << "]";
			//check to see if the parent of this node is valid (valid is [parent > -1 && parent < the max index of the previous level])  -1 denotes a node with no parent
			if(nodes[level][i].parent > -1 || (level > 0 && nodes[level][i].parent > nodes[level - 1].size() - 1))
			{
				errStream << "(Parent valid)" << endl;
			}
			else
			{
				errStream << "(Parent INVALID)" << endl;
				//only set valid to false for invalid parent if we're not looking at the root level
				valid = (level !=0) ? false : valid;
			}
			for (int child = 0; child < nodes[level][i].num_children; ++child)
			{
				int childIndex = first_child + child;
				if(childIndex < 0
						|| childIndex > nodes[level + 1].size() - 1
						|| nodes[level + 1][childIndex].parent != i)
				{
					errStream << "Child::" << first_child + child << " (INVALID)" << endl;
					valid = false;
				}
				else
				{
					errStream << "Child::" << first_child + child << " (valid)" << endl;
				}

			}
		}
	}

#ifndef NDEBUG
	std::cout << errStream.str();
#else
	if(!valid) std::cout << errStream.str();
#endif


	return valid;
}

void WriteToConsole(TreeNode& tree)
{
	//Count the nodes in the tree
	int nodeCount = 0;
	tree.DepthFirst(
		[&nodeCount](TreeNode& t)
		{
			++nodeCount;
		},
		[](TreeNode& t) { return; }
	);

	std::cout << "Number of nodes:" << nodeCount << std::endl;
	tree.DepthFirst(
		[](TreeNode& t)
		{
			std::cout << t.GetDepth();
			for (int i = 0; i < t.GetDepth(); i++) std::cout << "--->";
			if(!t.GetLabel().empty()) std::cout << "[" << t.GetLabel() << "," << t.GetValue() << "]" << std::endl;
			else std::cout << "[BLANK," << t.GetValue() << "]" << std::endl;
		},
		[](TreeNode& t) { return; }
	);
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime()
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}

void log(std::string message)
{
	std::cout << "[" << currentDateTime() << "] " << message << std::endl;
}

int Main(std::vector<std::string> args)
{
	int retVal = 0;
	std::string fileName;

	log("parse_data.exe started!");

#ifndef NDEBUG
	log("DEBUG output enabled!");
#endif

	if(args.size() < 2)
	{
		log("Bad args:  1 expected [input_file] received 0");
		retVal = -1;
	}
	else
	{
		fileName = args[1];
		log("Running with args: " + fileName);
	}

	//place to store the lines from the file
	std::vector<std::string> lines;
	if(retVal == 0)
	{
		//scope the datafile input for RAII
		{
			std::ifstream datafile(fileName);
			if (datafile.is_open())
			{
				for (std::string line; safeGetline(datafile, line);)
				{
					if(!line.empty()) lines.push_back(line);
				}
			}
			else
			{
				log("File Not Found! - " + fileName);
				retVal = -1;
			}
		}
	}

	if(retVal == 0)
	{
		//parse the data file, tokenizing each line into a vector of strings so
		//that we have a 2d vector of string tokens.
		//parsing is based on tab delimiter
		std::vector<std::vector<std::string>> tokens(lines.size());
		std::string delimiter = "\t";

		for (size_t i = 0; i < lines.size(); i++)
		{
			size_t pos = 0;
			std::string token;
			while ((pos = lines[i].find(delimiter)))
			{
				if (pos == std::string::npos)
				{
					token = lines[i];
					tokens[i].push_back(token);
					break;
				}
				else
				{
					token = lines[i].substr(0, pos);
					lines[i].erase(0, pos + delimiter.length());
					tokens[i].push_back(token);
				}
			}
		}

		//transform the 2d vector of string tokens into R16_read classifications
		std::vector<R16_read> classifications;

		for (auto line : tokens)
		{
			if (line.size() != 11)
			{
				log("Error, incorrect line length.  Expected 11 tokens but only received " + line.size());
				std::stringstream errstream;
				errstream << ">>>";
				for (std::string s : line)
				{
					errstream << s << ",";
				}
				errstream << std::endl;
				log(errstream.str());
				continue;
			}
			R16_read c;
			//percentage of reads for this classification (second column in the file)
			c.value = atof(line[1].c_str());
			//get all levels of the classification
			//classification level prefixes are removed (i.e. for "g:Streptococcus" the "g:" will be discarded and only "Streptococcus" will be saved.
			c.classification.push_back(line[4].substr(2));  //kingdom
			c.classification.push_back(line[5].substr(2));	//phylum
			c.classification.push_back(line[6].substr(2));	//class
			c.classification.push_back(line[7].substr(2));	//order
			c.classification.push_back(line[8].substr(2));	//family
			c.classification.push_back(line[9].substr(2));	//genus
			c.classification.push_back(line[10].substr(2));	//species
			classifications.push_back(c);
		}

	#ifndef NDEBUG
		//debug:  sum all percentages to see if they equal 100%
		double cValue = std::accumulate(std::begin(classifications), std::end(classifications), 0.0, [](double sum, R16_read& r)->double { return sum + r.value; });

		std::stringstream classificationStream;
		classificationStream << "Accumulated classification values:" << cValue;
		log(classificationStream.str());

		//clear stream
		classificationStream.str(std::string());

		//debug:  console dump all classifications.
		for (R16_read c : classifications)
		{
			classificationStream << c.value;
			for (std::string s : c.classification)
			{
				classificationStream << "," << s;
			}
			classificationStream << std::endl;
		}
		log(classificationStream.str());

		//Write debug info to console as well as the tree contents
		log("Number of lines:" + lines.size());
		log("Number of classifications: " + classifications.size());
	#endif

		//build the tree structure from the classifications
		TreeNode root("", 0);
		for (R16_read c : classifications)
		{
			root.Insert(c);
		}

		root.UpdateValues();

	#ifndef NDEBUG
		log("DUMP TREE");
		//debug dump
		WriteToConsole(root);
	#endif

		//copy the tree into a 2d array of DGNode to allow writing to a binary file in the
		//format required for the GraphPhylogeny program.
		//DGNode is the structure used by GraphPhylogeny.
		std::vector<std::vector<DGNode>> dgnodes(8);
		root.DepthFirst(
			//pre-order function
			[&dgnodes](TreeNode& t)
			{
				Assert(t.GetDepth() < 8);
				DGNode node;
				node.count = t.GetValue();
				node.num_children = t.GetChildren().size();
				//first_child, if there are children, will be put one level down at the end of the vector
				node.first_child = (node.num_children > 0) ? dgnodes[t.GetDepth() + 1].size() : -1;
				node.label = t.GetLabel();
				//parent will be the last node in the vector one level up
				node.parent = dgnodes[t.GetDepth() - 1].size() - 1;
				dgnodes[t.GetDepth()].push_back(node);
			},
			//post-order function
			[](TreeNode& t){ return; }
		);

		//validate the dgnodes making sure the parent/child relationships make sense
		bool valid = validate(dgnodes);
		if(valid)
		{
			log("Writing tmp.dat!");
			filebuf fb;
			fb.open("tmp.dat",std::ios::out);
			ostream os(&fb);
			//serialize the dgnodes into the final file which will be used by the next program "GraphPhylogeny"
			Write(os, dgnodes);
		}
		else
		{
			log("Nodes are invalid.  No data file written!");
			retVal = -1;
		}
	}

	std::stringstream returnstring;
	returnstring << "parse_data.exe exiting with return code: ";
	returnstring << retVal;
	log(returnstring.str());
	return retVal;
}
