#include <common/CLI11.hpp>
#include <common/djb2.hpp>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <part/initpart.hpp>
#include <part/licenses.hpp>
#include <random>
#include <strings.h>

int main(int argc, char **argv) {
	CLI::App app;
	argv = app.ensure_utf8(argv);
	app.require_subcommand(1);
	app.add_flag_callback("--licenses", print_licenses, "Print all 3rd party licenses and exit");

	mdfs::InitPartInfo initpartInfo;
	CLI::App *initpart = mdfs::make_initpart_app(initpartInfo, app);

	CLI11_PARSE(app, argc, argv);

	if (initpart->parsed()) { return mdfs::do_initpart(initpartInfo, initpart); }

	return EXIT_SUCCESS;
}