/* 
 * This file is part of the EPFLLTPN/DDQMC distribution (https://github.com/EPFLLTPN/DDQMC).
 * Copyright (c) 2016 Alexandra Nagy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * System_Model.cpp
 *
 *  Created on: Jul 5, 2016
 *      Author: Alexandra Nagy
 *              alex.nagy.phys@gmail.com
 *
 *      REFERENCES:
 *      - A. Nagy, V. Savona, Driven-dissipative quantum Monte Carlo method for open quantum systems, Physical Review A 97 (5), 052129, 2018
 *      - A. Nagy's PhD Dissertation, École polytechnique fédérale de Lausanne, Quantum Monte Carlo Approach to the non-equilibrium steady state of open quantum systems, 2020
 */

#include "../Header/System_Model.h"
#include <climits>

using namespace std;

unsigned long toInt(std::string const &s);

void System_Model::Init(string filename)
{
	//basic properties initialization

	string line;
	ifstream myfile;

	myfile.exceptions ( ifstream::failbit | ifstream::badbit );
	try
	{
		myfile.open (filename.c_str());

		for(int i=0;i<3;++i) getline(myfile, line);
					name = line;

		name.erase(name.end()-1);

		for(int i=0;i<5;++i) getline(myfile, line);
			prop.xSite = atoi(line.c_str());

		for(int i=0;i<5;++i) getline(myfile, line);
			prop.J_x = atof(line.c_str());

		for(int i=0;i<5;++i) getline(myfile, line);
			prop.J_y = atof(line.c_str());

		for(int i=0;i<5;++i) getline(myfile, line);
			prop.J_z = atof(line.c_str());

		for(int i=0;i<5;++i) getline(myfile, line);
			prop.gamma = atof(line.c_str());

		for(int i=0;i<5;++i) getline(myfile, line);
			prop.h = atof(line.c_str());

		for(int i=0;i<5;++i) getline(myfile, line);
			prop.theta = atof(line.c_str());

		for(int i=0;i<5;++i) getline(myfile, line);

		for(int j =0 ; j<prop.xSite; ++j){
			prop.reference[prop.xSite - 1 - j] = boost::lexical_cast<int>(line[j]);}

		getline(myfile, line);
		getline(myfile, line);
			for(int j =0 ; j<prop.xSite; ++j){
				prop.reference[(2*prop.xSite) - 1 - j] = boost::lexical_cast<int>(line[j]);}

		for(int i = (2* prop.xSite); i<Bl; ++i)
			prop.reference[i] = 0;

		prop.hop = (prop.J_x+prop.J_y)/4.0;
		prop.double_hop = (prop.J_x-prop.J_y)/4.0;

		myfile.close();
	}
	catch (ifstream::failure& e)
	{
		cout << "Exception opening/reading file!!";
	}
}

void System_Model::MPI_Data(MPI_Datatype & p, MPI_Datatype & s, MPI_Datatype & sp, MPI_Datatype & sh, MPI_Datatype & bitset_type)
{

	///////////////////////////////////////////////////////////////////////////////////
	//bitset type
	MPI_Datatype oldtypes00[1];
	int blockcounts00[1];
	MPI_Aint offsets00[1];

	blockcounts00[0] = ceil(Bl/8.0);

	oldtypes00[0] = MPI_BYTE;

	offsets00[0] = 0;

	MPI_Type_create_struct(1, blockcounts00, offsets00, oldtypes00, &bitset_type);
	MPI_Type_commit(&bitset_type);

	///////////////////////////////////////////////////////////////////////////////////
	//properties
	Model_Properties mod;
	MPI_Datatype oldtypes[3];
	int blockcounts[3];
	MPI_Aint offsets[3];

	blockcounts[0] = 1;
	blockcounts[1] = 8;
	blockcounts[2] = 1;

	oldtypes[0] = MPI_INTEGER;
	oldtypes[1] = MPI_DOUBLE;
	oldtypes[2] = bitset_type;

	MPI_Get_address(&mod.xSite, &offsets[0]);
	MPI_Get_address(&mod.J_x, &offsets[1]);
	MPI_Get_address(&mod.reference, &offsets[2]);

	offsets[2] = offsets[2] - offsets[0];
	offsets[1] = offsets[1] - offsets[0];
	offsets[0] = 0;

	MPI_Type_create_struct(3, blockcounts, offsets, oldtypes, &p);
	MPI_Type_commit(&p);

	///////////////////////////////////////////////////////////////////////////////////
	//statistics
	Stats proba;
	MPI_Datatype oldtypes2[2];
	int blockcounts2[2];
	MPI_Aint offsets2[2];

	blockcounts2[0] = 6;
	blockcounts2[1] = 4;

	oldtypes2[0] = MPI_INTEGER;
	oldtypes2[1] = MPI_CXX_DOUBLE_COMPLEX;

	MPI_Get_address(&proba.N_w, &offsets2[0]);
	MPI_Get_address(&proba.Mx, &offsets2[1]);

	offsets2[1] = offsets2[1] - offsets2[0];
	offsets2[0] = 0;

	MPI_Type_create_struct(2, blockcounts2, offsets2, oldtypes2, &s);
	MPI_Type_commit(&s);

	///////////////////////////////////////////////////////////////////////////////////
	//spawned data
	Spawned_data sp_probe;
	MPI_Datatype oldtypes3[2];
	int blockcounts3[2];
	MPI_Aint offsets3[2];

	blockcounts3[0] = 1;
	blockcounts3[1] = 2;

	oldtypes3[0] = bitset_type;
	oldtypes3[1] = MPI_INTEGER;

	MPI_Get_address(&sp_probe.det, &offsets3[0]);
	MPI_Get_address(&sp_probe.nbr_re, &offsets3[1]);

	offsets3[1] = offsets3[1] - offsets3[0];
	offsets3[0] = 0;

	MPI_Type_create_struct(2, blockcounts3, offsets3, oldtypes3, &sp);
	MPI_Type_commit(&sp);


	///////////////////////////////////////////////////////////////////////////////////
	//population control
	Pop_control pop_probe;
	MPI_Datatype oldtypes4[2];
	int blockcounts4[2];
	MPI_Aint offsets4[2];

	blockcounts4[0] = 1;
	blockcounts4[1] = 1;

	oldtypes4[0] = MPI_INTEGER;
	oldtypes4[1] = MPI_DOUBLE;

	MPI_Get_address(&pop_probe.ctrl, &offsets4[0]);
	MPI_Get_address(&pop_probe.shift, &offsets4[1]);

	offsets4[1] = offsets4[1] - offsets4[0];
	offsets4[0] = 0;

	MPI_Type_create_struct(2, blockcounts4, offsets4, oldtypes4, &sh);
	MPI_Type_commit(&sh);
}

void System_Model::MPI_Free(MPI_Datatype & p, MPI_Datatype & s, MPI_Datatype & sp, MPI_Datatype & sh, MPI_Datatype & bitset_type)
{
	MPI_Type_free(&s);
	MPI_Type_free(&p);
	MPI_Type_free(&sp);
	MPI_Type_free(&bitset_type);
	MPI_Type_free(&sh);
}


double System_Model::DiagonalElement(bitset<Bl> & det, int j)
{
	/*Contribution to Hamiltonian from spin interactions:
	 * For a lattice the number of bonds stored as Nbonds.
	 * Bonds of type 0-0 or 1-1 will give a contribution of J/4 to the matrix element,
	 * 0-1, 1-0 bonds will give -J/4 contribution.
	 */

		double element;
		bitset<Bl> determinant;
		int counter= 0;
		int Nbonds = 0;
		int col_right;

		if(j)
			determinant = det;
		else
		{
			for(int i = 0; i<prop.xSite;++i)
			{
				determinant[i] = det[i + prop.xSite];
			}
		}

		for(int pos = 0; pos < prop.xSite; ++pos)
		{
			++Nbonds;

			col_right = (prop.xSite + (pos+1)%prop.xSite)%prop.xSite;
			counter += determinant[col_right]^determinant[pos]; // -
		}

		element = (prop.J_z / 4.0) * (Nbonds - 2.0 * counter);

		return element;
}


string System_Model::OutputGen(int cycle, int target_nbr, double shift, bitset<Bl> reference, double tau, string & paath, int seed, double shift_cst) {

	string line;
	ofstream myfile;
	stringstream s, ss;
	string support;
	time_t now = time(0);
	s << path << "result/" << name  << "_site" << prop.xSite << "_Jy" << prop.J_y << "_Nbr"<< target_nbr << "_h" << prop.h;
	paath = s.str();
	s << ".out";

	myfile.exceptions(ofstream::failbit | ofstream::badbit);
	try {

		myfile.open(s.str().c_str());

		myfile << "Restart? - " << restart << endl;
		myfile << "Model:\t" << name << endl;
		myfile << "Jx:\t" << prop.J_x << endl;
		myfile << "Jy:\t" << prop.J_y << endl;
		myfile << "Jz:\t" << prop.J_z << endl;
		myfile << "Field:\t" << prop.h << endl;
		myfile << "Theta:\t" << prop.theta << endl;
		myfile << "Dissipation:\t" << prop.gamma << endl;
		myfile << "Reference:\t" << endl;
			for(int j = 0; j<prop.xSite;++j)
			{
				myfile << reference[prop.xSite -1 - j];
			}
			myfile << endl;


			for(int j = 0; j<prop.xSite;++j)
			{
				myfile << reference[(2*prop.xSite) -1 - j];
			}
			myfile << endl;


		myfile << endl;
		myfile << "---------------------------" << endl;
		myfile << endl; myfile << endl;

		myfile << "Number of iteration:\t" << cycle << endl;
		myfile << "Target walker number:\t" << target_nbr << endl;
		myfile << "Initial energy shift:\t" << shift << endl;
		myfile << "Time step:\t" << tau << endl;
		myfile << "Random seed:\t" << seed << endl;
		myfile << "Population control damping:\t" << shift_cst << endl;
		myfile << "Starting time:\t" << ctime(&now) << endl;
		myfile << "---------------------------" << endl;
		myfile << "Time \t Nbr of walk. \t Diag_walk \t Energy shift \t State number \t Mx \t My \t Mz \t  Diag_walk_re \t Diag_walk_im \t Real Mz \t Imag Mz"<< endl;
		myfile << "--------------------------------------------" << endl;

		myfile.close();
	} catch (ofstream::failure& e) {
		cout << "Exception opening/writing file";
	}


	support = s.str();
	support.erase(support.size()-3);
	ss.str("");
	ss << support << "_SUPPORT.out";

	myfile.exceptions(ofstream::failbit | ofstream::badbit);
	try {

		myfile.open(ss.str().c_str());

		myfile << "Restart? - " << restart << endl;
		myfile << "Model:\t" << name << endl;
		myfile << "Jx:\t" << prop.J_x << endl;
		myfile << "Jy:\t" << prop.J_y << endl;
		myfile << "Jz:\t" << prop.J_z << endl;
		myfile << "Field:\t" << prop.h << endl;
		myfile << "Theta:\t" << prop.theta << endl;
		myfile << "Dissipation:\t" << prop.gamma << endl;
		myfile << "Reference:\t" << endl;
		for(int j = 0; j<prop.xSite;++j)
		{
			myfile << reference[prop.xSite -1 - j];
		}
		myfile << endl;


		for(int j = 0; j<prop.xSite;++j)
		{
			myfile << reference[(2*prop.xSite) -1 - j];
		}
		myfile << endl;

		myfile << endl;
		myfile << "---------------------------" << endl;
		myfile << endl; myfile << endl;

		myfile << "Number of iteration:\t" << cycle << endl;
		myfile << "Target walker number:\t" << target_nbr << endl;
		myfile << "Initial energy shift:\t" << shift << endl;
		myfile << "Time step:\t" << tau << endl;
		myfile << "Random seed:\t" << seed << endl;
		myfile << "Population control damping:\t" << shift_cst << endl;
		myfile << "Starting time:\t" << ctime(&now) << endl;
		myfile << "---------------------------" << endl;
		myfile << "Time \t Nbr of walk. \t Diag_walk \t Energy shift \t State number \t Divisior \t Mx \t My \t Mz \t  Diag_walk_re \t Diag_walk_im \t Real Mz \t Imag Mz"<< endl;
		myfile << "--------------------------------------------" << endl;

		myfile.close();
	} catch (ofstream::failure& e) {
		cout << "Exception opening/writing file";
	}

	return s.str();
}

unsigned long toInt(std::string const &s) {
    static const std::size_t MaxSize = CHAR_BIT*sizeof(unsigned long);
    if (s.size() > MaxSize) return 0; // handle error or just truncate?

    std::bitset<MaxSize> bits;
    std::istringstream is(s);
    is >> bits;
    return bits.to_ulong();
}

