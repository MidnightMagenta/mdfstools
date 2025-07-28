#ifndef MDFS_LICENSES_H
#define MDFS_LICENSES_H

#include <cstdlib>
#include <iostream>
#include <string>

std::string licenses =
		"CLI11 license:\n\n"
		"CLI11 2.5 Copyright (c) 2017-2025 University of Cincinnati, developed by Henry\nSchreiner under NSF AWARD "
		"1414736. All rights reserved.\n\nRedistribution and use in source and binary forms of CLI11, with or "
		"without\nmodification, are permitted provided that the following conditions are met:\n\n1. Redistributions of "
		"source code must retain the above copyright notice, this\n   list of conditions and the following "
		"disclaimer.\n2. Redistributions in binary form must reproduce the above copyright notice,\n   this list of "
		"conditions and the following disclaimer in the documentation\n   and/or other materials provided with the "
		"distribution.\n3. Neither the name of the copyright holder nor the names of its contributors\n   may be used "
		"to endorse or promote products derived from this software without\n   specific prior written "
		"permission.\n\nTHIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\nANY EXPRESS "
		"OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\nWARRANTIES OF MERCHANTABILITY AND FITNESS "
		"FOR A PARTICULAR PURPOSE ARE\nDISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE "
		"FOR\nANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n(INCLUDING, BUT NOT "
		"LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\nLOSS OF USE, DATA, OR PROFITS; OR BUSINESS "
		"INTERRUPTION) HOWEVER CAUSED AND ON\nANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR "
		"TORT\n(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\nSOFTWARE, EVEN IF "
		"ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n";

void print_licenses() {
	std::cout << licenses;
	std::exit(0);
}

#endif