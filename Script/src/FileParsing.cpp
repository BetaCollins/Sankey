// FileParsing.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <Windows.h>
#include "Classification.h"
#include <cassert>




int _tmain(int argc, _TCHAR* argv[])
{
	std::cout << "FileParsing!" << std::endl;

	std::string fileName = "AHH16599_raw-table.txt";
	std::vector<std::string> lines;

	//scope the datafile input for RAII
	{
		std::ifstream datafile(fileName);
		if (datafile.is_open())
		{
			for (std::string line; std::getline(datafile, line);)
			{
				lines.push_back(line);
			}
		}
		else
		{
			std::cout << "File Not Found! - " << fileName << std::endl;
		}
	}

	

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

	std::vector<R16_read> classifications;

	for (auto line : tokens)
	{
		if (line.size() != 11)
		{
			std::cout << "Error, incorrect line length.  Expected 11 tokens but only received " << line.size() << std::endl;
			std::cout << ">>>";
			for (std::string s : line)
			{
				std::cout << s << ",";
			}
			std::cout << std::endl;
			continue;
		}
		R16_read c;
		c.value = atof(line[1].c_str());
		c.classification.push_back(line[4].substr(2));
		c.classification.push_back(line[5].substr(2));
		c.classification.push_back(line[6].substr(2));
		c.classification.push_back(line[7].substr(2));
		c.classification.push_back(line[8].substr(2));
		c.classification.push_back(line[9].substr(2));
		c.classification.push_back(line[10].substr(2));
		classifications.push_back(c);
	}


	for (R16_read c : classifications)
	{
		std::cout << c.value;
		for (std::string s : c.classification)
		{
			std::cout << "," << s;
		}
		std::cout << std::endl;
	}


	TreeNode root("", 0);

	for (R16_read c : classifications)
	{
		root.Insert(c);
	}

	std::vector<std::vector<std::string>> treeDump(8);
	root.DepthFirst(
		[&treeDump](TreeNode& t)
		{
			assert(t.GetDepth() < 8);
			treeDump[t.GetDepth()].push_back(t.GetLabel());
		}
	);
	int nodeCount = 0;
	root.DepthFirst(
		[&nodeCount](TreeNode& t)
	{
		++nodeCount;
	}
	);

	std::cout << "Number of lines:" << lines.size() << std::endl;
	std::cout << "Number of classifications: " << classifications.size() << std::endl;
	std::cout << "Number of nodes:" << nodeCount << std::endl;
	root.DepthFirst(
		[](TreeNode& t) 
		{ 
			std::cout << t.GetDepth();
			for (int i = 0; i < t.GetDepth(); i++) std::cout << "--->";
			if(!t.GetLabel().empty()) std::cout << t.GetLabel() << std::endl; 
			else std::cout << "BLANK" << std::endl;
		}
	);

	std::cout << "TREE DUMP" << std::endl;
	int dumpNodeCount = 0;
	for (auto d : treeDump)
	{
		for (auto l : d)
		{
			dumpNodeCount++;
			std::cout << l << ",";
		}
		std::cout << std::endl;
	}
	std::wcout << "TREEDUMP NODECOUNT:" << dumpNodeCount << std::endl;

	std::cout << std::endl << "Press Q to exit:" << std::endl;
	bool quit = false;
	while (!quit)
	{
		if (GetAsyncKeyState('Q'))
			quit = true;
	}


	return 0;
}
