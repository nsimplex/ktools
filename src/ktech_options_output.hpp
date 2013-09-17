/*
Copyright (C) 2013  simplex

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef KTECH_OPTIONS_OUTPUT_HPP
#define KTECH_OPTIONS_OUTPUT_HPP

#include "ktech_common.hpp"
#include <tclap/CmdLine.h>
//#include "ktex/ktex.hpp"

#include <algorithm>

namespace KTech {
	namespace options {
		class Output : public TCLAP::StdOutput {
			std::string license;

			// Lists of options categories, in the order they
			// should be printed.
			typedef std::vector<std::string> categories_list_t;
			categories_list_t categories;

			// Inverse map of the above.
			typedef std::map<categories_list_t::value_type, size_t> categories_inverse_t;
			categories_inverse_t categories_inverse;

			// Map of options to category indexes.
			typedef std::map<const TCLAP::Arg*, size_t> category_map_t;
			category_map_t category_map;

			size_t get_arg_category_idx(const TCLAP::Arg* a) const {
				category_map_t::const_iterator it = category_map.find(a);
				
				// Put it at the end.
				if(it == category_map.end()) {
					return categories.size();
				}
				else {
					return it->second;
				}
			}

			class arg_comparer;
			friend class arg_comparer;
			class arg_comparer {
				const Output& o;
			public:
				arg_comparer(const Output* _o) : o(*_o) {}

				// Comparison based on category order.
				bool operator()(const TCLAP::Arg* a, const TCLAP::Arg* b) const {
					return o.get_arg_category_idx(a) < o.get_arg_category_idx(b);
				}
			};

			static bool isUnlabeled(TCLAP::Arg* a) {
				const std::string& desc = a->shortID();

				return desc.length() <= 1 || desc[1] != TCLAP::Arg::flagStartChar();
			}

			void getSortedArgs(const std::list<TCLAP::Arg*>& args, std::vector<TCLAP::Arg*>& sorted_args) const {
				sorted_args.clear();

				for(std::list<TCLAP::Arg*>::const_iterator it = args.begin(); it != args.end(); ++it) {
					if(!isUnlabeled(*it)) {
						sorted_args.push_back(*it);
					}
				}

				std::stable_sort(sorted_args.begin(), sorted_args.end(), arg_comparer(this));
			}

		protected:
			virtual void usage_short(TCLAP::CmdLineInterface& c, std::ostream& out);
			virtual void usage_long(TCLAP::CmdLineInterface& c, std::ostream& out);

		public:
			Output(const std::string& _license) : license(_license) {}

			void addCategory(const std::string& cat) {
				if(categories_inverse.count(cat)) {
					return;
				}

				categories_inverse.insert( std::make_pair(cat, categories.size()) );
				categories.push_back(cat);
			}

			void setArgCategory(const TCLAP::Arg& a, const std::string& cat) {
				categories_inverse_t::const_iterator it = categories_inverse.find(cat);
				if(it == categories_inverse.end()) {
					throw( Error("Invalid argument category '" + cat + "'") );
				}

				category_map.insert( std::make_pair(&a, it->second) );
			}

			virtual void usage(TCLAP::CmdLineInterface& c);

			virtual void version(TCLAP::CmdLineInterface& c);

			virtual void failure(TCLAP::CmdLineInterface& c, TCLAP::ArgException& e);
		};
	}
}


inline void KTech::options::Output::version(TCLAP::CmdLineInterface& c) {
	std::cout << c.getProgramName() << " " << c.getVersion() << std::endl;
	std::cout << std::endl;
	std::cout << license << std::endl;
}


inline void KTech::options::Output::usage_short(TCLAP::CmdLineInterface& c, std::ostream& out) {
	using namespace std;
	using namespace TCLAP;

	std::list<Arg*> argList = c.getArgList();

	string s = "Usage: " + c.getProgramName() + " [OPTION]...";

	for(ArgListIterator it = argList.begin(); it != argList.end(); ++it) {
		if((*it)->isRequired() || isUnlabeled(*it)) {
			s += " " + (*it)->shortID();
		}
	}

	spacePrint( out, s, 80, 0, 0 );
}

inline void KTech::options::Output::usage_long(TCLAP::CmdLineInterface& c, std::ostream& out) {
	using namespace std;
	using namespace TCLAP;

	std::vector<Arg*> sorted_args;
	getSortedArgs(c.getArgList(), sorted_args);

	const size_t nargs = sorted_args.size();
	const size_t ncats = categories.size();

	assert( nargs <= c.getArgList().size() );

	// Last category idx;
	size_t last_cat_idx = ncats + 1;

	for(std::vector<Arg*>::const_iterator it = sorted_args.begin(); it != sorted_args.end(); ++it) {
		const size_t cat_idx = get_arg_category_idx(*it);

		if(cat_idx != last_cat_idx) {
			if(cat_idx >= ncats) {
				assert( cat_idx == ncats );
				out << "Other options:" << endl;
			}
			else {
				out << categories[cat_idx] << ":" << endl;
			}
		}

		spacePrint(out, (*it)->longID(), 80, 4, 2);
		spacePrint(out, (*it)->getDescription(), 80, 9, 0);

		last_cat_idx = cat_idx;
	}
}

inline void KTech::options::Output::usage(TCLAP::CmdLineInterface& c) {
	using namespace std;

	usage_short(c, cout);
	cout << endl;
	usage_long(c, cout);
	cout << endl;
	//spacePrint(cout, c.getMessage(), 80, 0, 0);
	cout << c.getMessage() << endl;
}


inline void KTech::options::Output::failure(TCLAP::CmdLineInterface& c, TCLAP::ArgException& e) {
	using namespace std;
	using namespace TCLAP;

	const string& programName = c.getProgramName();
	string prefix = programName + ": ";

	cerr << prefix << e.error() << endl;
	if(e.argId().length() > 0 && e.argId().compare(" ")) {
		cerr << string(prefix.length(), ' ') << e.argId() << endl;
	}
	cerr << endl;
	usage_short(c, cerr);
	cerr << "Try '" << programName << " --help' for more information." << endl;

	throw ExitException(1);
}

#endif
