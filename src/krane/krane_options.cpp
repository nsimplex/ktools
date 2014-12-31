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

#include "krane.hpp"
#include "krane_options.hpp"
#include "ktools_options_customization.hpp"
#include <tclap/CmdLine.h>

#include <algorithm>
#include <iterator>
#include <cctype>


// Message appended to the bottom of the (long) usage statement.
static const std::string usage_message = 
"Converts the INPUT-PATH(s) into a Spriter project, placed in the directory\n\
OUTPUT-DIR, which is created if it doesn't exist.\n\
\n\
Whenever an INPUT-PATH is a directory, the files build.bin and anim.bin in\n\
it (if any) are used, otherwise INPUT-PATH itself is used. The location of\n\
build atlases is automatically determined from the build file, so they need\n\
not be given as arguments.";


namespace Krane {
	namespace options {
		Maybe<std::string> allowed_build;

		allowed_banks_t allowed_banks;

		Maybe<std::string> build_rename;

		Maybe<std::string> banks_rename;

		bool check_animation_fidelity = false;

		bool mark_atlas = false;

		int verbosity = 0;
		bool info = false;
	}
}

namespace Krane {
	namespace options_custom {
		using namespace KTools::options_custom;
	}
}


static const std::string TO_SCML = "Options for scml output";
/*
static const std::string TO_BIN = "Options for binary output";
*/
static const std::string OUTPUT_CTRL = "Options controlling the output mode";
static const std::string INPUT_CTRL = "Options filtering input selection";

void Krane::parse_commandline_options(int& argc, char**& argv, std::list<KTools::VirtualPath>& input_paths, Compat::Path& output_path) {
	using namespace Krane;
	using namespace TCLAP;
	using namespace Krane::options_custom;
	using namespace std;

	try {
		Output myOutput(license);

		CmdLine cmd(usage_message, ' ', PACKAGE_VERSION);
		cmd.setOutput(&myOutput);

		list<Arg*> args;

		{
			list<Arg*>& raw_args = cmd.getArgList();
			raw_args.push_back( raw_args.front() );
			raw_args.pop_front();
		}

		
		myOutput.addCategory(INPUT_CTRL);
		myOutput.addCategory(OUTPUT_CTRL);
		myOutput.addCategory(TO_SCML);
		//myOutput.addCategory(TO_BIN);


		/*
		SwitchArg info_flag("i", "info", "Prints information for the given input files instead of performing any conversion.");
		args.push_back(&info_flag);
		*/

		MyValueArg<string> allowed_build_opt("", "build", "Selects only a build with the given name.", false, "", "build name");
		args.push_back(&allowed_build_opt);
		myOutput.setArgCategory(allowed_build_opt, INPUT_CTRL);

		MyMultiArg<string> allowed_banks_opt("", "bank", "Selects only animations in the given banks.", false, "bank name");
		args.push_back(&allowed_banks_opt);
		myOutput.setArgCategory(allowed_banks_opt, INPUT_CTRL);

		SwitchArg mark_atlas_opt("", "mark-atlases", "Instead of performing any conversion, saves the atlases in the specified build as PNG, with their clipped regions shaded grey.");
		args.push_back(&mark_atlas_opt);
		myOutput.setArgCategory(mark_atlas_opt, OUTPUT_CTRL);

		MyValueArg<string> build_rename_opt("", "rename-build", "Renames the input build to the given name.", false, "", "build name");
		args.push_back(&build_rename_opt);
		//myOutput.setArgCategory(build_rename_opt, TO_SCML);

		MyValueArg<string> banks_rename_opt("", "rename-bank", "Renames the input banks to the given name.", false, "", "bank name");
		args.push_back(&banks_rename_opt);
		//myOutput.setArgCategory(banks_rename_opt, TO_SCML);

		SwitchArg check_anim_fidelity_opt("", "check-animation-fidelity", "Checks if the Spriter representation of the animations is faithful to the source animations.");
		args.push_back(&check_anim_fidelity_opt);
		myOutput.setArgCategory(check_anim_fidelity_opt, TO_SCML);

		MultiSwitchArg verbosity_flag("v", "verbose", "Increases output verbosity.");
		args.push_back(&verbosity_flag);

		SwitchArg quiet_flag("q", "quiet", "Disables text output. Overrides the verbosity value.");
		args.push_back(&quiet_flag);


		MultiArgumentOption multiinput_opt("INPUT-PATH", "Input path.", true, "INPUT-PATH");
		cmd.add(multiinput_opt);

		/*
		 * This only marks the option visually, since all paths are globbed by multiinput_opt.
		 */
		ArgumentOption dummy_output_opt("OUTPUT-PATH", "Output path.", false, "OUTPUT-DIR");
		dummy_output_opt.setVisuallyRequired(true);
		cmd.add(dummy_output_opt);


		for(list<Arg*>::reverse_iterator it = args.rbegin(); it != args.rend(); ++it) {
			cmd.add(*it);
		}


		cmd.parse(argc, argv);


		if(allowed_build_opt.isSet()) {
			options::allowed_build = Just(allowed_build_opt.getValue());
		}
		if(allowed_banks_opt.isSet()) {
			const vector<string>& banks = allowed_banks_opt.getValue();
			options::allowed_banks.insert(options::allowed_banks.begin(), banks.begin(), banks.end());
		}
		options::mark_atlas = mark_atlas_opt.getValue();
		if(build_rename_opt.isSet()) {
			options::build_rename = Just(build_rename_opt.getValue());
		}
		if(banks_rename_opt.isSet()) {
			options::banks_rename = Just(banks_rename_opt.getValue());
		}
		options::check_animation_fidelity = check_anim_fidelity_opt.getValue();

		if(quiet_flag.getValue()) {
			options::verbosity = -1;
		}
		else {
			options::verbosity = verbosity_flag.getValue();
		}
		
		/*
		options::info = info_flag.getValue();
		*/

		const std::vector<std::string>& all_paths = multiinput_opt.getValue();

		if(all_paths.size() < 2) {
			dummy_output_opt.forceRequired();
			cmd.parse(argc, argv);
			// This should never be throws, because the above cmd.parse() is meant
			// to raise an error and exit the program.
			throw KToolsError("Missing output-path argument.");
		}

		std::copy(all_paths.begin(), all_paths.end(), std::back_inserter(input_paths));

		output_path = input_paths.back();
		input_paths.pop_back();

	} catch (ArgException& e) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit(1);
	} catch(exception& e) {
		cerr << "error: " << e.what() << endl;
		exit(1);
	}
}
