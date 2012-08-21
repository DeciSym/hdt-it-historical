/*
 * Tutorial01.cpp
 *
 *  Created on: 02/03/2011
 *      Author: mck
 */

#include <HDT.hpp>
#include <HDTFactory.hpp>
#include <Header.hpp>
#include <Dictionary.hpp>
#include <Triples.hpp>

#include <string>
#include <iostream>
#include <fstream>

#include "../src/triples/TriplesList.hpp"
#include "../src/triples/BitmapTriples.hpp"

#include "../src/util/StopWatch.hpp"

using namespace hdt;
using namespace std;

class ConvertProgress : public ProgressListener {
private:
public:
	virtual ~ConvertProgress() { }

    void notifyProgress(float level, const char *section) {
    	cout << section << ": " << level << " %";
    	cout << "\r " << section << ": " << level << " %                      \r";
		cout.flush();
	}

};

int main(int argc, char **argv) {
	int c;
	string query, inputFile, outputFile;
	bool measure = false;

	while( (c = getopt(argc,argv,"hq:o:m"))!=-1) {
		switch(c) {
		case 'h':
			break;
		case 'q':
			query = optarg;
			break;
		case 'o':
			outputFile = optarg;
			break;
		case 'm':
			measure = true;
			break;
		default:
			cout << "ERROR: Unknown option" << endl;
			return 1;
		}
	}

	if(argc-optind<2) {
		cout << "ERROR: You must supply an input and HDT File" << endl << endl;
		return 1;
	}

	inputFile = argv[optind];
	outputFile = argv[optind+1];

	HDT *hdt = HDTFactory::createDefaultHDT();
	ConvertProgress progress;
	StopWatch st;

	try {
		// LOAD
		hdt->loadFromHDT(inputFile.c_str());

		// CONVERT triples to TripleList
		TriplesList tlist;
		Triples *triples = hdt->getTriples();
		cout << "Old Triples -> TriplesList" << endl;
		st.reset();
		IteratorTripleID *it = triples->searchAll();
		tlist.insert(it);
		delete it;
		cout << "         Old Triples -> TriplesList time" << st <<  endl;

		// Convert tlist to OPS
		cout << "TriplesList sort OPS" << endl;
		st.reset();
		tlist.sort(OPS, &progress);
		cout << "    TriplesList sort OPS time: " << st << endl;

		// Generate new OPS BitmapTriples
		cout << "TriplesList to new BitmapTriples" << endl;
		HDTSpecification spec;
		spec.set("triples.component.order", "OPS");
		BitmapTriples bt(spec);
		st.reset();
		bt.load(tlist, &progress);
		cout << "       TriplesList to new BitmapTriples time" << st << endl;

		// Update Header
#if 0
		cout << "Update Header" << endl;
		string rootNode("_:triples");
		TripleString ts (rootNode, "", "");
		hdt->getHeader()->remove(ts);
		bt.populateHeader(*hdt->getHeader(), "_:triples");
#endif

		// SAVE
		cout << "Save to " << outputFile << endl;
		ofstream out(outputFile.c_str(), ios::binary | ios::out);
		ControlInformation ci;

		// HEADER
		ci.clear();
		ci.setHeader(true);
		hdt->getHeader()->save(out, ci, NULL);

		// DICTIONARY
		ci.clear();
		ci.setDictionary(true);
		hdt->getDictionary()->save(out, ci, NULL);

		// NEW TRIPLES
		ci.clear();
		ci.setTriples(true);
		bt.save(out, ci, NULL);

		out.close();

		delete hdt;
	} catch (char *e) {
		cout << "ERROR: " << e << endl;
	} catch (const char *e) {
		cout << "ERROR: " << e << endl;
	}
}



